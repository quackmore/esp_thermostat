/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __ESPBOT_GLOBAL_HPP__
#define __ESPBOT_GLOBAL_HPP__


#include "espbot_logger.hpp"
#include "espbot_debug.hpp"
#include "espbot.hpp"
#include "spiffs_esp8266.hpp"
#include "espbot_wifi.hpp"
#include "espbot_mdns.hpp"
#include "espbot_sntp.hpp"
#include "espbot_webserver.hpp"
#include "espbot_ota.hpp"
#include "espbot_gpio.hpp"

extern char *espbot_release;
extern Flashfs espfs;
extern Esp_mem espmem;
extern Logger esplog;
extern Espbot espbot;
extern Mdns esp_mDns;
extern Sntp esp_sntp;
extern Websvr espwebsvr;
extern Ota_upgrade esp_ota;
extern Gpio esp_gpio;

#endif
