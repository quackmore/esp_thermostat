/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

// SDK includes
extern "C"
{
#include "mem.h"
#include "library_dio_task.h"
#include "esp8266_io.h"
}

#include "espbot_cron.hpp"
#include "espbot_config.hpp"
#include "espbot_global.hpp"
#include "espbot_list.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_heater.hpp"
#include "app_remote_log.hpp"
#include "app_temp_log.hpp"
#include "app_temp_control.hpp"
#include "app_temp_ctrl_program.hpp"

//
// heater mngmt
//

static struct _heater_vars
{
    uint32 last_heater_on;
    uint32 last_heater_off;
} heater_vars;

void switch_on_heater(void)
{
    heater_vars.last_heater_on = (get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    heater_start();
    DEBUG("TEMP CTRL -> heater on");
}

void switch_off_heater(void)
{
    heater_vars.last_heater_off = (get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    heater_stop();
    DEBUG("TEMP CTRL -> heater off");
}

//
// CTRL
//

static void save_cfg(void);

static struct _ctrl_vars
{
    int mode;              // off/manual/auto/program
    int program_id;        //
    struct prgm *program;  //
    int heater_on_period;  // minutes
    int heater_off_period; // minutes
    uint32 started_on;     // the timestamp when ctrl was started
    int stop_after;        // minutes
    int setpoint;          //
} ctrl;

//
// MANUAL CTRL
//

void ctrl_off(void)
{
    // log ctrl mode changes
    if (ctrl.mode != MODE_OFF)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_OFF);
    }
    ctrl.mode = MODE_OFF;
    if (is_heater_on())
        switch_off_heater();
    uint32 time = get_current_time()->timestamp;
    DEBUG("TEMP CTRL -> OFF [%d] %s", time, esp_time.get_timestr(time));
    //
    // writing to flash could interfere with reading DHT22 ?????
    //
    save_cfg();
}

void ctrl_manual(int heater_on_period, int heater_off_period, int stop_after)
{
    if (ctrl.mode != MODE_MANUAL)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_MANUAL);
    }
    ctrl.mode = MODE_MANUAL;
    ctrl.heater_on_period = heater_on_period;
    ctrl.heater_off_period = heater_off_period;
    ctrl.stop_after = stop_after;
    ctrl.started_on = (get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    switch_on_heater();
    DEBUG("TEMP CTRL -> MANUAL MODE [%d] %s", ctrl.started_on, esp_time.get_timestr(ctrl.started_on));
    save_cfg();
}

void ctrl_auto(int set_point, int stop_after)
{
    // log ctrl mode changes
    if (ctrl.mode != MODE_AUTO)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_AUTO);
    }
    ctrl.mode = MODE_AUTO;
    // log setpoint changes
    if (ctrl.setpoint != set_point)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, setpoint_change, set_point);
    }
    ctrl.setpoint = set_point;
    ctrl.stop_after = stop_after;
    ctrl.started_on = (get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    DEBUG("TEMP CTRL -> AUTO MODE [%d] %s", ctrl.started_on, esp_time.get_timestr(ctrl.started_on));
    save_cfg();
}

static void update_setpoint(void);

void ctrl_program(int id)
{
    ctrl.program_id = id;
    delete_program(ctrl.program);
    ctrl.program = load_program(id);
    if (ctrl.program == NULL)
    {
        // didn't find the program
        esp_diag.error(TEMP_CTRL_PROGRAM_NOT_FOUND, id);
        ERROR("ctrl_program cannot find program %d", id);
        if(ctrl.mode == MODE_PROGRAM)
            ctrl_off();
        return;
    }
    // a program was found!!
    // log ctrl mode changes
    if (ctrl.mode != MODE_PROGRAM)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_PROGRAM);
    }
    ctrl.mode = MODE_PROGRAM;
    // dummy set-point
    ctrl.setpoint = 0;
    ctrl.stop_after = 0;
    ctrl.started_on = (get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    DEBUG("TEMP CTRL -> PROGRAM MODE [%d] %s", ctrl.started_on, esp_time.get_timestr(ctrl.started_on));
    save_cfg();
    update_setpoint();
}

//
// e(t) = set-point - T(t)
// u(t) = Kp * e(t) + Kd * (d e(t)/dt) + Ki * Integral(0,t)(e(x)dx)
// u(t) is normalized into the range (0 - Umax)
//
// heater_on  [minutes] = max (heaterOnMin, u(t))
// heater_off [minutes] = heaterOnOffPeriod - heater_on
//

static struct _adv_ctrl_settings adv_settings;

static void compute_ctrl_vars(void)
{
    // defined as static so that previous value is preserved
    static int e_t = 0;
    static int de_dt = 0;
    static int i_e = 0;
    int current_temp = get_temp(0);
    // on valid temperature reading calculate error
    // otherwise go on with previous values
    if (current_temp != INVALID_TEMP)
    {
        // e(t) = set-point - T(t)
        e_t = ctrl.setpoint - current_temp;
        // d e(t)/dt
        // on valid temperature reading calculate error derivative
        // otherwise go on with previous values
        if (get_temp(1) != INVALID_TEMP)
            de_dt = current_temp - get_temp(1);
        // Integral(t-60,t)(e(x) dx)
        i_e = 0;
        int cur_val;
        int prev_val = get_temp(1);
        if (prev_val == INVALID_TEMP)
            prev_val = current_temp;
        int idx;
        // DEBUG
        // fs_printf(">>> PID: Integral begin\n");
        for (idx = 0; idx < 60; idx++)
        {
            cur_val = get_temp(idx);
            if (cur_val == INVALID_TEMP)
            {
                i_e += (ctrl.setpoint - prev_val);
                // DEBUG
                // fs_printf("v:%d e:%d, ", cur_val, (ctrl.setpoint - prev_val));
            }
            else
            {
                i_e += (ctrl.setpoint - cur_val);
                // DEBUG
                // fs_printf("v:%d e:%d, ", cur_val, (ctrl.setpoint - cur_val));
                prev_val = cur_val;
            }
        }
        // DEBUG
        // fs_printf("\n>>> PID: Integral end, i_e = %d\n", i_e);
        i_e /= 60;
    }
    // u(t) = Kp * e(t) + Kd * (d e(t)/dt) + Ki * Integral(0,t)(e(x)dx)
    int u_t = adv_settings.kp * e_t + adv_settings.kd * de_dt + adv_settings.ki * i_e;
    // normalize u_t and clamp it to max (CTRL_H_OFF - CTRL_H_ON)
    u_t = ((adv_settings.heater_on_max) * u_t) / adv_settings.u_max;
    if (u_t > adv_settings.heater_on_max)
        u_t = adv_settings.heater_on_max;
    // calculate the heater on and off periods
    if (u_t > adv_settings.heater_on_min)
        ctrl.heater_on_period = u_t;
    else
        ctrl.heater_on_period = adv_settings.heater_on_min;
    ctrl.heater_off_period = adv_settings.heater_on_off - ctrl.heater_on_period;
    // DEBUG
    // fs_printf(">>> PID:              e(t): %d\n", e_t);
    // fs_printf(">>> PID:         d e(t)/dt: %d\n", de_dt);
    // fs_printf(">>> PID: I(t-60,t) e(x) dx: %d\n", i_e);
    // fs_printf(">>> PID:              u(t): %d\n", (adv_settings.kp * e_t + adv_settings.kd * de_dt + adv_settings.ki * i_e));
    // fs_printf(">>> PID:   normalized u(t): %d\n", u_t);
    // fs_printf(">>> PID:   heater on timer: %d\n", ctrl.heater_on_period);
    // fs_printf(">>> PID:  heater off timer: %d\n", ctrl.heater_off_period);

    // WARM-UP
    // during warm-up the heater-on and heater-off timers are replaced with adv_settings.heater_on_min
    static uint32 warm_up_started_on = 0;
    struct date *current_time = get_current_time();
    uint32 heater_off_since = current_time->timestamp - heater_vars.last_heater_off;
    // the heater is off and was off since at least COLD_HEATER minutes
    if (!is_heater_on() && (heater_off_since > (adv_settings.heater_cold * 60)))
    {
        warm_up_started_on = current_time->timestamp;
    }
    if ((current_time->timestamp - warm_up_started_on) < (adv_settings.warm_up_period * 60))
    {
        DEBUG("WARM-UP PHASE");
        ctrl.heater_on_period = adv_settings.wup_heater_on;
        ctrl.heater_off_period = adv_settings.wup_heater_off;
    }
    DEBUG("CTRL  heater on period -> %d", ctrl.heater_on_period);
    DEBUG("CTRL heater off period -> %d", ctrl.heater_off_period);
}

// CRTL settings management

#define CTRL_SETTINGS_FILENAME f_str("ctrl_settings.cfg")

static bool restore_cfg(void)
{
    ALL("temp_control_restore_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_RESTORE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_cfg FS not available");
        return false;
    }
    File_to_json cfgfile(CTRL_SETTINGS_FILENAME);
    if (!cfgfile.exists())
        return false;

    // ctrl_mode
    if (cfgfile.find_string(f_str("ctrl_mode")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
        ERROR("temp_control_restore_cfg cannot find \"ctrl_mode\"");
        return false;
    }
    ctrl.mode = atoi(cfgfile.get_value());
    switch (ctrl.mode)
    {
    case MODE_MANUAL:
    {
        // manual_pulse_on
        if (cfgfile.find_string(f_str("manual_pulse_on")))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_control_restore_cfg cannot find \"manual_pulse_on\"");
            return false;
        }
        ctrl.heater_on_period = atoi(cfgfile.get_value());
        // manual_pulse_off
        if (cfgfile.find_string(f_str("manual_pulse_off")))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_control_restore_cfg cannot find \"manual_pulse_off\"");
            return false;
        }
        ctrl.heater_off_period = atoi(cfgfile.get_value());
        // pwr_off_timer
        if (cfgfile.find_string(f_str("pwr_off_timer")))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_control_restore_cfg cannot find \"pwr_off_timer\"");
            return false;
        }
        ctrl.stop_after = atoi(cfgfile.get_value());
    }
    break;
    case MODE_AUTO:
    {
        // auto_setpoint
        if (cfgfile.find_string(f_str("auto_setpoint")))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_control_restore_cfg cannot find \"auto_setpoint\"");
            return false;
        }
        ctrl.setpoint = atoi(cfgfile.get_value());
        // pwr_off_timer
        if (cfgfile.find_string(f_str("pwr_off_timer")))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_control_restore_cfg cannot find \"pwr_off_timer\"");
            return false;
        }
        ctrl.stop_after = atoi(cfgfile.get_value());
    }
    break;
    case MODE_PROGRAM:
    {
        // program_id
        if (cfgfile.find_string(f_str("program_id")))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_control_restore_cfg cannot find \"program_id\"");
            return false;
        }
        ctrl.program_id = atoi(cfgfile.get_value());
    }
    break;

    default:
        break;
    }
    return true;
}

static bool saved_cfg_not_updated(void)
{
    ALL("temp_control_saved_cfg_not_updated");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_saved_cfg_not_updated FS not available");
        return true;
    }
    File_to_json cfgfile(CTRL_SETTINGS_FILENAME);
    if (!cfgfile.exists())
        return true;

    // ctrl_mode
    if (cfgfile.find_string(f_str("ctrl_mode")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_cfg_not_updated cannot find \"ctrl_mode\"");
        return true;
    }
    if (ctrl.mode != atoi(cfgfile.get_value()))
        return true;
    switch (ctrl.mode)
    {
    case MODE_MANUAL:
    {
        // manual_pulse_on
        if (cfgfile.find_string(f_str("manual_pulse_on")))
        {
            esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_control_saved_cfg_not_updated cannot find \"manual_pulse_on\"");
            return true;
        }
        if (ctrl.heater_on_period != atoi(cfgfile.get_value()))
            return true;
        // manual_pulse_off
        if (cfgfile.find_string(f_str("manual_pulse_off")))
        {
            esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_control_saved_cfg_not_updated cannot find \"manual_pulse_off\"");
            return true;
        }
        if (ctrl.heater_off_period != atoi(cfgfile.get_value()))
            return true;
        // pwr_off_timer
        if (cfgfile.find_string(f_str("pwr_off_timer")))
        {
            esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_control_saved_cfg_not_updated cannot find \"pwr_off_timer\"");
            return true;
        }
        if (ctrl.stop_after != atoi(cfgfile.get_value()))
            return true;
    }
    break;
    case MODE_AUTO:
    {
        // auto_setpoint
        if (cfgfile.find_string(f_str("auto_setpoint")))
        {
            esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_control_saved_cfg_not_updated cannot find \"auto_setpoint\"");
            return true;
        }
        if (ctrl.heater_on_period != atoi(cfgfile.get_value()))
            return true;
        // pwr_off_timer
        if (cfgfile.find_string(f_str("pwr_off_timer")))
        {
            esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_control_saved_cfg_not_updated cannot find \"pwr_off_timer\"");
            return true;
        }
        if (ctrl.stop_after != atoi(cfgfile.get_value()))
            return true;
    }
    break;
    case MODE_PROGRAM:
    {
        // program_id
        if (cfgfile.find_string(f_str("program_id")))
        {
            esp_diag.error(TEMP_CTRL_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_control_saved_cfg_not_updated cannot find \"program_id\"");
            return true;
        }
        if (ctrl.program_id != atoi(cfgfile.get_value()))
            return true;
    }
    break;
    default:
        break;
    }
    return false;
}

static void remove_cfg(void)
{
    ALL("temp_control_remove_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_REMOVE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_remove_cfg FS not available");
        return;
    }
    if (Ffile::exists(&espfs, (char *)CTRL_SETTINGS_FILENAME))
    {
        Ffile cfgfile(&espfs, (char *)CTRL_SETTINGS_FILENAME);
        cfgfile.remove();
    }
}

static void save_cfg(void)
{
    ALL("temp_control_save_cfg");
    if (saved_cfg_not_updated())
        remove_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_save_cfg FS not available");
        return;
    }
    Ffile cfgfile(&espfs, (char *)CTRL_SETTINGS_FILENAME);
    if (!cfgfile.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_CFG_CANNOT_OPEN_FILE);
        ERROR("temp_control_save_cfg cannot open %s", CTRL_SETTINGS_FILENAME);
        return;
    }

    switch (ctrl.mode)
    {
    case MODE_OFF:
    {
        //  {"ctrl_mode": }
        char buffer[(15 + 1 + 1)];
        fs_sprintf(buffer,
                   "{\"ctrl_mode\": %d}",
                   ctrl.mode);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    break;
    case MODE_MANUAL:
    {
        //  {"ctrl_mode": ,"manual_pulse_on": ,"manual_pulse_off": ,"pwr_off_timer": }
        char buffer[(74 + 1 + 5 + 5 + 5 + 1)];
        fs_sprintf(buffer,
                   "{\"ctrl_mode\": %d,"
                   "\"manual_pulse_on\": %d,",
                   ctrl.mode,
                   ctrl.heater_on_period);
        fs_sprintf(buffer + os_strlen(buffer),
                   "\"manual_pulse_off\": %d,"
                   "\"pwr_off_timer\": %d}",
                   ctrl.heater_off_period,
                   ctrl.stop_after);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    break;
    case MODE_AUTO:
    {
        //  {"ctrl_mode": ,"auto_setpoint": ,"pwr_off_timer": }
        char buffer[(51 + 1 + 5 + 5 + 1)];
        fs_sprintf(buffer,
                   "{\"ctrl_mode\": %d,\"auto_setpoint\": %d,\"pwr_off_timer\": %d}",
                   ctrl.mode,
                   ctrl.setpoint,
                   ctrl.stop_after);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    break;
    case MODE_PROGRAM:
    {
        //  {"ctrl_mode": "program_id": }
        char buffer[(29 + 1 + 1 + 5)];
        fs_sprintf(buffer,
                   "{\"ctrl_mode\": %d,\"program_id\": %d}",
                   ctrl.mode,
                   ctrl.program_id);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    break;
    default:
        break;
    }
}

// ADVANCED CRTL settings management

#define ADV_CTRL_SETTINGS_FILENAME f_str("adv_ctrl_settings.cfg")

static bool restore_adv_cfg(void)
{
    ALL("temp_control_restore_adv_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg FS not available");
        return false;
    }
    File_to_json cfgfile(ADV_CTRL_SETTINGS_FILENAME);
    if (!cfgfile.exists())
        return false;
    // {
    //   kp: int,             6 digit (5 digit and sign)
    //   kd: int,             6 digit (5 digit and sign)
    //   ki: int,             6 digit (5 digit and sign)
    //   u_max: int,          6 digit (5 digit and sign)
    //   heater_on_min: int,  5 digit
    //   heater_on_max: int,  5 digit
    //   heater_on_off: int,  5 digit
    //   heater_cold: int,    5 digit
    //   warm_up_period: int, 5 digit
    //   wup_heater_on: int,  5 digit
    //   wup_heater_off: int  5 digit
    // }

    // kp
    if (cfgfile.find_string(f_str("kp")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_INCOMPLETE);
        ERROR("temp_control_restore_adv_cfg cannot find \"kp\"");
        return false;
    }
    adv_settings.kp = atoi(cfgfile.get_value());
    // kd
    if (cfgfile.find_string(f_str("kd")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"kd\"");
        return false;
    }
    adv_settings.kd = atoi(cfgfile.get_value());
    // ki
    if (cfgfile.find_string(f_str("ki")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"ki\"");
        return false;
    }
    adv_settings.ki = atoi(cfgfile.get_value());
    // u_max
    if (cfgfile.find_string(f_str("u_max")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"u_max\"");
        return false;
    }
    adv_settings.u_max = atoi(cfgfile.get_value());
    // heater_on_min
    if (cfgfile.find_string(f_str("heater_on_min")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"heater_on_min\"");
        return false;
    }
    adv_settings.heater_on_min = atoi(cfgfile.get_value());
    // heater_on_max
    if (cfgfile.find_string(f_str("heater_on_max")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"heater_on_max\"");
        return false;
    }
    adv_settings.heater_on_max = atoi(cfgfile.get_value());
    // heater_on_off
    if (cfgfile.find_string(f_str("heater_on_off")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"heater_on_off\"");
        return false;
    }
    adv_settings.heater_on_off = atoi(cfgfile.get_value());
    // heater_cold
    if (cfgfile.find_string(f_str("heater_cold")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"heater_cold\"");
        return false;
    }
    adv_settings.heater_cold = atoi(cfgfile.get_value());
    // warm_up_period
    if (cfgfile.find_string(f_str("warm_up_period")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"warm_up_period\"");
        return false;
    }
    adv_settings.warm_up_period = atoi(cfgfile.get_value());
    // wup_heater_on
    if (cfgfile.find_string(f_str("wup_heater_on")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"wup_heater_on\"");
        return false;
    }
    adv_settings.wup_heater_on = atoi(cfgfile.get_value());
    // wup_heater_off
    if (cfgfile.find_string(f_str("wup_heater_off")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_adv_cfg cannot find \"wup_heater_off\"");
        return false;
    }
    adv_settings.wup_heater_off = atoi(cfgfile.get_value());
    return true;
}

static bool saved_adv_cfg_not_updated(void)
{
    ALL("temp_control_saved_adv_cfg_not_updated");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_FS_NOT_AVAILABLE);
        ERROR("temp_control_saved_adv_cfg_not_updated FS not available");
        return true;
    }
    File_to_json cfgfile(ADV_CTRL_SETTINGS_FILENAME);
    if (!cfgfile.exists())
        return true;
    // {
    //   kp: int,             6 digit (5 digit and sign)
    //   kd: int,             6 digit (5 digit and sign)
    //   ki: int,             6 digit (5 digit and sign)
    //   u_max: int,          6 digit (5 digit and sign)
    //   heater_on_min: int,  5 digit
    //   heater_on_max: int,  5 digit
    //   heater_on_off: int,  5 digit
    //   heater_cold: int,    5 digit
    //   warm_up_period: int, 5 digit
    //   wup_heater_on: int,  5 digit
    //   wup_heater_off: int  5 digit
    // }

    // kp
    if (cfgfile.find_string(f_str("kp")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"kp\"");
        return true;
    }
    if (adv_settings.kp != atoi(cfgfile.get_value()))
        return true;
    // kd
    if (cfgfile.find_string(f_str("kd")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"kd\"");
        return true;
    }
    if (adv_settings.kd != atoi(cfgfile.get_value()))
        return true;
    // ki
    if (cfgfile.find_string(f_str("ki")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"ki\"");
        return true;
    }
    if (adv_settings.ki != atoi(cfgfile.get_value()))
        return true;
    // u_max
    if (cfgfile.find_string(f_str("u_max")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"u_max\"");
        return true;
    }
    if (adv_settings.u_max != atoi(cfgfile.get_value()))
        return true;
    // heater_on_min
    if (cfgfile.find_string(f_str("heater_on_min")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"heater_on_min\"");
        return true;
    }
    if (adv_settings.heater_on_min != atoi(cfgfile.get_value()))
        return true;
    // heater_on_max
    if (cfgfile.find_string(f_str("heater_on_max")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"heater_on_max\"");
        return true;
    }
    if (adv_settings.heater_on_max != atoi(cfgfile.get_value()))
        return true;
    // heater_on_off
    if (cfgfile.find_string(f_str("heater_on_off")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"heater_on_off\"");
        return true;
    }
    if (adv_settings.heater_on_off != atoi(cfgfile.get_value()))
        return true;
    // heater_cold
    if (cfgfile.find_string(f_str("heater_cold")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"heater_cold\"");
        return true;
    }
    if (adv_settings.heater_cold != atoi(cfgfile.get_value()))
        return true;
    // warm_up_period
    if (cfgfile.find_string(f_str("warm_up_period")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"warm_up_period\"");
        return true;
    }
    if (adv_settings.warm_up_period != atoi(cfgfile.get_value()))
        return true;
    // wup_heater_on
    if (cfgfile.find_string(f_str("wup_heater_on")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"wup_heater_on\"");
        return true;
    }
    if (adv_settings.wup_heater_on != atoi(cfgfile.get_value()))
        return true;
    // wup_heater_off
    if (cfgfile.find_string(f_str("wup_heater_off")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_ADV_CFG_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_adv_cfg_not_updated cannot find \"wup_heater_off\"");
        return true;
    }
    if (adv_settings.wup_heater_off != atoi(cfgfile.get_value()))
        return true;
    return false;
}

static void remove_adv_cfg(void)
{
    ALL("temp_control_remove_adv_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_REMOVE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_remove_adv_cfg FS not available");
        return;
    }
    if (Ffile::exists(&espfs, (char *)ADV_CTRL_SETTINGS_FILENAME))
    {
        Ffile cfgfile(&espfs, (char *)ADV_CTRL_SETTINGS_FILENAME);
        cfgfile.remove();
    }
}

static void save_adv_cfg(void)
{
    ALL("temp_control_save_adv_cfg");
    if (saved_adv_cfg_not_updated())
        remove_adv_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_ADV_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_control_save_adv_cfg FS not available");
        return;
    }
    Ffile cfgfile(&espfs, (char *)ADV_CTRL_SETTINGS_FILENAME);
    if (!cfgfile.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_ADV_CFG_CANNOT_OPEN_FILE);
        ERROR("temp_control_save_adv_cfg cannot open %s", ADV_CTRL_SETTINGS_FILENAME);
        return;
    }
    // {
    //   kp: int,             6 digit (5 digit and sign)
    //   kd: int,             6 digit (5 digit and sign)
    //   ki: int,             6 digit (5 digit and sign)
    //   u_max: int,          6 digit (5 digit and sign)
    //   heater_on_min: int,  5 digit
    //   heater_on_max: int,  5 digit
    //   heater_on_off: int,  5 digit
    //   heater_cold: int,    5 digit
    //   warm_up_period: int, 5 digit
    //   wup_heater_on: int,  5 digit
    //   wup_heater_off: int  5 digit
    // }

    // {"kp": ,"kd": ,"ki": ,"u_max": ,"heater_on_min": ,"heater_on_max": ,"heater_on_off": ,"heater_cold": ,"warm_up_period": ,"wup_heater_on": ,"wup_heater_off": }
    char buffer[(158 + 6 + 6 + 6 + 6 + 5 + 5 + 5 + 5 + 5 + 5 + 5 + 1)];
    fs_sprintf(buffer,
               "{\"kp\": %d,"
               "\"kd\": %d,"
               "\"ki\": %d,"
               "\"u_max\": %d,"
               "\"heater_on_min\": %d,",
               adv_settings.kp,
               adv_settings.kd,
               adv_settings.ki,
               adv_settings.u_max,
               adv_settings.heater_on_min);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"heater_on_max\": %d,"
               "\"heater_on_off\": %d,"
               "\"heater_cold\": %d,",
               adv_settings.heater_on_max,
               adv_settings.heater_on_off,
               adv_settings.heater_cold);
    fs_sprintf(buffer + os_strlen(buffer),
               "\"warm_up_period\": %d,"
               "\"wup_heater_on\": %d,"
               "\"wup_heater_off\": %d}",
               adv_settings.warm_up_period,
               adv_settings.wup_heater_on,
               adv_settings.wup_heater_off);
    cfgfile.n_append(buffer, os_strlen(buffer));
}

void set_adv_ctrl_settings(struct _adv_ctrl_settings *value)
{
    adv_settings.kp = value->kp;
    adv_settings.kd = value->kd;
    adv_settings.ki = value->ki;
    adv_settings.u_max = value->u_max;
    adv_settings.heater_on_min = value->heater_on_min;
    adv_settings.heater_on_max = value->heater_on_max;
    adv_settings.heater_on_off = value->heater_on_off;
    adv_settings.heater_cold = value->heater_cold;
    adv_settings.warm_up_period = value->warm_up_period;
    adv_settings.wup_heater_on = value->wup_heater_on;
    adv_settings.wup_heater_off = value->wup_heater_off;
    save_adv_cfg();
}

struct _adv_ctrl_settings *get_adv_ctrl_settings(void)
{
    return &adv_settings;
}

// end CFG management

void temp_control_init(void)
{
    ALL("temp_control_init");
    // heater
    heater_vars.last_heater_on = 0;
    heater_vars.last_heater_off = 0;

    init_program_list();

    if (restore_cfg())
    {
        switch (ctrl.mode)
        {
        case MODE_OFF:
            ctrl_off();
            break;
        case MODE_MANUAL:
            ctrl_manual(ctrl.heater_on_period, ctrl.heater_off_period, ctrl.stop_after);
            break;
        case MODE_AUTO:
            ctrl_auto(ctrl.setpoint, ctrl.stop_after);
            break;
        case MODE_PROGRAM:
            ctrl_program(ctrl.program_id);
            break;
        default:
            break;
        }
    }
    else
    {
        // CTRL DEFAULT
        ctrl_off();
        esp_diag.warn(TEMP_CTRL_INIT_DEFAULT_CTRL_CFG);
        WARN("temp_control_init no ctrl cfg available");
    }

    if (!restore_adv_cfg())
    {
        adv_settings.kp = CTRL_KP;
        adv_settings.kd = CTRL_KD;
        adv_settings.ki = CTRL_KI;
        adv_settings.u_max = CTRL_U_MAX;
        adv_settings.heater_on_min = CTRL_HEATER_ON_MIN;
        adv_settings.heater_on_max = CTRL_HEATER_ON_MAX;
        adv_settings.heater_on_off = CTRL_HEATER_ON_OFF_P;
        adv_settings.heater_cold = COLD_HEATER;
        adv_settings.warm_up_period = WARM_UP_PERIOD;
        adv_settings.wup_heater_on = CTRL_HEATER_ON_WUP;
        adv_settings.wup_heater_off = CTRL_HEATER_OFF_WUP;
        esp_diag.info(TEMP_CTRL_INIT_DEFAULT_ADV_CTRL_CFG);
        INFO("temp_control_init using default adv ctrl cfg");
    }
    else
    {
        esp_diag.info(TEMP_CTRL_INIT_CUSTOM_ADV_CTRL_CFG);
        INFO("temp_control_init using custom advanced ctrl cfg");
    }
}

static void run_control_manual(void)
{
    ALL("run_control_manual");
    struct date *current_time = get_current_time();

    // switch off timer management (if enabled)
    if (ctrl.stop_after > 0)
    {
        uint32 running_since = current_time->timestamp - ctrl.started_on;
        if (running_since >= (ctrl.stop_after * 60))
        {
            ctrl_off();
            return;
        }
    }
    // running continuosly
    if (ctrl.heater_on_period == 0)
    {
        if (!is_heater_on())
            switch_on_heater();
        return;
    }
    // heater on duty cycle
    if (is_heater_on())
    {
        if ((current_time->timestamp - heater_vars.last_heater_on) >= (ctrl.heater_on_period * 60))
            switch_off_heater();
    }
    else
    {
        if ((current_time->timestamp - heater_vars.last_heater_off) >= (ctrl.heater_off_period * 60))
            switch_on_heater();
    }
}

static void run_control_auto(void)
{
    ALL("run_control_auto");
    struct date *current_time = get_current_time();
    // switch off timer management (if enabled)
    if (ctrl.stop_after > 0)
    {
        uint32 running_since = current_time->timestamp - ctrl.started_on;
        if (running_since >= (ctrl.stop_after * 60))
        {
            ctrl_off();
            return;
        }
    }
    // update ctrl vars
    compute_ctrl_vars();
    // check if it's time to switch the heater off
    if (is_heater_on())
    {
        // temperature reached the setpoint
        if (get_temp(0) >= ctrl.setpoint)
        {
            switch_off_heater();
            return;
        }
        // the heater on period has completed
        uint32 heater_on_since = current_time->timestamp - heater_vars.last_heater_on;
        if (heater_on_since >= (ctrl.heater_on_period * 60))
        {
            switch_off_heater();
            return;
        }
    }
    else
    {
        // here the heater is off

        // temperature reached the setpoint
        if (get_temp(0) >= ctrl.setpoint)
        {
            // do nothing
            return;
        }
        // when the heater off period exhausted then turn on the heater
        uint32 heater_off_since = current_time->timestamp - heater_vars.last_heater_off;
        if (heater_off_since >= (ctrl.heater_off_period * 60))
        {
            switch_on_heater();
            return;
        }
    }
}

static void update_setpoint(void)
{
    ALL("update_setpoint");
    struct date *current_time = get_current_time();
    // find the current program
    if(ctrl.program == NULL)
    {
        esp_diag.error(TEMP_CTRL_UPDATE_SETPOINT_NO_PROGRAM_AVAILABLE);
        ERROR("update_setpoint no program available");
        return;
    }
    // set the default temperature setpoint
    int set_point = ctrl.program->min_temp;
    // program
    // find the set_point for day_of_week, hours, minutes
    // struct date
    // {
    //     int year;
    //     char month;
    //     char day_of_month;
    //     char hours;
    //     char minutes;
    //     char seconds;
    //     char day_of_week;
    //     uint32 timestamp;
    // };
    int idx;
    int current_minutes = (current_time->hours * 60) + current_time->minutes;
    for (idx = 0; idx < ctrl.program->period_count; idx++)
    {
        if ((ctrl.program->period[idx].day_of_week == everyday) || (ctrl.program->period[idx].day_of_week == current_time->day_of_week))
        {
            if ((ctrl.program->period[idx].mm_start <= current_minutes) && (current_minutes < ctrl.program->period[idx].mm_end))
            {
                set_point = ctrl.program->period[idx].setpoint;
                break;
            }
        }
    }

    // and log setpoint changes
    if (ctrl.setpoint != set_point)
    {
        log_event(current_time->timestamp, setpoint_change, set_point);
        DEBUG("CTRL set point changed to %d", set_point);
    }
    ctrl.setpoint = set_point;
}

static void run_control_program(void)
{
    ALL("run_control_program");
    struct date *current_time = get_current_time();
    update_setpoint();
    // update ctrl vars
    compute_ctrl_vars();
    // check if it's time to switch the heater off
    if (is_heater_on())
    {
        // temperature reached the setpoint
        if (get_temp(0) >= ctrl.setpoint)
        {
            switch_off_heater();
            return;
        }
        // the heater on period has completed
        uint32 heater_on_since = current_time->timestamp - heater_vars.last_heater_on;
        if (heater_on_since >= (ctrl.heater_on_period * 60))
        {
            switch_off_heater();
            return;
        }
    }
    else
    {
        // here the heater is off

        // temperature reached the setpoint
        if (get_temp(0) >= ctrl.setpoint)
        {
            // do nothing
            return;
        }
        // when the heater off period exhausted then turn on the heater
        uint32 heater_off_since = current_time->timestamp - heater_vars.last_heater_off;
        if (heater_off_since >= (ctrl.heater_off_period * 60))
        {
            switch_on_heater();
            return;
        }
    }
}

void temp_control_run(void)
{
    DEBUG("CTRL STATUS at [%d] %s", get_current_time()->timestamp, esp_time.get_timestr(get_current_time()->timestamp));
    switch (ctrl.mode)
    {
    case MODE_OFF:
        DEBUG("CTRL MODE OFF");
        if (is_heater_on())
            switch_off_heater();
        break;
    case MODE_MANUAL:
        DEBUG("CTRL MODE MANUAL (since [%d] %s)", ctrl.started_on, esp_time.get_timestr(ctrl.started_on));
        run_control_manual();
        break;
    case MODE_AUTO:
        DEBUG("CTRL MODE AUTO (since [%d] %s)", ctrl.started_on, esp_time.get_timestr(ctrl.started_on));
        run_control_auto();
        break;
    case MODE_PROGRAM:
        DEBUG("CTRL MODE PROGRAM (since [%d] %s)", ctrl.started_on, esp_time.get_timestr(ctrl.started_on));
        run_control_program();
        break;
    default:
        break;
    }
    DEBUG("after ctrl, HEATER -> %d", is_heater_on());
    DEBUG("  last switched on -> [%d] %s", heater_vars.last_heater_on, esp_time.get_timestr(heater_vars.last_heater_on));
    DEBUG(" last switched off -> [%d] %s", heater_vars.last_heater_off, esp_time.get_timestr(heater_vars.last_heater_off));
    subsequent_function(send_events_to_external_host);
}

int get_current_mode(void)
{
    return ctrl.mode;
}

int get_pwr_off_timer(void)
{
    return ctrl.stop_after;
}

uint32 get_pwr_off_timer_started_on(void)
{
    return ctrl.started_on;
}

int get_auto_setpoint(void)
{
    return ctrl.setpoint;
}

int get_manual_pulse_on(void)
{
    return ctrl.heater_on_period;
}

int get_manual_pulse_off(void)
{
    return ctrl.heater_off_period;
}

int get_program_id(void)
{
    return ctrl.program_id;
}