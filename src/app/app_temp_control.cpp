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
#include "drivers_dio_task.h"
#include "esp8266_io.h"
}

#include "espbot.hpp"
#include "espbot_cron.hpp"
#include "espbot_cfgfile.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_list.hpp"
#include "espbot_mem_mon.hpp"
#include "espbot_timedate.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_heater.hpp"
#include "app_remote_log.hpp"
#include "app_temp_log.hpp"
#include "app_temp_control.hpp"
#include "app_temp_ctrl_program.hpp"

// CTRL vars

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
    bool ctrl_paused;      // temperature control is always active
                           // but when (ctrl_paused == true)
                           // heater is always off
} ctrl;

//
// heater mngmt
//

static struct _heater_vars
{
    bool heater_on;
    uint32 last_heater_on;
    uint32 last_heater_off;
} heater_vars;

void switch_on_heater(void)
{
    // set local status on
    heater_vars.heater_on = true;
    heater_vars.last_heater_on = (cron_get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    DEBUG("TEMP CTRL -> heater on");
    // set real status depending on ctrl_paused
    if (!ctrl.ctrl_paused)
    {
        heater_start();
    }
    else
    {
        DEBUG("TEMP CTRL -> heater CTRL is suspended");
    }
}

void switch_off_heater(void)
{
    heater_vars.heater_on = false;
    heater_vars.last_heater_off = (cron_get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    heater_stop();
    DEBUG("TEMP CTRL -> heater off");
}

//
// CTRL
//

//
// MANUAL CTRL
//

int ctrl_settings_save(void);

int ctrl_off(void)
{
    // log ctrl mode changes
    if (ctrl.mode != MODE_OFF)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), mode_change, MODE_OFF);
    ctrl.mode = MODE_OFF;
    if (heater_vars.heater_on)
        switch_off_heater();
    uint32 time = cron_get_current_time()->timestamp;
    DEBUG("TEMP CTRL -> OFF [%d] %s", time, timedate_get_timestr(time));
    return ctrl_settings_save();
}

int ctrl_manual(int heater_on_period, int heater_off_period, int stop_after)
{
    if (ctrl.mode != MODE_MANUAL)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), mode_change, MODE_MANUAL);
    ctrl.mode = MODE_MANUAL;
    ctrl.heater_on_period = heater_on_period;
    ctrl.heater_off_period = heater_off_period;
    ctrl.stop_after = stop_after;
    ctrl.started_on = (cron_get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    switch_on_heater();
    DEBUG("TEMP CTRL -> MANUAL MODE [%d] %s", ctrl.started_on, timedate_get_timestr(ctrl.started_on));
    return ctrl_settings_save();
}

int ctrl_auto(int set_point, int stop_after)
{
    // log ctrl mode changes
    if (ctrl.mode != MODE_AUTO)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), mode_change, MODE_AUTO);
    ctrl.mode = MODE_AUTO;
    // log setpoint changes
    if (ctrl.setpoint != set_point)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), setpoint_change, set_point);
    ctrl.setpoint = set_point;
    ctrl.stop_after = stop_after;
    ctrl.started_on = (cron_get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    DEBUG("TEMP CTRL -> AUTO MODE [%d] %s", ctrl.started_on, timedate_get_timestr(ctrl.started_on));
    return ctrl_settings_save();
}

static void update_setpoint(void);

int ctrl_program(int id)
{
    ctrl.program_id = id;
    delete_program(ctrl.program);
    int res = load_program(id, ctrl.program);
    if (res != id)
    {
        // can't load the program
        if (ctrl.mode == MODE_PROGRAM)
            ctrl_off();
        return CFG_error;
    }
    // a program was found!!
    // log ctrl mode changes
    if (ctrl.mode != MODE_PROGRAM)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), mode_change, MODE_PROGRAM);
    ctrl.mode = MODE_PROGRAM;
    // default set-point
    ctrl.setpoint = ctrl.program->min_temp;
    ctrl.stop_after = 0;
    ctrl.started_on = (cron_get_current_time()->timestamp / 60) * 60; // rounding to previous minute
    DEBUG("TEMP CTRL -> PROGRAM MODE [%d] %s", ctrl.started_on, timedate_get_timestr(ctrl.started_on));
    update_setpoint();
    return ctrl_settings_save();
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
        if (get_temp(adv_settings.kd_dt) != INVALID_TEMP)
            de_dt = current_temp - get_temp(adv_settings.kd_dt);
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
    struct date *current_time = cron_get_current_time();
    uint32 heater_off_since = current_time->timestamp - heater_vars.last_heater_off;
    // the heater is off and was off since at least COLD_HEATER minutes
    if (!heater_vars.heater_on && (heater_off_since > (adv_settings.heater_cold * 60)))
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

#define CTRL_SETTINGS_FILENAME ((char *)f_str("ctrl_settings.cfg"))
//  {"ctrl_mode":,"manual_pulse_on":,"manual_pulse_off":,"auto_setpoint":,"program_id":,"pwr_off_timer":}

static int ctrl_settings_restore_cfg(void)
{
    ALL("ctrl_settings_restore_cfg");
    if (!Espfile::exists(CTRL_SETTINGS_FILENAME))
        return CFG_cantRestore;
    Cfgfile cfgfile(CTRL_SETTINGS_FILENAME);
    int mode = cfgfile.getInt(f_str("ctrl_mode"));
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(CTRL_SETTINGS_RESTORE_CFG_ERROR);
        ERROR("ctrl_settings_restore_cfg error");
        return CFG_error;
    }
    switch (mode)
    {
    case MODE_MANUAL:
    {
        int manual_pulse_on = cfgfile.getInt(f_str("manual_pulse_on"));
        int manual_pulse_off = cfgfile.getInt(f_str("manual_pulse_off"));
        int pwr_off_timer = cfgfile.getInt(f_str("pwr_off_timer"));
        if (cfgfile.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_SETTINGS_RESTORE_CFG_ERROR);
            ERROR("ctrl_settings_restore_cfg error");
            return CFG_error;
        }
        ctrl.mode = mode;
        ctrl.heater_on_period = manual_pulse_on;
        ctrl.heater_off_period = manual_pulse_off;
        ctrl.stop_after = pwr_off_timer;
    }
    break;
    case MODE_AUTO:
    {
        int auto_setpoint = cfgfile.getInt(f_str("auto_setpoint"));
        int pwr_off_timer = cfgfile.getInt(f_str("pwr_off_timer"));
        if (cfgfile.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_SETTINGS_RESTORE_CFG_ERROR);
            ERROR("ctrl_settings_restore_cfg error");
            return CFG_error;
        }
        ctrl.mode = mode;
        ctrl.setpoint = auto_setpoint;
        ctrl.stop_after = pwr_off_timer;
    }
    break;
    case MODE_PROGRAM:
    {
        int program_id = cfgfile.getInt(f_str("program_id"));
        if (cfgfile.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_SETTINGS_RESTORE_CFG_ERROR);
            ERROR("ctrl_settings_restore_cfg error");
            return CFG_error;
        }
        ctrl.mode = mode;
        ctrl.program_id = program_id;
    }
    break;

    default:
        ctrl.mode = MODE_OFF;
        break;
    }
    mem_mon_stack();
    return CFG_ok;
}

static int ctrl_settings_saved_cfg_updated(void)
{
    ALL("ctrl_settings_saved_cfg_updated");
    if (!Espfile::exists(CTRL_SETTINGS_FILENAME))
    {
        return CFG_notUpdated;
    }
    Cfgfile cfgfile(CTRL_SETTINGS_FILENAME);
    int mode = cfgfile.getInt(f_str("ctrl_mode"));
    if (cfgfile.getErr() != JSON_noerr)
        return CFG_error;
    if (ctrl.mode != mode)
        return CFG_notUpdated;
    switch (mode)
    {
    case MODE_MANUAL:
    {
        int manual_pulse_on = cfgfile.getInt(f_str("manual_pulse_on"));
        int manual_pulse_off = cfgfile.getInt(f_str("manual_pulse_off"));
        int pwr_off_timer = cfgfile.getInt(f_str("pwr_off_timer"));
        if (cfgfile.getErr() != JSON_noerr)
            return CFG_error;
        if ((ctrl.heater_on_period != manual_pulse_on) ||
            (ctrl.heater_off_period != manual_pulse_off) ||
            (ctrl.stop_after != pwr_off_timer))
            return CFG_notUpdated;
    }
    break;
    case MODE_AUTO:
    {
        int auto_setpoint = cfgfile.getInt(f_str("auto_setpoint"));
        int pwr_off_timer = cfgfile.getInt(f_str("pwr_off_timer"));
        if (cfgfile.getErr() != JSON_noerr)
            return CFG_error;
        if ((ctrl.setpoint != auto_setpoint) ||
            (ctrl.stop_after != pwr_off_timer))
            return CFG_notUpdated;
    }
    break;
    case MODE_PROGRAM:
    {
        int program_id = cfgfile.getInt(f_str("program_id"));
        if (cfgfile.getErr() != JSON_noerr)
            return CFG_error;
        if (ctrl.program_id != program_id)
            return CFG_notUpdated;
    }
    break;

    default:
        return CFG_notUpdated;
    }
    mem_mon_stack();
    return CFG_ok;
}

char *ctrl_vars_json_stringify(char *dest, int len)
{
    // {"timestamp":,"timezone":,"current_temp":,"current_humi":,"heater_status":,"auto_setpoint":,"ctrl_mode":,"program_name":"","pwr_off_timer_started_on":,"pwr_off_timer":,"ctrl_paused":}
    // {
    //   "timestamp": uint32,                 11 digits
    //   "timezone": int                       3 digits
    //   "current_temp": int,                  5 digits
    //   "current_humi": int,                  5 digits
    //   "heater_status": int,                 1 digit
    //   "auto_setpoint": int,                 4 digits
    //   "ctrl_mode": int,                     1 digit
    //   "program_name": "string",            32 digit
    //   "pwr_off_timer_started_on": uint32,  11 digits
    //   "pwr_off_timer": int                  4 digits
    //   "ctrl_paused": int                    1 digits
    // }
    int msg_len = 183 + 11 + 3 + 6 + 6 + 1 + 6 + 1 + 32 + 11 + 6 + 1 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_VARS_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("ctrl_settings_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    struct date *current_time = cron_get_current_time();
    fs_sprintf(msg,
               "{\"timestamp\":%d,"
               "\"timezone\":%d,",
               (current_time->timestamp), // sending UTC time
               timedate_get_timezone());
    fs_sprintf(msg + os_strlen(msg),
               "\"current_temp\":%d,"
               "\"current_humi\":%d,",
               get_temp(0),
               get_humi(0));
    fs_sprintf(msg + os_strlen(msg),
               "\"heater_status\":%d,"
               "\"auto_setpoint\":%d,",
               (is_heater_on() ? 1 : 0),
               ctrl.setpoint);
    fs_sprintf(msg + os_strlen(msg),
               "\"ctrl_mode\":%d,"
               "\"program_name\":\"%s\",",
               ctrl.mode,
               get_cur_program_name(ctrl.program_id));
    fs_sprintf(msg + os_strlen(msg),
               "\"pwr_off_timer_started_on\":%d,"
               "\"pwr_off_timer\":%d,",
               ctrl.started_on,
               ctrl.stop_after);
    fs_sprintf(msg + os_strlen(msg),
               "\"ctrl_paused\":%d}",
               ctrl.ctrl_paused);
    mem_mon_stack();
    return msg;
}

char *ctrl_settings_full_json_stringify(char *dest, int len)
{
    //  {"ctrl_mode":,"manual_pulse_on":,"manual_pulse_off":,"auto_setpoint":,"program_id":,"program_name":"","pwr_off_timer":}
    int msg_len = 119 + 1 + 6 + 6 + 6 + 2 + 32 + 6 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_SETTINGS_FULL_JSON_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("ctrl_settings_full_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"ctrl_mode\":%d,"
               "\"manual_pulse_on\":%d,"
               "\"manual_pulse_off\":%d,",
               ctrl.mode,
               ctrl.heater_on_period,
               ctrl.heater_off_period);
    fs_sprintf(msg + os_strlen(msg),
               "\"auto_setpoint\":%d,"
               "\"program_id\":%d,",
               ctrl.setpoint,
               ctrl.program_id);
    fs_sprintf(msg + os_strlen(msg),
               "\"program_name\":\"%s\","
               "\"pwr_off_timer\":%d}",
               get_cur_program_name(ctrl.program_id),
               ctrl.stop_after);
    mem_mon_stack();
    return msg;
}

char *ctrl_settings_json_stringify(char *dest, int len)
{
    //  {"ctrl_mode":,"manual_pulse_on":,"manual_pulse_off":,"auto_setpoint":,"program_id":,"pwr_off_timer":}
    int msg_len = 101 + 1 + 6 + 6 + 6 + 2 + 6 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_SETTINGS_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("ctrl_settings_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"ctrl_mode\":%d,"
               "\"manual_pulse_on\":%d,"
               "\"manual_pulse_off\":%d,",
               ctrl.mode,
               ctrl.heater_on_period,
               ctrl.heater_off_period);
    fs_sprintf(msg + os_strlen(msg),
               "\"auto_setpoint\":%d,"
               "\"program_id\":%d,",
               ctrl.setpoint,
               ctrl.program_id);
    fs_sprintf(msg + os_strlen(msg),
               "\"pwr_off_timer\":%d}",
               ctrl.stop_after);
    mem_mon_stack();
    return msg;
}

char *ctrl_paused_json_stringify(char *dest, int len)
{
    // {
    //   ctrl_paused: int,    1 digit
    // }
    // {"ctrl_paused":}
    int msg_len = 16 + 1 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_SETTINGS_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("ctrl_settings_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"ctrl_pause\":%d}",
               ctrl.ctrl_paused);
    mem_mon_stack();
    return msg;
}

int ctrl_settings_save(void)
{
    ALL("ctrl_settings_save");
    if (ctrl_settings_saved_cfg_updated() == CFG_ok)
        return CFG_ok;
    Cfgfile cfgfile(CTRL_SETTINGS_FILENAME);
    if (cfgfile.clear() != SPIFFS_OK)
        return CFG_error;
    char str[129];
    ctrl_settings_json_stringify(str, 129);
    int res = cfgfile.n_append(str, os_strlen(str));
    if (res < SPIFFS_OK)
        return CFG_error;
    mem_mon_stack();
    return CFG_ok;
}

// ADVANCED CRTL settings management

#define ADV_CTRL_SETTINGS_FILENAME ((char *)f_str("adv_ctrl_settings.cfg"))
// {
//   kp: int,             6 digit (5 digit and sign)
//   kd: int,             6 digit (5 digit and sign)
//   ki: int,             6 digit (5 digit and sign)
//   kd_dt: int,          6 digit
//   u_max: int,          6 digit (5 digit and sign)
//   heater_on_min: int,  5 digit
//   heater_on_max: int,  5 digit
//   heater_on_off: int,  5 digit
//   heater_cold: int,    5 digit
//   warm_up_period: int, 5 digit
//   wup_heater_on: int,  5 digit
//   wup_heater_off: int  5 digit
// }

static int adv_settings_restore_cfg(void)
{
    ALL("adv_settings_restore_cfg");
    if (!Espfile::exists(ADV_CTRL_SETTINGS_FILENAME))
        return CFG_cantRestore;
    Cfgfile cfgfile(ADV_CTRL_SETTINGS_FILENAME);
    int kp = cfgfile.getInt(f_str("kp"));
    int kd = cfgfile.getInt(f_str("kd"));
    int ki = cfgfile.getInt(f_str("ki"));
    int kd_dt = cfgfile.getInt(f_str("kd_dt"));
    int u_max = cfgfile.getInt(f_str("u_max"));
    int heater_on_min = cfgfile.getInt(f_str("heater_on_min"));
    int heater_on_max = cfgfile.getInt(f_str("heater_on_max"));
    int heater_on_off = cfgfile.getInt(f_str("heater_on_off"));
    int heater_cold = cfgfile.getInt(f_str("heater_cold"));
    int warm_up_period = cfgfile.getInt(f_str("warm_up_period"));
    int wup_heater_on = cfgfile.getInt(f_str("wup_heater_on"));
    int wup_heater_off = cfgfile.getInt(f_str("wup_heater_off"));
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(CTRL_ADV_SETTINGS_RESTORE_CFG_ERROR);
        ERROR("adv_settings_restore_cfg error");
        return CFG_error;
    }
    adv_settings.kp = kp;
    adv_settings.kd = kd;
    adv_settings.ki = ki;
    adv_settings.kd_dt = kd_dt;
    adv_settings.u_max = u_max;
    adv_settings.heater_on_min = heater_on_min;
    adv_settings.heater_on_max = heater_on_max;
    adv_settings.heater_on_off = heater_on_off;
    adv_settings.heater_cold = heater_cold;
    adv_settings.warm_up_period = warm_up_period;
    adv_settings.wup_heater_on = wup_heater_on;
    adv_settings.wup_heater_off = wup_heater_off;
    mem_mon_stack();
    return CFG_ok;
}

static int adv_settings_saved_cfg_updated(void)
{
    ALL("adv_settings_saved_cfg_updated");
    if (!Espfile::exists(ADV_CTRL_SETTINGS_FILENAME))
    {
        return CFG_notUpdated;
    }
    Cfgfile cfgfile(ADV_CTRL_SETTINGS_FILENAME);
    int kp = cfgfile.getInt(f_str("kp"));
    int kd = cfgfile.getInt(f_str("kd"));
    int ki = cfgfile.getInt(f_str("ki"));
    int kd_dt = cfgfile.getInt(f_str("kd_dt"));
    int u_max = cfgfile.getInt(f_str("u_max"));
    int heater_on_min = cfgfile.getInt(f_str("heater_on_min"));
    int heater_on_max = cfgfile.getInt(f_str("heater_on_max"));
    int heater_on_off = cfgfile.getInt(f_str("heater_on_off"));
    int heater_cold = cfgfile.getInt(f_str("heater_cold"));
    int warm_up_period = cfgfile.getInt(f_str("warm_up_period"));
    int wup_heater_on = cfgfile.getInt(f_str("wup_heater_on"));
    int wup_heater_off = cfgfile.getInt(f_str("wup_heater_off"));
    mem_mon_stack();
    if (cfgfile.getErr() != JSON_noerr)
    {
        // no need to raise an error, the cfg file will be overwritten
        // dia_error_evnt(CTRL_ADV_SETTINGS_SAVED_CFG_UPDATED_ERROR);
        // ERROR("adv_settings_saved_cfg_updated error");
        return CFG_error;
    }
    if ((adv_settings.kp != kp) ||
        (adv_settings.kd != kd) ||
        (adv_settings.ki != ki) ||
        (adv_settings.kd_dt != kd_dt) ||
        (adv_settings.u_max != u_max) ||
        (adv_settings.heater_on_min != heater_on_min) ||
        (adv_settings.heater_on_max != heater_on_max) ||
        (adv_settings.heater_on_off != heater_on_off) ||
        (adv_settings.heater_cold != heater_cold) ||
        (adv_settings.warm_up_period != warm_up_period) ||
        (adv_settings.wup_heater_on != wup_heater_on) ||
        (adv_settings.wup_heater_off != wup_heater_off))
    {
        return CFG_notUpdated;
    }
    return CFG_ok;
}

char *adv_settings_json_stringify(char *dest, int len)
{
    // {"kp":,"kd":,"ki":,"kd_dt":,"u_max":,"heater_on_min":,"heater_on_max":,"heater_on_off":,"heater_cold":,"warm_up_period":,"wup_heater_on":,"wup_heater_off":}
    int msg_len = 156 + 6 + 6 + 6 + 6 + 6 + 5 + 5 + 5 + 5 + 5 + 5 + 5 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_ADV_SETTINGS_CFG_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("adv_settings_cfg_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"kp\":%d,"
               "\"kd\":%d,"
               "\"ki\":%d,",
               adv_settings.kp,
               adv_settings.kd,
               adv_settings.ki);
    fs_sprintf(msg + os_strlen(msg),
               "\"kd_dt\":%d,"
               "\"u_max\":%d,"
               "\"heater_on_min\":%d,",
               adv_settings.kd_dt,
               adv_settings.u_max,
               adv_settings.heater_on_min);
    fs_sprintf(msg + os_strlen(msg),
               "\"heater_on_max\":%d,"
               "\"heater_on_off\":%d,"
               "\"heater_cold\":%d,",
               adv_settings.heater_on_max,
               adv_settings.heater_on_off,
               adv_settings.heater_cold);
    fs_sprintf(msg + os_strlen(msg),
               "\"warm_up_period\":%d,"
               "\"wup_heater_on\":%d,"
               "\"wup_heater_off\":%d}",
               adv_settings.warm_up_period,
               adv_settings.wup_heater_on,
               adv_settings.wup_heater_off);
    mem_mon_stack();
    return msg;
}

int adv_settings_cfg_save(void)
{
    ALL("adv_settings_cfg_save");
    if (adv_settings_saved_cfg_updated() == CFG_ok)
        return CFG_ok;
    Cfgfile cfgfile(ADV_CTRL_SETTINGS_FILENAME);
    if (cfgfile.clear() != SPIFFS_OK)
        return CFG_error;
    char str[222];
    adv_settings_json_stringify(str, 222);
    int res = cfgfile.n_append(str, os_strlen(str));
    if (res < SPIFFS_OK)
        return CFG_error;
    mem_mon_stack();
    return CFG_ok;
}

int set_adv_ctrl_settings(struct _adv_ctrl_settings *value)
{
    adv_settings.kp = value->kp;
    adv_settings.kd = value->kd;
    adv_settings.ki = value->ki;
    adv_settings.kd_dt = value->kd_dt;
    adv_settings.u_max = value->u_max;
    adv_settings.heater_on_min = value->heater_on_min;
    adv_settings.heater_on_max = value->heater_on_max;
    adv_settings.heater_on_off = value->heater_on_off;
    adv_settings.heater_cold = value->heater_cold;
    adv_settings.warm_up_period = value->warm_up_period;
    adv_settings.wup_heater_on = value->wup_heater_on;
    adv_settings.wup_heater_off = value->wup_heater_off;
    return adv_settings_cfg_save();
}

// struct _adv_ctrl_settings *get_adv_ctrl_settings(void)
// {
//     return &adv_settings;
// }

// end CFG management

void temp_control_init(void)
{
    ALL("temp_control_init");
    // heater
    heater_vars.heater_on = false;
    heater_vars.last_heater_on = 0;
    heater_vars.last_heater_off = 0;

    // CTRL suspended
    // CTRL suspended is not persistent
    ctrl.ctrl_paused = false;

    // programs
    init_program_list();

    // now remaining settings that are persistent

    if (ctrl_settings_restore_cfg() == CFG_ok)
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
        dia_warn_evnt(CTRL_INIT_DEFAULT_CTRL_CFG);
        WARN("temp_control_init no ctrl cfg available");
    }

    if (adv_settings_restore_cfg() != CFG_ok)
    {
        adv_settings.kp = CTRL_KP;
        adv_settings.kd = CTRL_KD;
        adv_settings.ki = CTRL_KI;
        adv_settings.kd_dt = CTRL_KD_DT;
        adv_settings.u_max = CTRL_U_MAX;
        adv_settings.heater_on_min = CTRL_HEATER_ON_MIN;
        adv_settings.heater_on_max = CTRL_HEATER_ON_MAX;
        adv_settings.heater_on_off = CTRL_HEATER_ON_OFF_P;
        adv_settings.heater_cold = COLD_HEATER;
        adv_settings.warm_up_period = WARM_UP_PERIOD;
        adv_settings.wup_heater_on = CTRL_HEATER_ON_WUP;
        adv_settings.wup_heater_off = CTRL_HEATER_OFF_WUP;
        dia_info_evnt(CTRL_INIT_DEFAULT_ADV_CTRL_CFG);
        INFO("temp_control_init using default adv ctrl cfg");
    }
    else
    {
        dia_info_evnt(CTRL_INIT_CUSTOM_ADV_CTRL_CFG);
        INFO("temp_control_init using custom advanced ctrl cfg");
    }
}

static void run_control_manual(void)
{
    ALL("run_control_manual");
    struct date *current_time = cron_get_current_time();

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
        if (!heater_vars.heater_on)
            switch_on_heater();
        return;
    }
    // heater on duty cycle
    if (heater_vars.heater_on)
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
    struct date *current_time = cron_get_current_time();
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
    if (heater_vars.heater_on)
    {
        // temperature reached the setpoint
        if (get_temp(0) >= ctrl.setpoint)
        {
            switch_off_heater();
            return;
        }
        // the heater on period has completed
        // or the heater off period is zero
        uint32 heater_on_since = current_time->timestamp - heater_vars.last_heater_on;
        if ((heater_on_since >= (ctrl.heater_on_period * 60)) &&
            (ctrl.heater_off_period > 0))
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
    struct date *current_time = cron_get_current_time();
    // find the current program
    if (ctrl.program == NULL)
    {
        dia_error_evnt(CTRL_UPDATE_SETPOINT_NO_PROGRAM_AVAILABLE);
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
        if ((ctrl.program->periods[idx].day_of_week == everyday) || (ctrl.program->periods[idx].day_of_week == current_time->day_of_week))
        {
            if ((ctrl.program->periods[idx].mm_start <= current_minutes) && (current_minutes < ctrl.program->periods[idx].mm_end))
            {
                set_point = ctrl.program->periods[idx].setpoint;
                break;
            }
        }
    }

    // and log setpoint changes
    if (ctrl.setpoint != set_point)
    {
        log_event(timedate_get_timestamp(), setpoint_change, set_point);
        DEBUG("CTRL set point changed to %d", set_point);
    }
    ctrl.setpoint = set_point;
}

static void run_control_program(void)
{
    ALL("run_control_program");
    struct date *current_time = cron_get_current_time();
    update_setpoint();
    // update ctrl vars
    compute_ctrl_vars();
    // check if it's time to switch the heater off
    if (heater_vars.heater_on)
    {
        // temperature reached the setpoint
        if (get_temp(0) >= ctrl.setpoint)
        {
            switch_off_heater();
            return;
        }
        // the heater on period has completed
        // or the heater off period is zero
        uint32 heater_on_since = current_time->timestamp - heater_vars.last_heater_on;
        if ((heater_on_since >= (ctrl.heater_on_period * 60)) &&
            (ctrl.heater_off_period > 0))
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
    DEBUG("CTRL STATUS at [%d] %s", cron_get_current_time()->timestamp, timedate_get_timestr(cron_get_current_time()->timestamp));
    switch (ctrl.mode)
    {
    case MODE_OFF:
        DEBUG("CTRL MODE OFF");
        if (heater_vars.heater_on)
            switch_off_heater();
        break;
    case MODE_MANUAL:
        DEBUG("CTRL MODE MANUAL (since [%d] %s)", ctrl.started_on, timedate_get_timestr(ctrl.started_on));
        run_control_manual();
        break;
    case MODE_AUTO:
        DEBUG("CTRL MODE AUTO (since [%d] %s)", ctrl.started_on, timedate_get_timestr(ctrl.started_on));
        run_control_auto();
        break;
    case MODE_PROGRAM:
        DEBUG("CTRL MODE PROGRAM (since [%d] %s)", ctrl.started_on, timedate_get_timestr(ctrl.started_on));
        run_control_program();
        break;
    default:
        break;
    }
    // check if exiting from CTRL suspended requires switching on the heater
    if (!ctrl.ctrl_paused)
    {
        if (heater_vars.heater_on && (!is_heater_on()))
            switch_on_heater();
    }
    if (heater_vars.heater_on && (!is_heater_on()))
        DEBUG("after ctrl, HEATER -> %d (actually suspended)", is_heater_on());
    else
        DEBUG("after ctrl, HEATER -> %d", is_heater_on());
    DEBUG("  last switched on -> [%d] %s", heater_vars.last_heater_on, timedate_get_timestr(heater_vars.last_heater_on));
    DEBUG(" last switched off -> [%d] %s", heater_vars.last_heater_off, timedate_get_timestr(heater_vars.last_heater_off));
    next_function(send_events_to_external_host);
}

int get_current_mode(void)
{
    return ctrl.mode;
}

int get_program_id(void)
{
    return ctrl.program_id;
}

void set_ctrl_paused(bool val)
{
    ctrl.ctrl_paused = val;
    if (ctrl.ctrl_paused)
    {
        DEBUG("TEMP CTRL -> heater ctrl paused");
        heater_stop();
        DEBUG("TEMP CTRL -> heater off");
    }
    else
    {
        DEBUG("TEMP CTRL -> heater ctrl active");
    }
}