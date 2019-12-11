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

#include "espbot_config.hpp"
#include "espbot_global.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_activity_log.hpp"
#include "app_cron.hpp"
#include "app_heater.hpp"
#include "app_temp_log.hpp"
#include "app_temp_control.hpp"

//
// CTRL
//
static int ctrl_mode; // off/manual/auto

static void save_cfg(void);

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
    esplog.all("%s\n", __FUNCTION__);
    heater_vars.last_heater_on = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    heater_start();
    esplog.debug("TEMP CTRL -> heater on\n");
}

void switch_off_heater(void)
{
    esplog.all("%s\n", __FUNCTION__);
    heater_vars.last_heater_off = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    heater_stop();
    esplog.debug("TEMP CTRL -> heater off\n");
}

//
// MANUAL CTRL
//
static struct _manual_ctrl_vars
{
    int heater_on_period;  // minutes, 0 minutes -> continuos
    int heater_off_period; // minutes
    uint32 started_on;     // the timestamp when manual ctrl was started
    int stop_after;        // minutes
} manual_ctrl_vars;

void ctrl_off(void)
{
    esplog.all("%s\n", __FUNCTION__);
    // log ctrl mode changes
    if (ctrl_mode != MODE_OFF)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_OFF);
    }
    ctrl_mode = MODE_OFF;
    if (is_heater_on())
        switch_off_heater();
    uint32 time = esp_sntp.get_timestamp();
    esplog.debug("TEMP CTRL -> OFF [%d] %s\n", time, esp_sntp.get_timestr(time));
    save_cfg();
}

void ctrl_manual(int heater_on_period, int heater_off_period, int stop_after)
{
    esplog.all("%s\n", __FUNCTION__);
    if (ctrl_mode != MODE_MANUAL)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_MANUAL);
    }
    ctrl_mode = MODE_MANUAL;
    manual_ctrl_vars.heater_on_period = heater_on_period;
    manual_ctrl_vars.heater_off_period = heater_off_period;
    manual_ctrl_vars.stop_after = stop_after;
    manual_ctrl_vars.started_on = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    switch_on_heater();
    esplog.debug("TEMP CTRL -> MANUAL MODE [%d] %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
    save_cfg();
}

static struct _auto_ctrl_vars
{
    int setpoint;
    uint32 started_on; // the timestamp when auto was started
    int stop_after;    // power off timer in minutes
    int heater_on_period;
    int heater_off_period;
} auto_ctrl_vars;

void ctrl_auto(int set_point, int stop_after)
{
    esplog.all("%s\n", __FUNCTION__);
    // log ctrl mode changes
    if (ctrl_mode != MODE_AUTO)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, mode_change, MODE_AUTO);
    }
    ctrl_mode = MODE_AUTO;
    // log setpoint changes
    if (auto_ctrl_vars.setpoint != set_point)
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, setpoint_change, set_point);
    }
    auto_ctrl_vars.setpoint = set_point;
    auto_ctrl_vars.stop_after = stop_after;
    auto_ctrl_vars.started_on = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    esplog.debug("TEMP CTRL -> AUTO MODE [%d] %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
    save_cfg();
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

static void compute_auto_ctrl_vars(void)
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
        e_t = auto_ctrl_vars.setpoint - current_temp;
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
        // os_printf(">>> PID: Integral begin\n");
        for (idx = 0; idx < 60; idx++)
        {
            cur_val = get_temp(idx);
            if (cur_val == INVALID_TEMP)
            {
                i_e += (auto_ctrl_vars.setpoint - prev_val);
                // DEBUG
                // os_printf("v:%d e:%d, ", cur_val, (auto_ctrl_vars.setpoint - prev_val));
            }
            else
            {
                i_e += (auto_ctrl_vars.setpoint - cur_val);
                // DEBUG
                // os_printf("v:%d e:%d, ", cur_val, (auto_ctrl_vars.setpoint - cur_val));
                prev_val = cur_val;
            }
        }
        // DEBUG
        // os_printf("\n>>> PID: Integral end, i_e = %d\n", i_e);
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
        auto_ctrl_vars.heater_on_period = u_t;
    else
        auto_ctrl_vars.heater_on_period = adv_settings.heater_on_min;
    auto_ctrl_vars.heater_off_period = adv_settings.heater_on_off - auto_ctrl_vars.heater_on_period;
    // DEBUG
    // os_printf(">>> PID:              e(t): %d\n", e_t);
    // os_printf(">>> PID:         d e(t)/dt: %d\n", de_dt);
    // os_printf(">>> PID: I(t-60,t) e(x) dx: %d\n", i_e);
    // os_printf(">>> PID:              u(t): %d\n", (adv_settings.kp * e_t + adv_settings.kd * de_dt + adv_settings.ki * i_e));
    // os_printf(">>> PID:   normalized u(t): %d\n", u_t);
    // os_printf(">>> PID:   heater on timer: %d\n", auto_ctrl_vars.heater_on_period);
    // os_printf(">>> PID:  heater off timer: %d\n", auto_ctrl_vars.heater_off_period);

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
        esplog.debug("%s: warm-up phase\n", __FUNCTION__);
        auto_ctrl_vars.heater_on_period = adv_settings.wup_heater_on;
        auto_ctrl_vars.heater_off_period = adv_settings.wup_heater_off;
    }
}

// CRTL settings management

static char *ctrl_settings_filename = "ctrl_settings.cfg";

static bool restore_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return false;
    }
    File_to_json cfgfile(ctrl_settings_filename);
    if (!cfgfile.exists())
        return false;
    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    pwr_off_timer: int      4 digits
    //  }

    // ctrl_mode
    if (cfgfile.find_string("ctrl_mode"))
    {
        esplog.error("%s - cannot find \"ctrl_mode\"\n", __FUNCTION__);
        return false;
    }
    ctrl_mode = atoi(cfgfile.get_value());
    if (ctrl_mode == MODE_MANUAL)
    {
        // manual_pulse_on
        if (cfgfile.find_string("manual_pulse_on"))
        {
            esplog.error("%s - cannot find \"manual_pulse_on\"\n", __FUNCTION__);
            return false;
        }
        manual_ctrl_vars.heater_on_period = atoi(cfgfile.get_value());
        // manual_pulse_off
        if (cfgfile.find_string("manual_pulse_off"))
        {
            esplog.error("%s - cannot find \"manual_pulse_off\"\n", __FUNCTION__);
            return false;
        }
        manual_ctrl_vars.heater_off_period = atoi(cfgfile.get_value());
        // pwr_off_timer
        if (cfgfile.find_string("pwr_off_timer"))
        {
            esplog.error("%s - cannot find \"pwr_off_timer\"\n", __FUNCTION__);
            return false;
        }
        manual_ctrl_vars.stop_after = atoi(cfgfile.get_value());
    }
    if (ctrl_mode == MODE_AUTO)
    {
        // auto_setpoint
        if (cfgfile.find_string("auto_setpoint"))
        {
            esplog.error("%s - cannot find \"auto_setpoint\"\n", __FUNCTION__);
            return false;
        }
        auto_ctrl_vars.setpoint = atoi(cfgfile.get_value());
        // pwr_off_timer
        if (cfgfile.find_string("pwr_off_timer"))
        {
            esplog.error("%s - cannot find \"pwr_off_timer\"\n", __FUNCTION__);
            return false;
        }
        auto_ctrl_vars.stop_after = atoi(cfgfile.get_value());
    }
    return true;
}

static bool saved_cfg_not_updated(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return true;
    }
    File_to_json cfgfile(ctrl_settings_filename);
    if (!cfgfile.exists())
        return true;
    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    pwr_off_timer: int      4 digits
    //  }
    // ctrl_mode
    if (cfgfile.find_string("ctrl_mode"))
    {
        esplog.error("%s - cannot find \"ctrl_mode\"\n", __FUNCTION__);
        return true;
    }
    if (ctrl_mode != atoi(cfgfile.get_value()))
        return true;
    if (ctrl_mode == MODE_MANUAL)
    {
        // manual_pulse_on
        if (cfgfile.find_string("manual_pulse_on"))
        {
            esplog.error("%s - cannot find \"manual_pulse_on\"\n", __FUNCTION__);
            return true;
        }
        if (manual_ctrl_vars.heater_on_period != atoi(cfgfile.get_value()))
            return true;
        // manual_pulse_off
        if (cfgfile.find_string("manual_pulse_off"))
        {
            esplog.error("%s - cannot find \"manual_pulse_off\"\n", __FUNCTION__);
            return true;
        }
        if (manual_ctrl_vars.heater_off_period != atoi(cfgfile.get_value()))
            return true;
        // pwr_off_timer
        if (cfgfile.find_string("pwr_off_timer"))
        {
            esplog.error("%s - cannot find \"pwr_off_timer\"\n", __FUNCTION__);
            return true;
        }
        if (manual_ctrl_vars.stop_after != atoi(cfgfile.get_value()))
            return true;
    }
    if (ctrl_mode == MODE_AUTO)
    {
        // auto_setpoint
        if (cfgfile.find_string("auto_setpoint"))
        {
            esplog.error("%s - cannot find \"auto_setpoint\"\n", __FUNCTION__);
            return true;
        }
        if (manual_ctrl_vars.heater_on_period != atoi(cfgfile.get_value()))
            return true;
        // pwr_off_timer
        if (cfgfile.find_string("pwr_off_timer"))
        {
            esplog.error("%s - cannot find \"pwr_off_timer\"\n", __FUNCTION__);
            return true;
        }
        if (auto_ctrl_vars.stop_after != atoi(cfgfile.get_value()))
            return true;
    }
    return false;
}

static void remove_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    if (Ffile::exists(&espfs, ctrl_settings_filename))
    {
        Ffile cfgfile(&espfs, ctrl_settings_filename);
        cfgfile.remove();
    }
}

static void save_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (saved_cfg_not_updated())
        remove_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    Ffile cfgfile(&espfs, ctrl_settings_filename);
    if (!cfgfile.is_available())
    {
        esplog.error("%s - cannot open %s\n", __FUNCTION__, ctrl_settings_filename);
        return;
    }
    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    pwr_off_timer: int      4 digits
    //  }

    if (ctrl_mode == MODE_OFF)
    {
        //  {"ctrl_mode": }
        char buffer[(15 + 1 + 1)];
        os_sprintf(buffer,
                   "{\"ctrl_mode\": %d}",
                   ctrl_mode);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    if (ctrl_mode == MODE_MANUAL)
    {
        //  {"ctrl_mode": ,"manual_pulse_on": ,"manual_pulse_off": ,"pwr_off_timer": }
        char buffer[(74 + 1 + 5 + 5 + 5 + 1)];
        os_sprintf(buffer,
                   "{\"ctrl_mode\": %d,\"manual_pulse_on\": %d,\"manual_pulse_off\": %d,\"pwr_off_timer\": %d}",
                   ctrl_mode,
                   manual_ctrl_vars.heater_on_period,
                   manual_ctrl_vars.heater_off_period,
                   manual_ctrl_vars.stop_after);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    if (ctrl_mode == MODE_AUTO)
    {
        //  {"ctrl_mode": ,"auto_setpoint": ,"pwr_off_timer": }
        char buffer[(51 + 1 + 5 + 5 + 1)];
        os_sprintf(buffer,
                   "{\"ctrl_mode\": %d,\"auto_setpoint\": %d,\"pwr_off_timer\": %d}",
                   ctrl_mode,
                   auto_ctrl_vars.setpoint,
                   auto_ctrl_vars.stop_after);
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
}

// ADVANCED CRTL settings management

static char *adv_ctrl_settings_filename = "adv_ctrl_settings.cfg";

static bool restore_adv_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return false;
    }
    File_to_json cfgfile(adv_ctrl_settings_filename);
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
    if (cfgfile.find_string("kp"))
    {
        esplog.error("%s - cannot find \"kp\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.kp = atoi(cfgfile.get_value());
    // kd
    if (cfgfile.find_string("kd"))
    {
        esplog.error("%s - cannot find \"kd\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.kd = atoi(cfgfile.get_value());
    // ki
    if (cfgfile.find_string("ki"))
    {
        esplog.error("%s - cannot find \"ki\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.ki = atoi(cfgfile.get_value());
    // u_max
    if (cfgfile.find_string("u_max"))
    {
        esplog.error("%s - cannot find \"u_max\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.u_max = atoi(cfgfile.get_value());
    // heater_on_min
    if (cfgfile.find_string("heater_on_min"))
    {
        esplog.error("%s - cannot find \"heater_on_min\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.heater_on_min = atoi(cfgfile.get_value());
    // heater_on_max
    if (cfgfile.find_string("heater_on_max"))
    {
        esplog.error("%s - cannot find \"heater_on_max\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.heater_on_max = atoi(cfgfile.get_value());
    // heater_on_off
    if (cfgfile.find_string("heater_on_off"))
    {
        esplog.error("%s - cannot find \"heater_on_off\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.heater_on_off = atoi(cfgfile.get_value());
    // heater_cold
    if (cfgfile.find_string("heater_cold"))
    {
        esplog.error("%s - cannot find \"heater_cold\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.heater_cold = atoi(cfgfile.get_value());
    // warm_up_period
    if (cfgfile.find_string("warm_up_period"))
    {
        esplog.error("%s - cannot find \"warm_up_period\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.warm_up_period = atoi(cfgfile.get_value());
    // wup_heater_on
    if (cfgfile.find_string("wup_heater_on"))
    {
        esplog.error("%s - cannot find \"wup_heater_on\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.wup_heater_on = atoi(cfgfile.get_value());
    // wup_heater_off
    if (cfgfile.find_string("wup_heater_off"))
    {
        esplog.error("%s - cannot find \"wup_heater_off\"\n", __FUNCTION__);
        return false;
    }
    adv_settings.wup_heater_off = atoi(cfgfile.get_value());
    return true;
}

static bool saved_adv_cfg_not_updated(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return true;
    }
    File_to_json cfgfile(adv_ctrl_settings_filename);
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
    if (cfgfile.find_string("kp"))
    {
        esplog.error("%s - cannot find \"kp\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.kp != atoi(cfgfile.get_value()))
        return true;
    // kd
    if (cfgfile.find_string("kd"))
    {
        esplog.error("%s - cannot find \"kd\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.kd != atoi(cfgfile.get_value()))
        return true;
    // ki
    if (cfgfile.find_string("ki"))
    {
        esplog.error("%s - cannot find \"ki\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.ki != atoi(cfgfile.get_value()))
        return true;
    // u_max
    if (cfgfile.find_string("u_max"))
    {
        esplog.error("%s - cannot find \"u_max\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.u_max != atoi(cfgfile.get_value()))
        return true;
    // heater_on_min
    if (cfgfile.find_string("heater_on_min"))
    {
        esplog.error("%s - cannot find \"heater_on_min\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.heater_on_min != atoi(cfgfile.get_value()))
        return true;
    // heater_on_max
    if (cfgfile.find_string("heater_on_max"))
    {
        esplog.error("%s - cannot find \"heater_on_max\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.heater_on_max != atoi(cfgfile.get_value()))
        return true;
    // heater_on_off
    if (cfgfile.find_string("heater_on_off"))
    {
        esplog.error("%s - cannot find \"heater_on_off\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.heater_on_off != atoi(cfgfile.get_value()))
        return true;
    // heater_cold
    if (cfgfile.find_string("heater_cold"))
    {
        esplog.error("%s - cannot find \"heater_cold\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.heater_cold != atoi(cfgfile.get_value()))
        return true;
    // warm_up_period
    if (cfgfile.find_string("warm_up_period"))
    {
        esplog.error("%s - cannot find \"warm_up_period\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.warm_up_period != atoi(cfgfile.get_value()))
        return true;
    // wup_heater_on
    if (cfgfile.find_string("wup_heater_on"))
    {
        esplog.error("%s - cannot find \"wup_heater_on\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.wup_heater_on != atoi(cfgfile.get_value()))
        return true;
    // wup_heater_off
    if (cfgfile.find_string("wup_heater_off"))
    {
        esplog.error("%s - cannot find \"wup_heater_off\"\n", __FUNCTION__);
        return true;
    }
    if (adv_settings.wup_heater_off != atoi(cfgfile.get_value()))
        return true;
    return false;
}

static void remove_adv_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    if (Ffile::exists(&espfs, adv_ctrl_settings_filename))
    {
        Ffile cfgfile(&espfs, adv_ctrl_settings_filename);
        cfgfile.remove();
    }
}

static void save_adv_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (saved_adv_cfg_not_updated())
        remove_adv_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    Ffile cfgfile(&espfs, adv_ctrl_settings_filename);
    if (!cfgfile.is_available())
    {
        esplog.error("%s - cannot open %s\n", __FUNCTION__, adv_ctrl_settings_filename);
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
    os_sprintf(buffer,
               "{\"kp\": %d,"
               "\"kd\": %d,"
               "\"ki\": %d,"
               "\"u_max\": %d,"
               "\"heater_on_min\": %d,"
               "\"heater_on_max\": %d,"
               "\"heater_on_off\": %d,"
               "\"heater_cold\": %d,"
               "\"warm_up_period\": %d,"
               "\"wup_heater_on\": %d,"
               "\"wup_heater_off\": %d}",
               adv_settings.kp,
               adv_settings.kd,
               adv_settings.ki,
               adv_settings.u_max,
               adv_settings.heater_on_min,
               adv_settings.heater_on_max,
               adv_settings.heater_on_off,
               adv_settings.heater_cold,
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
    esplog.all("%s\n", __FUNCTION__);
    // heater
    heater_vars.last_heater_on = 0;
    heater_vars.last_heater_off = 0;

    if (restore_cfg())
    {
        switch (ctrl_mode)
        {
        case MODE_OFF:
            ctrl_off();
            break;
        case MODE_MANUAL:
            ctrl_manual(manual_ctrl_vars.heater_on_period, manual_ctrl_vars.heater_off_period, manual_ctrl_vars.stop_after);
            break;
        case MODE_AUTO:
            ctrl_auto(auto_ctrl_vars.setpoint, auto_ctrl_vars.stop_after);
            break;
        default:
            break;
        }
    }
    else
    {
        // CTRL DEFAULT
        ctrl_off();
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
    }
}

void temp_control_manual(void)
{
    esplog.all("%s\n", __FUNCTION__);
    struct date *current_time = get_current_time();

    // switch off timer management (if enabled)
    if (manual_ctrl_vars.stop_after > 0)
    {
        uint32 running_since = current_time->timestamp - manual_ctrl_vars.started_on;
        if (running_since >= (manual_ctrl_vars.stop_after * 60))
        {
            ctrl_off();
            return;
        }
    }
    // running continuosly
    if (manual_ctrl_vars.heater_on_period == 0)
    {
        if (!is_heater_on())
            switch_on_heater();
        return;
    }
    // heater on duty cycle
    if (is_heater_on())
    {
        if ((current_time->timestamp - heater_vars.last_heater_on) >= (manual_ctrl_vars.heater_on_period * 60))
            switch_off_heater();
    }
    else
    {
        if ((current_time->timestamp - heater_vars.last_heater_off) >= (manual_ctrl_vars.heater_off_period * 60))
            switch_on_heater();
    }
}

void temp_control_auto(void)
{
    esplog.all("%s\n", __FUNCTION__);
    struct date *current_time = get_current_time();
    // switch off timer management (if enabled)
    if (auto_ctrl_vars.stop_after > 0)
    {
        uint32 running_since = current_time->timestamp - auto_ctrl_vars.started_on;
        if (running_since >= (auto_ctrl_vars.stop_after * 60))
        {
            ctrl_off();
            return;
        }
    }
    // update ctrl vars
    compute_auto_ctrl_vars();
    // check if it's time to switch the heater off
    if (is_heater_on())
    {
        // temperature reached the setpoint
        if (get_temp(0) >= auto_ctrl_vars.setpoint)
        {
            switch_off_heater();
            return;
        }
        // the heater on period has completed
        uint32 heater_on_since = current_time->timestamp - heater_vars.last_heater_on;
        if (heater_on_since >= (auto_ctrl_vars.heater_on_period * 60))
        {
            switch_off_heater();
            return;
        }
    }
    else
    {
        // here the heater is off

        // temperature reached the setpoint
        if (get_temp(0) >= auto_ctrl_vars.setpoint)
        {
            // do nothing
            return;
        }
        // when the heater off period exhausted then turn on the heater
        uint32 heater_off_since = current_time->timestamp - heater_vars.last_heater_off;
        if (heater_off_since >= (auto_ctrl_vars.heater_off_period * 60))
        {
            switch_on_heater();
            return;
        }
    }
}

void temp_control_run(void)
{
    esplog.all("%s\n", __FUNCTION__);
    esplog.debug("CTRL STATUS ------------------------------------------\n");
    esplog.debug("   CTRL MODE -> %d\n", ctrl_mode);
    switch (ctrl_mode)
    {
    case MODE_OFF:
        if (is_heater_on())
            switch_off_heater();
        break;
    case MODE_MANUAL:
        temp_control_manual();
        esplog.debug("  started on -> %d %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
        break;
    case MODE_AUTO:
        temp_control_auto();
        esplog.debug("  started on -> %d %s\n", auto_ctrl_vars.started_on, esp_sntp.get_timestr(auto_ctrl_vars.started_on));
        break;
    default:
        break;
    }
    esplog.debug("      HEATER -> %d\n", is_heater_on());
    esplog.debug(" switched on -> %d %s\n", heater_vars.last_heater_on, esp_sntp.get_timestr(heater_vars.last_heater_on));
    esplog.debug("switched off -> %d %s\n", heater_vars.last_heater_off, esp_sntp.get_timestr(heater_vars.last_heater_off));
}

int get_current_mode(void)
{
    return ctrl_mode;
}

int get_pwr_off_timer(void)
{
    if (ctrl_mode == MODE_MANUAL)
        return manual_ctrl_vars.stop_after;
    if (ctrl_mode == MODE_AUTO)
        return auto_ctrl_vars.stop_after;
}

uint32 get_pwr_off_timer_started_on(void)
{
    if (ctrl_mode == MODE_MANUAL)
        return manual_ctrl_vars.started_on;
    if (ctrl_mode == MODE_AUTO)
        return auto_ctrl_vars.started_on;
}

int get_auto_setpoint(void)
{
    return auto_ctrl_vars.setpoint;
}

int get_manual_pulse_on(void)
{
    return manual_ctrl_vars.heater_on_period;
}

int get_manual_pulse_off(void)
{
    return manual_ctrl_vars.heater_off_period;
}