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
    last_heater_on = esp_sntp.get_timestamp();
    heater_start();
    esplog.debug("TEMP CTRL -> heater on\n");
}

void switch_off_heater(void)
{
    last_heater_off = esp_sntp.get_timestamp();
    heater_stop();
    esplog.debug("TEMP CTRL -> heater off\n");
}

//
// CTRL mngmt
//
static struct _manual_ctrl_vars
{
    int heater_on_period;  // minutes, 0 minutes -> continuos
    int heater_off_period; // minutes
    uint32 started_on;     // last timestamp when heater was started
    int stop_after;        // minutes
} manual_ctrl_vars;

void ctrl_off(void)
{
    ctrl_mode = MODE_OFF;
    if (is_heater_on())
        switch_off_heater();
    uint32 time = esp_sntp.get_timestamp();
    esplog.debug("TEMP CTRL -> OFF [%d] %s\n", time, esp_sntp.get_timestr(time));
}

void ctrl_manual(int heater_on_period, int heater_off_period, int stop_after)
{
    ctrl_mode = MODE_MANUAL;
    manual_ctrl_vars.heater_on_period = heater_on_period;
    manual_ctrl_vars.heater_off_period = heater_off_period;
    manual_ctrl_vars.stop_after = stop_after;
    manual_ctrl_vars.started_on = esp_sntp.get_timestamp();
    switch_on_heater();
    esplog.debug("TEMP CTRL -> MANUAL MODE [%d] %s\n", manual_ctrl_vars.started_on, esp_sntp.get_timestr(manual_ctrl_vars.started_on));
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
    esplog.debug("... running auto temperature control\n");
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
        break;
    default:
        break;
    }
    esplog.debug("      HEATER -> %d\n", is_heater_on());
    esplog.debug(" switched on -> %d %s\n", last_heater_on, esp_sntp.get_timestr(last_heater_on));
    esplog.debug("switched off -> %d %s\n", last_heater_off, esp_sntp.get_timestr(last_heater_off));
}