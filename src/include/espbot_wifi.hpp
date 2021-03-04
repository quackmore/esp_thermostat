/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __WIFI_HPP__
#define __WIFI_HPP__

extern "C"
{
#include "osapi.h"
#include "user_interface.h"
}

#define WIFI_WAIT_BEFORE_RECONNECT 5000
#define GOT_IP_AFTER_CONNECTION ((os_param_t)1)
#define GOT_IP_ALREADY_CONNECTED ((os_param_t)2)

void espwifi_init(void); // Will try to start as STATION:
                         //   IF there is no valid STATION cfg
                         //      or failed to connect
                         //   THEN will switch wifi to STATIONAP
                         //        (if there is a valid STATION cfg
                         //         will keep on trying to connect to AP)
                         // While working as STATION, any connection failure
                         // will switch wifi to STATIONAP

// WORKING MODE AND STATUS
void espwifi_get_ip_address(struct ip_info *);
void espwifi_work_as_ap(void);
void espwifi_connect_to_ap(void);
bool espwifi_is_connected(void); // true  -> connected to AP
                                 // false -> not connected to AP

// scan
void espwifi_scan_for_ap(struct scan_config *config, void (*)(void *), void *); // start a new AP scan
int espwifi_get_ap_count(void);                                                 // return the number of APs found
char *espwifi_get_ap_name(int);                                                 // return the name of AP number xx
char *espwifi_scan_results_json_stringify(char *dest = NULL, int len = 0);
// (from 0 to (ap_count-1))
void espwifi_free_ap_list(void);

// CONFIG
void espwifi_station_set_ssid(char *t_str, int t_len); // won't save configuraion to flash
void espwifi_station_set_pwd(char *t_str, int t_len);  // won't save configuraion to flash
void espwifi_ap_set_pwd(char *t_str, int t_len);       // won't save configuraion to flash
void espwifi_ap_set_ch(int ch);                        // won't save configuraion to flash
int espwifi_cfg_save(void);                            // returned values according to espbot_cfgfile enum
                                                       // save configuration to flash
char *espwifi_station_get_ssid(void);
char *espwifi_cfg_json_stringify(char *dest = NULL, int len = 0);
char *espwifi_status_json_stringify(char *dest = NULL, int len = 0);

#endif