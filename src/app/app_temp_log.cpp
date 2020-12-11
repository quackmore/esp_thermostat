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

#include "espbot_config.hpp"
#include "espbot_cron.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_global.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_remote_log.hpp"
#include "app_temp_control.hpp"
#include "app_temp_log.hpp"

static int temperature_log[TEMP_LOG_LENGTH];
static int humidity_log[2];
static char current_idx;
static Dht *dht22;
static int temp_cal_offset;
static int humi_cal_offset;

static bool restore_cfg(void);

void temp_log_init(void)
{
    ALL("temp_log_init");
    // calibration offsets
    if (restore_cfg())
    {
        esp_diag.info(TEMPLOG_CUSTOM_CAL_CFG);
        INFO("temp_log_init custom calibration offsets found");
    }
    else
    {
        temp_cal_offset = 0;
        humi_cal_offset = 0;
        esp_diag.info(TEMPLOG_DEFAULT_CAL_CFG);
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
        temperature_log[new_idx] = get_temp(current_idx);
    else
        temperature_log[new_idx] = (int)(event.temperature * 10) + temp_cal_offset;
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
        int humi_reading = (int)(event.relative_humidity * 10) + humi_cal_offset;
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

// PERSISTENCY

#define READING_CAL_FILENAME f_str("temp_log.cfg")

static bool restore_cfg(void)
{
    ALL("temp_log_restore_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMPLOG_RESTORE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_log_restore_cfg FS not available");
        return false;
    }
    File_to_json cfgfile(READING_CAL_FILENAME);
    if (cfgfile.exists())
    {
        // "{"temp_cal_offset":,"humi_cal_offset":}",
        // temp_cal_offset
        if (cfgfile.find_string(f_str("temp_cal_offset")))
        {
            esp_diag.error(TEMPLOG_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_log_restore_cfg cannot find \"temp_cal_offset\"");
            return false;
        }
        // humi_cal_offset
        temp_cal_offset = atoi(cfgfile.get_value());
        if (cfgfile.find_string(f_str("humi_cal_offset")))
        {
            esp_diag.error(TEMPLOG_RESTORE_CFG_INCOMPLETE);
            ERROR("temp_log_restore_cfg cannot find \"humi_cal_offset\"");
            return false;
        }
        humi_cal_offset = atoi(cfgfile.get_value());
        return true;
    }
    return false;
}

static bool saved_cfg_not_updated(void)
{
    ALL("temp_log_saved_cfg_not_updated");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMPLOG_SAVED_CFG_NOT_UPDATED_FS_NOT_AVAILABLE);
        ERROR("temp_log_saved_cfg_not_updated FS not available");
        return true;
    }
    File_to_json cfgfile(READING_CAL_FILENAME);
    if (cfgfile.exists())
    {
        // "{"temp_cal_offset":,"humi_cal_offset":}",
        // temp_cal_offset
        if (cfgfile.find_string(f_str("temp_cal_offset")))
        {
            esp_diag.error(TEMPLOG_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_log_saved_cfg_not_updated cannot find \"temp_cal_offset\"");
            return true;
        }
        if (temp_cal_offset != atoi(cfgfile.get_value()))
            return true;
        // humi_cal_offset
        if (cfgfile.find_string(f_str("humi_cal_offset")))
        {
            esp_diag.error(TEMPLOG_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("temp_log_saved_cfg_not_updated cannot find \"humi_cal_offset\"");
            return true;
        }
        if (humi_cal_offset != atoi(cfgfile.get_value()))
            return true;
        return false;
    }
    espmem.stack_mon();
    return true;
}

static void remove_cfg(void)
{
    ALL("temp_log_remove_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMPLOG_REMOVE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_log_remove_cfg FS not available");
        return;
    }
    if (Ffile::exists(&espfs, (char *)READING_CAL_FILENAME))
    {
        Ffile cfgfile(&espfs, (char *)READING_CAL_FILENAME);
        cfgfile.remove();
    }
}

static void save_cfg(void)
{
    ALL("temp_log_save_cfg");
    if (saved_cfg_not_updated())
        remove_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esp_diag.error(TEMPLOG_SAVE_CFG_FS_NOT_AVAILABLE);
        ERROR("temp_log_save_cfg FS not available");
        return;
    }
    Ffile cfgfile(&espfs, (char *)READING_CAL_FILENAME);
    if (!cfgfile.is_available())
    {
        esp_diag.error(TEMPLOG_SAVE_CFG_CANNOT_OPEN_FILE);
        ERROR("temp_log_save_cfg cannot open %s", READING_CAL_FILENAME);
        return;
    }
    // "{"temp_cal_offset":,"humi_cal_offset":}"
    // int file_len = 39 + 6 + 6 + 1;
    char buffer[52];
    os_memset(buffer, 0, 52);
    fs_sprintf(buffer,
               "{\"temp_cal_offset\":%d,\"humi_cal_offset\":%d}",
               temp_cal_offset,
               humi_cal_offset);
    cfgfile.n_append(buffer, os_strlen(buffer));
    espmem.stack_mon();
}

void set_cal_offset(int temp_offset, int humi_offset)
{
    temp_cal_offset = temp_offset;
    humi_cal_offset = humi_offset;
    save_cfg();
}

int get_temp_cal_offset(void)
{
    return temp_cal_offset;
}

int get_humi_cal_offset(void)
{
    return humi_cal_offset;
}