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

#define REMOTE_LOG_LENGTH 400
// #define REMOTE_LOG_HOST_IP "192.168.1.102"
// #define REMOTE_LOG_HOST_PORT 1880
// #define REMOTE_LOG_HOST_PATH "/remote_log"

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
    // union {
    //     int temperature;
    //     int heater_status;
    //     int ctrl_mode;
    //     int setpoint;
    // };
};

void init_remote_logger(void);
void log_event(uint32 timestamp, activity_event_t type, int value);
int events_count(void);
struct activity_event *get_event(int idx);
void send_events_to_external_host(void);

// remote host configuration
void set_remote_log(bool enabled, char* host, int port, char* path);
bool get_remote_log_enabled(void);
char *get_remote_log_host(void);
int get_remote_log_port(void);
char *get_remote_log_path(void);

#endif