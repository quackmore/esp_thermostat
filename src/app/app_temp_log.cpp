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

#include "espbot.hpp"
#include "espbot_cfgfile.hpp"
#include "espbot_cron.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_mem_mon.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_temp_log.hpp"
#include "app_temp_control.hpp"
#include "app_temp_log.hpp"
#include "app_remote_log.hpp"

static int temperature_log[TEMP_LOG_LENGTH];
static int humidity_log[2];
static char current_idx;
static Dht *dht22;
static struct
{
    int temp_cal_offset;
    int humi_cal_offset;
} cal_cfg;

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
        temperature_log[new_idx] = (int)(event.temperature * 10) + cal_cfg.temp_cal_offset;
    DEBUG("dht_read_completed temperature (*10): %d", temperature_log[new_idx]);
    current_idx++;
    if (current_idx >= TEMP_LOG_LENGTH)
        current_idx = 0;

    // log temperature changes
    if (get_temp(0) != get_temp(1))
    {
        struct date *current_time = cron_get_current_time();
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
        int humi_reading = (int)(event.relative_humidity * 10) + cal_cfg.humi_cal_offset;
        DEBUG("dht_read_completed humidity    (*10): %d", humi_reading);
        int int_humi = (humi_reading) / 10;
        int dec_humi = (humi_reading) % 10;
        if (dec_humi >= 5)
            humidity_log[0] = (int_humi * 10) + 10;
        else
            humidity_log[0] = int_humi * 10;
    }
    // log humidity changes
    if (get_humi(0) != get_humi(1))
    {
        struct date *current_time = cron_get_current_time();
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
    next_function(temp_control_run);
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

// PERSISTENCY

#define TEMP_LOG_FILENAME ((char *)f_str("temp_log.cfg"))
// {"temp_cal_offset":,"humi_cal_offset":}"

static int temp_log_restore_cfg(void)
{
    ALL("temp_log_restore_cfg");
    if (!Espfile::exists(TEMP_LOG_FILENAME))
        return CFG_cantRestore;
    Cfgfile cfgfile(TEMP_LOG_FILENAME);
    int temp_cal_offset = cfgfile.getInt(f_str("temp_cal_offset"));
    int humi_cal_offset = cfgfile.getInt(f_str("humi_cal_offset"));
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(TEMPLOG_RESTORE_CFG_ERROR);
        ERROR("temp_log_restore_cfg error");
        return CFG_error;
    }
    cal_cfg.temp_cal_offset = temp_cal_offset;
    cal_cfg.humi_cal_offset = humi_cal_offset;
    mem_mon_stack();
    return CFG_ok;
}

static int temp_log_saved_cfg_updated(void)
{
    ALL("temp_log_saved_cfg_updated");
    if (!Espfile::exists(TEMP_LOG_FILENAME))
    {
        return CFG_notUpdated;
    }
    Cfgfile cfgfile(TEMP_LOG_FILENAME);
    int temp_cal_offset = cfgfile.getInt(f_str("temp_cal_offset"));
    int humi_cal_offset = cfgfile.getInt(f_str("humi_cal_offset"));
    mem_mon_stack();
    if (cfgfile.getErr() != JSON_noerr)
    {
        // no need to raise an error, the cfg file will be overwritten
        // dia_error_evnt(TEMP_LOG_SAVED_CFG_UPDATED_ERROR);
        // ERROR("temp_log_saved_cfg_updated error");
        return CFG_error;
    }
    if ((cal_cfg.temp_cal_offset != temp_cal_offset) ||
        (cal_cfg.humi_cal_offset != humi_cal_offset))
    {
        return CFG_notUpdated;
    }
    return CFG_ok;
}

char *temp_log_cfg_json_stringify(char *dest, int len)
{
// {"temp_cal_offset":,"humi_cal_offset":}"
    int msg_len = 40 + 6 + 6 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(TEMPLOG_CFG_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("temp_log_cfg_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"temp_cal_offset\":%d,\"humi_cal_offset\":%d}",
               cal_cfg.temp_cal_offset,
               cal_cfg.humi_cal_offset);
    mem_mon_stack();
    return msg;
}

int temp_log_cfg_save(void)
{
    ALL("temp_log_cfg_save");
    if (temp_log_saved_cfg_updated() == CFG_ok)
        return CFG_ok;
    Cfgfile cfgfile(TEMP_LOG_FILENAME);
    if (cfgfile.clear() != SPIFFS_OK)
        return CFG_error;
    char str[53];
    temp_log_cfg_json_stringify(str, 53);
    int res = cfgfile.n_append(str, os_strlen(str));
    if (res < SPIFFS_OK)
        return CFG_error;
    mem_mon_stack();
    return CFG_ok;
}

int set_cal_offset(int temp_offset, int humi_offset)
{
    cal_cfg.temp_cal_offset = temp_offset;
    cal_cfg.humi_cal_offset = humi_offset;
    return temp_log_cfg_save();
}

void temp_log_init(void)
{
    ALL("temp_log_init");
    // calibration offsets
    if (temp_log_restore_cfg() == CFG_ok)
    {
        dia_info_evnt(TEMPLOG_CUSTOM_CAL_CFG);
        INFO("temp_log_init custom calibration offsets found");
    }
    else
    {
        cal_cfg.temp_cal_offset = 0;
        cal_cfg.humi_cal_offset = 0;
        dia_info_evnt(TEMPLOG_DEFAULT_CAL_CFG);
        INFO("temp_log_init custom calibration offsets found");
    }

    // reading buffers
    for (current_idx = 0; current_idx < TEMP_LOG_LENGTH; current_idx++)
        temperature_log[current_idx] = INVALID_TEMP;
    humidity_log[0] = INVALID_HUMI;
    humidity_log[1] = INVALID_HUMI;
    current_idx = 0;
    // sensor class
    dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
    if (dht22 == NULL)
    {
        dia_error_evnt(TEMPLOG_INIT_HEAP_EXHAUSTED);
        ERROR("temp_log_init heap exhausted %d", sizeof(Dht));
    }
}