/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __SENSOR_H__
#define __SENSOR_H__

extern "C"
{
#include "c_types.h"
}
// Sensor types
typedef enum
{
    SENSOR_TYPE_TEMPERATURE = 1,
    SENSOR_TYPE_RELATIVE_HUMIDITY
} sensors_type_t;

// Sensor event
typedef struct
{
    int sensor_id;       // unique sensor identifier
    sensors_type_t type; // sensor type
    uint32_t timestamp;  // system time in milliseconds
    bool invalid;        // flag for setting an event as invalid
    union {
        float temperature;       // temperature (Celsius)
        float relative_humidity; // relative humidity in percent
    };
} sensors_event_t;

// Sensor details
// struct sensor_t is used to describe basic information about a specific sensor
typedef struct
{
    char name[12];       // sensor name
    int sensor_id;       // unique sensor identifier
    sensors_type_t type; // this sensor's type
    float max_value;     // maximum value of this sensor's value
    float min_value;     // minimum value of this sensor's value in SI units
    float resolution;    // smallest difference between two values reported by this sensor
    uint32 min_delay;    // min delay in microseconds between events
} sensor_t;

class Esp8266_Sensor
{
  public:
    Esp8266_Sensor() {}
    virtual ~Esp8266_Sensor() {}

    virtual void getSensor(sensor_t *) = 0;
    virtual int get_max_events_count(void) = 0;
    virtual void getEvent(sensors_event_t *, int idx = 0) = 0; // idx = 0 => latest event
                                                               // idx = 1 => previous event
                                                               // ...
    virtual void force_reading(void (*callback)(void *), void *param) = 0;
};

#endif