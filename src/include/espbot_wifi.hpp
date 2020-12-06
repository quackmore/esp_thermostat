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

struct fast_scan
{
  char ssid_len;
  char *ch_list;
  char ch_count;
  int ch_idx;
  void (*callback)(void *);
  void *param;
  sint8 best_rssi;
  char best_channel;
  char best_bssid[6];
};

class Wifi
{
public:
  Wifi(){};
  ~Wifi(){};

  static void init(void); // Will try to start as STATION:
                          //   IF there is no valid STATION cfg
                          //      or failed to connect
                          //   THEN will switch wifi to STATIONAP
                          //        (if there is a valid STATION cfg
                          //         will keep on trying to connect to AP)
                          // While working as STATION, any connection failure
                          // will switch wifi to STATIONAP

  static void station_set_ssid(char *t_str, int t_len); // won't save configuraion to flash
  static void station_set_pwd(char *t_str, int t_len);  // won't save configuraion to flash
  static void ap_set_pwd(char *t_str, int t_len);       // won't save configuraion to flash
  static void ap_set_ch(int ch);                        // won't save configuraion to flash
  static char *station_get_ssid(void);
  static char *station_get_password(void);
  static char *ap_get_password(void);
  static int ap_get_ch(void);

  static int save_cfg(void); // return 0 on success, otherwise 1 save configuration to flash

  static void set_stationap(void); // static because called by timer exhaustion (pointer required)
  static void connect(void);       // static because called by timer exhaustion (pointer required)
  static bool is_connected(void);  // true  -> connected to AP
                                   // false -> not connected to AP

  static void scan_for_ap(struct scan_config *, void (*)(void *), void *); // start a new AP scan
  static int get_ap_count(void);                                           // return the number of APs found
  static char *get_ap_name(int);                                           // return the name of AP number xx (from 0 to (ap_count-1))
  static void free_ap_list(void);
  static void fast_scan_for_best_ap(char *ssid, char ch_list[], char ch_count, void (*callback)(void *), void *param);
  static struct fast_scan *get_fast_scan_results(void);
  static int get_op_mode(void);
  static void get_ip_address(struct ip_info *);
};

#endif