/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __TIMEDATE_H__
#define __TIMEDATE_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "c_types.h"
#ifdef __cplusplus
}
#endif

// declared public cause it's being stored into RTC memory
struct espbot_time
{
    uint32 sntp_time;
    uint32 rtc_time;
};

#endif