/* DHT library

MIT license
written by Adafruit Industries
*/
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> modified this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __DHT_H__
#define __DHT_H__

extern "C"
{
#include "c_types.h"
#include "osapi.h"
}

#include "library_sensor.hpp"
#include "library_common_types.hpp"

typedef enum
{
  DHT11 = 11,
  DHT22 = 2,
  DHT21 = 21,
  AM2301 = 21
} Dht_type;

class Dht
{
public:
  // pin             => the gpio pin D1.. D8
  // type            => the sensor type (e.g. DHT22)
  // temperature_id  => sensor indentifier
  // humidity_id     => sensor indentifier
  // poll_interval   => in milliseconds (0 -> no polling)
  // buffer_length   => max number of stored readings
  Dht(int pin, Dht_type type, int temperature_id, int humidity_id, int poll_interval, int buffer_length);
  ~Dht();

  class Temperature : public Esp8266_Sensor
  {
  public:
    Temperature(Dht *, int id);
    ~Temperature();
    int get_max_events_count(void);
    void force_reading(void (*callback)(void *), void *param);
    void getEvent(sensors_event_t *, int idx = 0); // idx = 0 => latest sample
                                                   // idx = 1 => previous sample
    void getSensor(sensor_t *);

  private:
    Dht *m_parent;
    int m_id;
  };

  Temperature temperature;

  class Humidity : public Esp8266_Sensor
  {
  public:
    Humidity(Dht *, int id);
    ~Humidity();
    int get_max_events_count(void);
    void force_reading(void (*callback)(void *), void *param);
    void getEvent(sensors_event_t *, int idx = 0); // idx = 0 => latest sample
                                                   // idx = 1 => previous sample
    void getSensor(sensor_t *);

  private:
    Dht *m_parent;
    int m_id;
  };

  Humidity humidity;

  // this is private but into public section
  // for making variables accessible to timer callback functions
  uint8_t m_data[5];
  int m_pin;
  Dht_type m_type;
  int m_poll_interval;
  os_timer_t m_poll_timer;
  struct do_seq *m_dht_out_sequence;
  struct di_seq *m_dht_in_sequence;
  int *m_temperature_buffer;
  int *m_humidity_buffer;
  bool *m_invalid_buffer;
  uint32_t *m_timestamp_buffer;
  int m_max_buffer_size;
  int m_buffer_idx;
  bool m_force_reading;
  void (*m_force_reading_cb)(void *param);
  void *m_force_reading_param;
  bool m_reading_ongoing;
};

#endif