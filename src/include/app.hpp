/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_HPP__
#define __APP_HPP__

#include "library_dht.hpp"
#include "library_max6675.hpp"

void app_init_before_wifi(void);
void app_init_after_wifi(void);
void app_deinit_on_wifi_disconnect(void);
uint32 get_last_reboot_date(void);

extern char *app_name;
extern char *app_release;

extern Dht *dht22;
extern Max6675 *max6675;

#endif