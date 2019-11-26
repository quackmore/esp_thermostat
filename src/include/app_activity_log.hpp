/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_ACTIVITY_LOG_HPP__
#define __APP_ACTIVITY_LOG_HPP__

#define ACTIVITY_LOG_LENGTH 60
#define ACTIVITY_LOG_HOST_IP "192.168.1.102"
#define ACTIVITY_LOG_HOST_PORT 1880
#define ACTIVITY_LOG_HOST_PATH "/activity_log"

typedef enum
{
    none = 0,
    temp_change,
    heater_change,
    mode_change,
    setpoint_change
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

void init_activity_logger(void);
void log_event(uint32 timestamp, activity_event_t type, int value);
int events_count(void);
struct activity_event *get_event(int idx);
void send_events_to_external_host(void);

#endif