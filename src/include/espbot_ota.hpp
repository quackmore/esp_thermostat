/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __OTA_UPGRADE_HPP__
#define __OTA_UPGRADE_HPP__

// extern "C"
// {
// #include "ip_addr.h"
// #include "osapi.h"
// }

typedef enum
{
  OTA_idle = 0,
  OTA_version_checking,
  OTA_version_checked,
  OTA_upgrading,
  OTA_success,
  OTA_failed,
  OTA_already_to_the_lastest
} Ota_status_type;

// config
void ota_init(void);
void ota_set_host(char *);
void ota_set_port(unsigned int);
void ota_set_path(char *);
void ota_set_check_version(bool);
void ota_set_reboot_on_completion(bool);
int ota_cfg_save(void);
char *ota_cfg_json_stringify(char *dest = NULL, int len = 0);

void ota_start(void);
Ota_status_type ota_get_status(void);
Ota_status_type ota_get_last_result(void);
void ota_set_cb_on_completion(void (*fun)(void *));
void ota_set_cb_param(void *);

// char *get_host(void);
// ip_addr *get_host_ip(void);
// int get_port(void);
// char *get_path(void);
// bool get_check_version(void);
// bool check_version(void);
// bool get_reboot_on_completion(void);
// bool reboot_on_completion(void);
// 
// void ota_set_status(Ota_status_type);
// void ota_set_last_result(Ota_status_type);
// int ota_get_rel_cmp_result(void);
// void ota_set_rel_cmp_result(int);
// void ota_cb_on_completion(void);

#endif