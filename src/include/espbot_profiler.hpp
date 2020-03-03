/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __PROFILER_HPP__
#define __PROFILER_HPP__

extern "C"
{
#include "c_types.h"
}

class Profiler
{
private:
  char *m_msg;
  uint32_t m_start_time_us;
  uint32_t m_stop_time_us;

public:
  Profiler(char *); // pass the message to be printed
                    // constructor will start the timer
  ~Profiler();      // destructor will stop the timer and print elapsed msg to serial
  // EXAMPLE
  // {
  //    Profiler esp_profiler;
  //    ...
  //    this is the code you want to profile
  //    place it into a block
  //    and declare a Profiler object at the beginning
  //    ...
  // }
};

#endif