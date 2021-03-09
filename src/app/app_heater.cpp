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

#include "app.hpp"
#include "app_heater.hpp"
#include "app_remote_log.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_gpio.hpp"
#include "espbot_timedate.hpp"

#define HEATER_PIN ESPBOT_D2
#define HEATER_ON ESPBOT_HIGH
#define HEATER_OFF ESPBOT_LOW

static bool heater_on;

void heater_init(void)
{
    ALL("heater_init");
    gpio_config(HEATER_PIN, ESPBOT_GPIO_OUTPUT);
    gpio_set(HEATER_PIN, HEATER_OFF);
    heater_on = false;
}

void heater_switch_on(void)
{
    gpio_set(HEATER_PIN, HEATER_ON);
}

void heater_start(void)
{
    ALL("heater_start");
    heater_switch_on();
    if (heater_on != true)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), heater_change, true);
    heater_on = true;
}

void heater_switch_off(void)
{
    gpio_set(HEATER_PIN, HEATER_OFF);
}

void heater_stop(void)
{
    ALL("heater_stop");
    // log heater status change
    heater_switch_off();
    if (heater_on != false)
        // can be manually set, use actual time
        log_event(timedate_get_timestamp(), heater_change, false);
    heater_on = false;
}

bool is_heater_on(void)
{
    return heater_on;
}