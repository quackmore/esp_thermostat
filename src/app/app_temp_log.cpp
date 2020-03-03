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

#include "espbot_cron.hpp"
#include "espbot_global.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_remote_log.hpp"
#include "app_temp_control.hpp"
#include "app_temp_log.hpp"

static int temperature_log[TEMP_LOG_LENGTH];
static char current_idx;
static Dht *dht22;

void temp_log_init(void)
{
    ALL("temp_log_init");
    for (current_idx = 0; current_idx < TEMP_LOG_LENGTH; current_idx++)
        temperature_log[current_idx] = INVALID_TEMP;
    current_idx = 0;
    dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
    if (dht22 == NULL)
    {
        esp_diag.error(TEMP_LOG_INIT_HEAP_EXHAUSTED);
        ERROR("temp_log_init heap exhausted %d", sizeof(Dht));
    }
}

// reading sensor results and logging locally and remotely
static void temp_read_completed(void *param)
{
    ALL("temp_read_completed");
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
    DEBUG("temp_read_completed temperature (*10): %d", temperature_log[new_idx]);
    current_idx++;
    if (current_idx >= TEMP_LOG_LENGTH)
        current_idx = 0;
    // log temperature changes
    if (get_temp(0) != get_temp(1))
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, temp_change, get_temp(0));
    }
}

void init_temperature_readings(void)
{
    ALL("temp_log_read");
    if (dht22)
        dht22->temperature.force_reading(temp_read_completed, NULL);
}

// reading sensor results and scheduling temp_control_run
static void temp_log_read_completed(void *param)
{
    ALL("temp_log_read_completed");
    temp_read_completed(NULL);
    subsequent_function(temp_control_run);
}

// periodic temperature reading
void temp_log_read(void)
{
    ALL("temp_log_read");
    if (dht22)
        dht22->temperature.force_reading(temp_log_read_completed, NULL);
}

// getting temperature readings from local log
int get_temp(int idx)
{
    ALL("get_temp");
    if (idx >= TEMP_LOG_LENGTH)
        idx = 0;
    int reading_idx = current_idx - idx;
    if (reading_idx < 0)
        reading_idx = TEMP_LOG_LENGTH - 1;
    return temperature_log[reading_idx];
}
