/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

extern "C"
{
#include "c_types.h"
}

#include "espbot_list.hpp"

#define LOG_OFF 0
#define LOG_FATAL 1
#define LOG_ERROR 2
#define LOG_WARN 3
#define LOG_INFO 4
#define LOG_DEBUG 5
#define LOG_TRACE 6
#define LOG_ALL 7
#define LOG_BUF_SIZE 512

// found somewhere on www.esp8266.com
// 3fffeb30 and 3fffffff is the designated area for the user stack

#define DEBUG_MAX_SAVED_ERRORS 20
#define DEBUG_MAX_FILE_SAVED_ERRORS 20

#define CFG_OK 0
#define CFG_REQUIRES_UPDATE 1
#define CFG_ERROR 2

class Logger
{
private:
  int m_serial_level;
  int m_memory_level;

  List<char> *m_log;

  int restore_cfg(void);          // return CFG_OK on success, otherwise CFG_ERROR
  int saved_cfg_not_update(void); // return CFG_OK when cfg does not require update
                                  // return CFG_REQUIRES_UPDATE when cfg require update
                                  // return CFG_ERROR otherwise
  int save_cfg(void);             // return CFG_OK on success, otherwise CFG_ERROR

public:
  Logger(){};
  ~Logger(){};

  void init_cfg();
  void essential_init();
  int get_serial_level(void);
  int get_memory_level(void);
  void set_levels(char t_serial_level, char t_memory_level);

  void fatal(const char *, ...);
  void error(const char *, ...);
  void warn(const char *, ...);
  void info(const char *, ...);
  void debug(const char *, ...);
  void trace(const char *, ...);
  void all(const char *, ...);

  char *get_log_head();
  char *get_log_next();
  int get_log_size();
};

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