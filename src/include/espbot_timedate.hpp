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

void timedate_init_essential(void);
void timedate_init(void);

void timedate_enable_sntp(void);
void timedate_disable_sntp(void);
bool timedate_sntp_enabled(void);
void timedate_start_sntp(void);
void timedate_stop_sntp(void);
void timedate_set_timezone(signed char);
signed char timedate_get_timezone(void);
void timedate_set_time_manually(uint32); // takes UTC time
uint32 timedate_get_timestamp();
char *timedate_get_timestr(uint32);

// {"sntp_enabled":1, "timezone":-12}
char *timedate_cfg_json_stringify(char *dest = NULL, int len = 0);
// {"timestamp":1589272200,"timedate":"Tue May 12 09:30:00 2020","sntp_enabled":0,"timezone":-12}
char *timedate_state_json_stringify(char *dest = NULL, int len = 0);
int timedate_cfg_save(void);

#endif