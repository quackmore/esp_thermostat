/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_REMOTE_LOG_HPP__
#define __APP_REMOTE_LOG_HPP__

#define REMOTE_LOG_LENGTH 600

typedef enum
{
    none = 0,
    temp_change,
    heater_change,
    mode_change,
    setpoint_change,
    humi_change
} activity_event_t;

struct activity_event
{
    uint32 timestamp;
    activity_event_t type;
    int value;
};

void init_remote_logger(void);
void log_event(uint32 timestamp, activity_event_t type, int value);
int events_count(void);
struct activity_event *get_event(int idx);
void send_events_to_external_host(void);

// remote host configuration
int set_remote_log(bool enabled, char* host, int port, char* path);
char *remote_log_cfg_json_stringify(char *dest = NULL, int len = 0);

#endif