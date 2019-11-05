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

#define DHT_DATA ESPBOT_D1
#define DHT_TEMP_ID 50000
#define DHT_HUMI_ID 51000
#define DHT_BUFFERS 1

#define TEMP_LOG_LENGTH 32
#define INVALID_TEMP -500

void temp_log_init(void);
void temp_log_read(struct date *);
int get_temp(int idx);

#endif