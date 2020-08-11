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

class Mdns
{
private:
  bool _enabled;
  bool _running;
  struct mdns_info _info;

  int restore_cfg(void);           // return CFG_OK on success, otherwise CFG_ERROR
  int saved_cfg_not_updated(void); // return CFG_OK when cfg does not require update
                                   // return CFG_REQUIRES_UPDATE when cfg require update
                                   // return CFG_ERROR otherwise

public:
  Mdns(){};
  ~Mdns(){};

  void init(void);
  int save_cfg(void); // return CFG_OK on success, otherwise CFG_ERROR

  void enable(void);
  void disable(void);
  bool is_enabled(void);

  void start(char *app_alias);
  void stop(void);
};

#endif