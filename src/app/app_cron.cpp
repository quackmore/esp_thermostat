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

#include "app.hpp"
#include "espbot_global.hpp"

#define HEATER_PIN ESPBOT_D4

static os_timer_t heater_period_timer;

//    os_timer_disarm(&heater_period_timer);
//    os_timer_setfn(&heater_period_timer, (os_timer_func_t *)heater_stop, NULL);
//    os_timer_arm(&heater_period_timer, period, 0);
//

struct job
{
    char minutes;
    char hours;
    char day_of_month;
    char month;
    char day_of_week;
    void (*command)(void *);
    void *param;
};

static os_timer_t cron_timer;
static int cron_period;

int cron_add_job(char min, char hour, char day_of_month, char month, char day_of_week, void (*command)(void *), void *param)
{
    esplog.all("%s\n", __FUNCTION__);
}

void cron_execute(void)
{
    esplog.all("%s\n", __FUNCTION__);
    cron_sync();
    // execute ...
    uint32 timestamp = esp_sntp.get_timestamp();
    esplog.trace("%s date: %s [%d]\n", __FUNCTION__, esp_sntp.get_timestr(timestamp), timestamp);

}

void cron_init(void)
{
    esplog.all("%s\n", __FUNCTION__);
    os_timer_disarm(&cron_timer);
    os_timer_setfn(&cron_timer, (os_timer_func_t *)cron_execute, NULL);
}

void cron_sync(void)
{
    esplog.all("%s\n", __FUNCTION__);
    uint32 timestamp = esp_sntp.get_timestamp();
    timestamp &= 0x000000ff;
    esplog.trace("%s: timestamp = %d\n", __FUNCTION__);

    if (timestamp < 30)
        // just await (60 - timestamp) seconds
        cron_period = 60000 - timestamp * 1000;
    else
        // too late, skip a minute
        cron_period = 120000 - timestamp * 1000;
    os_timer_arm(&cron_timer, cron_period, 0);    
}
