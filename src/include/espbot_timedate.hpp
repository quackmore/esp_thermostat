/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __TIMEDATE_HPP__
#define __TIMEDATE_HPP__

extern "C"
{
#include "c_types.h"
}

class TimeDate
{
private:
  bool _sntp_enabled;
  bool _sntp_running;
  signed char _timezone;

  int restore_cfg(void);           // return CFG_OK on success, otherwise CFG_ERROR
  int saved_cfg_not_updated(void); // return CFG_OK when cfg does not require update
                                   // return CFG_REQUIRES_UPDATE when cfg require update
                                   // return CFG_ERROR otherwise

public:
  TimeDate(){};
  ~TimeDate(){};

  void init(void);
  int save_cfg(void); // return CFG_OK on success, otherwise CFG_ERROR

  void enable_sntp(void);
  void disable_sntp(void);
  bool sntp_enabled(void);

  void start_sntp(void);
  void stop_sntp(void);

  void set_timezone(signed char);
  signed char get_timezone(void);

  void set_time_manually(uint32); // takes UTC time

  uint32 get_timestamp();
  char *get_timestr(uint32);
};

#endif