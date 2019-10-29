/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __SNTP_HPP__
#define __SNTP_HPP__

extern "C"
{
#include "c_types.h"
}

class Sntp
{
private:

public:
  Sntp(){};
  ~Sntp(){};

  void start(void);
  void stop(void);
  uint32 get_timestamp();
  char *get_timestr(uint32);
};

#endif