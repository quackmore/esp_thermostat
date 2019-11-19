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
#include "espbot_gpio.hpp"
#include "app_heater.hpp"

#define HEATER_PIN ESPBOT_D5

static os_timer_t heater_period_timer;
static bool heater_on;

void heater_init(void)
{
    esplog.all("%s\n", __FUNCTION__);
    esp_gpio.config(HEATER_PIN, ESPBOT_GPIO_OUTPUT);
    esp_gpio.set(HEATER_PIN, ESPBOT_HIGH);
    os_timer_disarm(&heater_period_timer);
    os_timer_setfn(&heater_period_timer, (os_timer_func_t *)heater_stop, NULL);
    heater_on = false;
}

void heater_start(void)
{
    esplog.all("%s\n", __FUNCTION__);
    esp_gpio.set(HEATER_PIN, ESPBOT_LOW);
    heater_on = true;
}

// void heater_start(int period)
// {
//     esplog.all("%s\n", __FUNCTION__);
//     os_timer_arm(&heater_period_timer, period, 0);
//     esp_gpio.set(HEATER_PIN, ESPBOT_LOW);
//     heater_on = true;
// }

void heater_stop(void)
{
    esplog.all("%s\n", __FUNCTION__);
    esp_gpio.set(HEATER_PIN, ESPBOT_HIGH);
    heater_on = false;
}

bool is_heater_on(void)
{
    return heater_on;
}