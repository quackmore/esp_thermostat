/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __ESPBOT_HPP__
#define __ESPBOT_HPP__

extern "C"
{
#include "osapi.h"
}

enum
{
  SIG_staMode_gotIp = 0,
  SIG_staMode_disconnected,
  SIG_softapMode_staConnected,
  SIG_softapMode_staDisconnected,
  SIG_softapMode_ready,
  SIG_http_checkPendingResponse,
  SIG_next_function
};

enum
{
  ESPBOT_restart = 0,
  ESPBOT_rebootAfterOta
};

// execute a function from a task
void next_function(void (*fun)(void));

// make espbot_init available to user_main.c
extern "C" void espbot_init(void);

void espbot_reset(int);
char *espbot_get_name(void);
char *espbot_get_version(void);
uint32 espbot_get_last_reboot_time(void);
void espbot_set_name(char *);
int espbot_cfg_save(void);
char *espbot_cfg_json_stringify(char *dest = NULL, int len = 0);
char *espbot_info_json_stringify(char *dest = NULL, int len = 0);

#endif
