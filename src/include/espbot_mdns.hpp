/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __MDNS_HPP__
#define __MDNS_HPP__

extern "C"
{
#include "espconn.h"
}

void mdns_init(void);
char *mdns_cfg_json_stringify(char *dest = NULL, int len = 0);
int mdns_cfg_save(void);
void mdns_enable(void);
void mdns_disable(void);
bool mdns_is_enabled(void);
void mdns_start(char *app_alias);
void mdns_stop(void);

#endif