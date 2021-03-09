/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_TEMP_LOG_HPP__
#define __APP_TEMP_LOG_HPP__

#define DHT_DATA ESPBOT_D5
#define DHT_TEMP_ID 50000
#define DHT_HUMI_ID 51000
#define DHT_BUFFERS 1

#define TEMP_LOG_LENGTH 60
#define INVALID_TEMP -500
#define INVALID_HUMI -500

void temp_log_init(void);
void init_temperature_readings(void);
void temp_log_read(void *);
int get_temp(int idx);
int get_humi(int idx);

// readings calibration offsets
int set_cal_offset(int temp_offset, int humi_offset);
char *temp_log_cfg_json_stringify(char *dest = NULL, int len = 0);

#endif