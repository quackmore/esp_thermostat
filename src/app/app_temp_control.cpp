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
#include "app_heater.hpp"
#include "app_temp_log.hpp"
#include "app_temp_control.hpp"


// static os_timer_t heater_period_timer;

void temp_control_init(void)
{
    esplog.all("%s\n", __FUNCTION__);
    // os_timer_disarm(&heater_period_timer);
    // os_timer_setfn(&heater_period_timer, (os_timer_func_t *)heater_stop, NULL);
}

void temp_control_run(void *param)
{
    esplog.all("%s\n", __FUNCTION__);
    esplog.trace("... running temperature control\n");
    int idx;
    esplog.trace("Temperature LOG\n");
    for(idx = 0; idx<TEMP_LOG_LENGTH; idx++)
        os_printf("%d:%d ", idx, get_temp(idx));
    os_printf("\n");
}