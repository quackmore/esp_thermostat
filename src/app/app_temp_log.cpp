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
static int humidity_log[2];
static char current_idx;
static Dht *dht22;

void temp_log_init(void)
{
    ALL("temp_log_init");
    for (current_idx = 0; current_idx < TEMP_LOG_LENGTH; current_idx++)
        temperature_log[current_idx] = INVALID_TEMP;
    humidity_log[0] = INVALID_HUMI;
    humidity_log[1] = INVALID_HUMI;
    current_idx = 0;
    dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
    if (dht22 == NULL)
    {
        esp_diag.error(TEMP_LOG_INIT_HEAP_EXHAUSTED);
        ERROR("temp_log_init heap exhausted %d", sizeof(Dht));
    }
}

// reading sensor results and logging locally and remotely
static void dht_read_completed(void *param)
{
    ALL("dht_read_completed");
    sensors_event_t event;
    // temperature reading
    os_memset(&event, 0, sizeof(sensors_event_t));
    dht22->temperature.getEvent(&event);

    int new_idx = current_idx + 1;
    if (new_idx >= TEMP_LOG_LENGTH)
        new_idx = 0;
    if (event.invalid)
        // keep the last temperature reading
        temperature_log[new_idx] = get_temp(0);
    else
        temperature_log[new_idx] = (int)(event.temperature * 10);
    DEBUG("dht_read_completed temperature (*10): %d", temperature_log[new_idx]);
    current_idx++;
    if (current_idx >= TEMP_LOG_LENGTH)
        current_idx = 0;

    // log temperature changes
    if (get_temp(0) != get_temp(1))
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, temp_change, get_temp(0));
    }

    // humidity reading
    os_memset(&event, 0, sizeof(sensors_event_t));
    dht22->humidity.getEvent(&event);
    if (event.invalid)
    {
        // keep the last humidity reading
        humidity_log[0] = humidity_log[1];
    }
    else
    {
        humidity_log[1] = humidity_log[0];
        // rounding humidity reading
        // 67.5 -> 68.0
        // 68.4 -> 68.0
        DEBUG("dht_read_completed humidity    (*10): %d", (int)(event.temperature * 10));
        int int_humi = ((int)(event.temperature * 10)) / 10;
        int dec_humi = ((int)(event.temperature * 10)) % 10;
        if (dec_humi >= 5)
            humidity_log[0] = (int_humi * 10) + 10;
        else
            humidity_log[0] = int_humi * 10;
    }
    // log humidity changes
    if (get_humi(0) != get_humi(1))
    {
        struct date *current_time = get_current_time();
        log_event(current_time->timestamp, humi_change, get_humi(0));
    }
}

void init_temperature_readings(void)
{
    ALL("temp_log_read");
    if (dht22)
        dht22->temperature.force_reading(dht_read_completed, NULL);
}

// reading sensor results and scheduling temp_control_run
static void temp_log_read_completed(void *param)
{
    ALL("temp_log_read_completed");
    dht_read_completed(NULL);
    subsequent_function(temp_control_run);
}

// periodic temperature reading
void temp_log_read(void *)
{
    ALL("temp_log_read");
    if (dht22)
        dht22->temperature.force_reading(temp_log_read_completed, NULL);
}

// getting temperature readings from local log
int get_temp(int idx)
{
    ALL("get_temp");
    if ((idx < 0) || (idx >= TEMP_LOG_LENGTH))
        idx = 0;
    int reading_idx = current_idx - idx;
    if (reading_idx < 0)
        reading_idx = TEMP_LOG_LENGTH - 1;
    return temperature_log[reading_idx];
}

// getting humidity readings from local log
int get_humi(int idx)
{
    ALL("get_humi");
    if ((idx < 0) || (idx >= 2))
        idx = 0;
    return humidity_log[idx];
}