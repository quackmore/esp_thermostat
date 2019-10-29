/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_LOGGER_HPP__
#define __APP_LOGGER_HPP__

extern "C"
{
#include "osapi.h"
#include "ip_addr.h"
}

#include "library_sensor.hpp"

#define DHT_DATA ESPBOT_D2
#define DHT_TEMP_ID 12000
#define DHT_HUMI_ID 13000
#define DHT_BUFFERS 1

#define MAX6675_CS ESPBOT_D5
#define MAX6675_SCK ESPBOT_D6
#define MAX6675_SO ESPBOT_D7
#define MAX6675_ID 11000
#define MAX6675_BUFFERS 1

void logger_init(void);
void logger_start_at_boot(void);
void get_dweet_ip(void);

char *get_sensor_type_str(sensors_type_t);
void get_sensor_value_str(sensors_event_t *, char *);

typedef enum
{
  app_log_active = 2,
  app_log_idle = 1,
  app_log_ok = 0,
  app_log_bad_id = -1,
  app_log_already_running = -2,
  app_log_not_enough_heap = -10,
  app_log_no_fs = -20,
  app_log_no_file = -21
} app_log_res;

typedef enum
{
  error = -1,
  none = 0,
  log_to_memory,
  log_to_file,
  log_to_dweet,
  log_to_node,
  action_max_count
} app_log_action;

app_log_res app_logger_start(int sensor_id,
                             int poll_period,
                             char *desc,
                             app_log_action action,
                             bool start_at_boot);
app_log_res app_logger_stop(int sensor_id);
app_log_res app_logger_is_active(int sensor_id);
app_log_res app_logger_get_desc(int sensor_id, char *desc);
app_log_res app_logger_get_sensor_info(int sensor_id, sensor_t *sensor);
app_log_res app_logger_async_read(int sensor_id, struct espconn *ptr_espconn);
void app_logger_delete_class(int sensor_id);
sensors_event_t *get_log_event(int idx = 0);
app_log_action get_action_id(char *str);

struct sensor_rd_completed
{
  int sensor_id;
  Esp8266_Sensor *sensor;
  espconn *p_espconn;
  bool delete_sensor_class;
};

void app_logger_read_completed(void *param);

// struct data_log_s
// {
//   class Esp8266_Sensor *sensor;
//   char filename[32];
//   uint32 latest_timestamp;
// };

class D_logger
{
public:
  D_logger(class Esp8266_Sensor *sensor,
           int period,
           char *desc,
           app_log_action read_completed_func_id,
           bool start_at_boot = true);
  ~D_logger();

  // public so that the timer function can use them
  bool _first_call;
  int _poll_period;
  os_timer_t _poll_timer;
  app_log_action _read_completed_func_id;
  // void (*_read_completed_func)(D_logger *);
  class Esp8266_Sensor *_sensor;
  char _desc[33];
  bool _start_at_boot;
};

#endif