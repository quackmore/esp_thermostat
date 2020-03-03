/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __RTC_MEM_MAP_H__
#define __RTC_MEM_MAP_H__

#include "espbot_timedate.h"

// espbot RTC mem usage
#define RTC_MEM_START 64
#define RTC_TIMEDATE (RTC_MEM_START)
// end of espbot RTC mem usage
#define RTC_FREE (RTC_TIMEDATE + sizeof(struct espbot_time))

#endif