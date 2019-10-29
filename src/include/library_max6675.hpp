/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> modified this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __MAX6675_HPP__
#define __MAX6675_HPP__

extern "C"
{
#include "c_types.h"
#include "osapi.h"
}

#include "library_sensor.hpp"

class Max6675 : public Esp8266_Sensor
{
public:
  // cs_pin           => the gpio pin D1.. D8 for CS
  // sck_pin          => the gpio pin D1.. D8 for SCK
  // so_pin           => the gpio pin D1.. D8 for SO
  // id               => sensor indentifier
  // poll_interval    => in milliseconds (0 -> no polling)
  // buffer_length    => max number of stored readings  
  Max6675(int cs_pin, int sck_pin, int so_pin, int id, int poll_interval, int buffer_length);
  ~Max6675();

  int get_max_events_count(void);
  void force_reading(void (*callback)(void *), void *param);
  void getEvent(sensors_event_t *, int idx = 0); // idx = 0 => latest sample
                                                 // idx = 1 => previous sample
  void getSensor(sensor_t *);

  // this is private but into public section
  // for easy access from timer callback functions
  int m_id;
  uint16_t m_data;
  int m_cs;
  int m_sck;
  int m_so;
  int m_bit_counter;
  os_timer_t m_read_timer;
  int m_poll_interval;
  os_timer_t m_poll_timer;
  int *m_temperature_buffer;
  uint32_t *m_timestamp_buffer;
  bool *m_invalid_buffer;
  int m_max_buffer_size;
  int m_buffer_idx;
  bool m_force_reading;
  void (*m_force_reading_cb)(void *param);
  void *m_force_reading_param;
  bool m_reading_ongoing;
};

#endif