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

#include "espbot_global.hpp"
#include "app.hpp"
#include "app_cron.hpp"
#include "app_heater.hpp"
#include "app_temp_log.hpp"
#include "app_temp_control.hpp"

//
// CTRL
//
static int ctrl_mode; // off/manual/auto

//
// heater mngmt
//

static uint32 last_heater_on;
static uint32 last_heater_off;

void switch_on_heater(void)
{
    esplog.all("%s\n", __FUNCTION__);
    last_heater_on = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    heater_start();
    esplog.debug("TEMP CTRL -> heater on\n");
}

void switch_off_heater(void)
{
    esplog.all("%s\n", __FUNCTION__);
    last_heater_off = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
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
    ctrl_mode = MODE_OFF;
    if (is_heater_on())
        switch_off_heater();
    uint32 time = esp_sntp.get_timestamp();
    esplog.debug("TEMP CTRL -> OFF [%d] %s\n", time, esp_sntp.get_timestr(time));
}

void ctrl_manual(int heater_on_period, int heater_off_period, int stop_after)
{
    esplog.all("%s\n", __FUNCTION__);
    ctrl_mode = MODE_MANUAL;
    manual_ctrl_vars.heater_on_period = heater_on_period;
    manual_ctrl_vars.heater_off_period = heater_off_period;
    manual_ctrl_vars.stop_after = stop_after;
    manual_ctrl_vars.started_on = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    switch_on_heater();
    esplog.debug("TEMP CTRL -> MANUAL MODE [%d] %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
}

static struct _auto_ctrl_vars
{
    int setpoint;
    uint32 started_on;     // the timestamp when auto was started
    int stop_after;        // power off timer in minutes
} auto_ctrl_vars;

void ctrl_auto(int set_point, int stop_after)
{
    esplog.all("%s\n", __FUNCTION__);
    ctrl_mode = MODE_AUTO;
    auto_ctrl_vars.setpoint = set_point;
    auto_ctrl_vars.stop_after = stop_after;
    auto_ctrl_vars.started_on = (esp_sntp.get_timestamp() / 60) * 60; // rounding to previous minute
    esplog.debug("TEMP CTRL -> AUTO MODE [%d] %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
}

void temp_control_init(void)
{
    esplog.all("%s\n", __FUNCTION__);
    // heater
    last_heater_on = 0;
    last_heater_off = 0;

    // CTRL
    ctrl_manual(0, 0, 0);
    // COMPLETE_ME: load ctrl_mode from flash
}

void temp_control_manual(struct date *time)
{
    esplog.all("%s\n", __FUNCTION__);
    // switch off timer management (if enabled)
    if (manual_ctrl_vars.stop_after > 0)
    {
        uint32 running_since = time->timestamp - manual_ctrl_vars.started_on;
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
        if ((time->timestamp - last_heater_on) >= (manual_ctrl_vars.heater_on_period * 60))
            switch_off_heater();
    }
    else
    {
        if ((time->timestamp - last_heater_off) >= (manual_ctrl_vars.heater_off_period * 60))
            switch_on_heater();
    }
}

void temp_control_auto(struct date *time)
{
    esplog.all("%s\n", __FUNCTION__);
    // switch off timer management (if enabled)
    if (auto_ctrl_vars.stop_after > 0)
    {
        uint32 running_since = time->timestamp - auto_ctrl_vars.started_on;
        if (running_since >= (auto_ctrl_vars.stop_after * 60))
        {
            ctrl_off();
            return;
        }
    }
}

void temp_control_run(struct date *time)
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
        temp_control_manual(time);
        esplog.debug("  started on -> %d %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
        break;
    case MODE_AUTO:
        temp_control_auto(time);
        esplog.debug("  started on -> %d %s\n", auto_ctrl_vars.started_on, esp_sntp.get_timestr(auto_ctrl_vars.started_on));
        break;
    default:
        break;
    }
    esplog.debug("      HEATER -> %d\n", is_heater_on());
    esplog.debug(" switched on -> %d %s\n", last_heater_on, esp_sntp.get_timestr(last_heater_on));
    esplog.debug("switched off -> %d %s\n", last_heater_off, esp_sntp.get_timestr(last_heater_off));
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