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
#include "app_heater.hpp"
#include "app_remote_log.hpp"
#include "espbot_cron.hpp"
#include "espbot_global.hpp"
#include "espbot_gpio.hpp"

#define HEATER_PIN ESPBOT_D2
#define HEATER_ON ESPBOT_HIGH
#define HEATER_OFF ESPBOT_LOW

// static os_timer_t heater_delay_timer;
static bool heater_on;

void heater_init(void)
{
    ALL("heater_init");
    esp_gpio.config(HEATER_PIN, ESPBOT_GPIO_OUTPUT);
    esp_gpio.set(HEATER_PIN, HEATER_OFF);
    // os_timer_disarm(&heater_delay_timer);
    heater_on = false;
}

void heater_switch_on(void)
{
    esp_gpio.set(HEATER_PIN, HEATER_ON);
}

void heater_start(void)
{
    ALL("heater_start");
    // os_timer_setfn(&heater_delay_timer, (os_timer_func_t *)heater_switch_on, NULL);
    // os_timer_arm(&heater_delay_timer, HEATER_RELAY_DELAY, 0);
    // log heater status change
    heater_switch_on();
    if (heater_on != true)
        // can be manually set, use actual time
        log_event(esp_time.get_timestamp(), heater_change, true);
    heater_on = true;
}

void heater_switch_off(void)
{
    esp_gpio.set(HEATER_PIN, HEATER_OFF);
}

void heater_stop(void)
{
    ALL("heater_stop");
    // os_timer_setfn(&heater_delay_timer, (os_timer_func_t *)heater_switch_off, NULL);
    // os_timer_arm(&heater_delay_timer, HEATER_RELAY_DELAY, 0);
    // log heater status change
    heater_switch_off();
    if (heater_on != false)
        // can be manually set, use actual time
        log_event(esp_time.get_timestamp(), heater_change, false);
    heater_on = false;
}

bool is_heater_on(void)
{
    return heater_on;
}