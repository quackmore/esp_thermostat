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
#include "app_temp_log.hpp"

static int temperature_log[TEMP_LOG_LENGTH];
static char current_idx;
static Dht *dht22;

void temp_log_init(void)
{
    esplog.all("%s\n", __FUNCTION__);
    for (current_idx = 0; current_idx < TEMP_LOG_LENGTH; current_idx++)
        temperature_log[current_idx] = INVALID_TEMP;
    current_idx = 0;
    dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
    if (dht22 == NULL)
        esplog.error("%s: not enough heap for allocating a Dht class\n", __FUNCTION__);
}

static void temp_log_read_completed(void *param)
{
    esplog.all("%s\n", __FUNCTION__);
    sensors_event_t event;
    os_memset(&event, 0, sizeof(sensors_event_t));
    dht22->temperature.getEvent(&event);

    int new_idx = current_idx + 1;
    if (new_idx >= TEMP_LOG_LENGTH)
        new_idx = 0;
    if (event.invalid)
        temperature_log[new_idx] = INVALID_TEMP;
    else
        temperature_log[new_idx] = (int)(event.temperature * 10);
    esplog.trace("temperature (*10): %d\n", temperature_log[new_idx]);
    current_idx++;
    if (current_idx >= TEMP_LOG_LENGTH)
        current_idx = 0;
}

void temp_log_read(void *param)
{
    esplog.all("%s\n", __FUNCTION__);
    esplog.trace("... reading temperature ...\n");
    if (dht22)
        dht22->temperature.force_reading(temp_log_read_completed, NULL);
}

int get_temp(int idx)
{
    esplog.all("%s\n", __FUNCTION__);
    if (idx >= TEMP_LOG_LENGTH)
        idx = 0;
    int reading_idx = current_idx - idx;
    if (reading_idx < 0)
        reading_idx = TEMP_LOG_LENGTH - 1;
    return temperature_log[reading_idx];
}
