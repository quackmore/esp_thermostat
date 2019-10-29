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

extern "C"
{
#include "ip_addr.h"
#include "osapi.h"
}

typedef enum
{
  OTA_IDLE = 0,
  OTA_VERSION_CHECKING,
  OTA_VERSION_CHECKED,
  OTA_STARTED,
  OTA_SUCCESS,
  OTA_FAILED
} Ota_status_type;

class Ota_upgrade
{
private:
  Ota_status_type m_status;
  char m_host_str[16];
  ip_addr m_host;
  int m_port;
  char *m_path;
  bool m_check_version;
  bool m_reboot_on_completion;

  os_timer_t m_ota_timer;
  static void ota_timer_function(void *);
  static void ota_completed_cb(void *);

  int restore_cfg(void);          // return CFG_OK on success, otherwise CFG_ERROR
  int saved_cfg_not_update(void); // return CFG_OK when cfg does not require update
                                  // return CFG_REQUIRES_UPDATE when cfg require update
                                  // return CFG_ERROR otherwise

public:
  Ota_upgrade(){};
  ~Ota_upgrade(){};

  // config
  void init(void);
  void set_host(char *);
  void set_port(char *);
  void set_path(char *);
  void set_check_version(char *);        // accept "true" and "false"
  void set_reboot_on_completion(char *); // accept "true" and "false"
  char *get_host(void); // if str len > 0 heap memory is allocated
  int get_port(void);
  char *get_path(void);
  char *get_check_version(void);        // return "true" and "false"
  char *get_reboot_on_completion(void); // return "true" and "false"
  int save_cfg(void);                   // return 0 on success, otherwise 1 save configuration to flash

  // upgrade
  void start_upgrade(void);
  Ota_status_type get_status(void);
};

#endif