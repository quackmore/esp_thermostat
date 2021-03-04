/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __WEBSERVER_HPP__
#define __WEBSERVER_HPP__

// extern "C"
// {
// #include "c_types.h"
// #include "espconn.h"
// }

#define SERVER_PORT 80

typedef enum
{
  http_svr_up = 0,
  http_svr_down
} Http_svr_status;

void http_svr_init(void);
void http_svr_start(uint32);
void http_svr_stop(void);
Http_svr_status http_svr_get_status(void);

#endif