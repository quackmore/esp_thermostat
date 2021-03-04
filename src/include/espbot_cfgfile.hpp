/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __CFGFILE_HPP__
#define __CFGFILE_HPP__

#include "espbot_spiffs.hpp"
#include "espbot_json.hpp"

enum {
  CFG_ok = 0,
  CFG_cantRestore,
  CFG_error,
  CFG_notUpdated
};

/**
 * @brief Config file class
 * A config file with JSON syntax
 * 
 */
class Cfgfile: public Espfile, public JSONP
{
public:
  char *_json_str;
  
  /**
   * @brief Construct a new Cfgfile object
   * 
   * @param filename 
   */
  Cfgfile(char *filename);
  ~Cfgfile();
};


#endif