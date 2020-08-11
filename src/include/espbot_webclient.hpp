/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __WEBCLIENT_HPP__
#define __WEBCLIENT_HPP__

extern "C"
{
#include "c_types.h"
#include "espconn.h"
}

#include "espbot_http.hpp"


// Init the webclient <-> espconn association data strucures
void init_webclients_data_stuctures(void);

#define WEBCLNT_COMM_TIMEOUT 10000
#define WEBCLNT_SEND_REQ_TIMEOUT 10000

typedef enum
{
  WEBCLNT_DISCONNECTED = 1,
  WEBCLNT_CONNECT_FAILURE,
  WEBCLNT_CONNECT_TIMEOUT,
  WEBCLNT_CONNECTING,
  WEBCLNT_CONNECTED,
  WEBCLNT_CANNOT_SEND_REQUEST,
  WEBCLNT_WAITING_RESPONSE,
  WEBCLNT_RESPONSE_ERROR,
  WEBCLNT_RESPONSE_TIMEOUT,
  WEBCLNT_RESPONSE_READY
} Webclnt_status_type;

class Webclnt
{
private:
  struct espconn _esp_conn;
  esp_tcp _esptcp;
  struct ip_addr _host;
  uint32 _port;
  Webclnt_status_type _status;

  void (*_completed_func)(void *);
  void *_param;
  void format_request(char *);

public:
  Webclnt();
  ~Webclnt();

  os_timer_t _connect_timeout_timer;
  os_timer_t _send_req_timeout_timer;
  uint32 _comm_timeout;

  char *request;
  int req_len;
  Http_parsed_response *parsed_response;

  // connect will temporary change webclient status to WEBCLNT_CONNECTING
  // and will end up into one of the following:
  // WEBCLNT_CONNECT_FAILURE: espconn_connect failed (sigh!)
  // WEBCLNT_CONNECTED
  // WEBCLNT_CONNECT_TIMEOUT
  // WEBCLNT_DISCONNECTED (??) not sure so just in case
  void connect(struct ip_addr, uint32, void (*completed_func)(void *), void *param, int comm_tout = 10000);

  // disconnect will change webclient status to WEBCLNT_DISCONNECTED
  void disconnect(void (*completed_func)(void *), void *param);

  // send_req will only act if status is WEBCLNT_CONNECTED or WEBCLNT_RESPONSE_READY
  // otherwise an error will be logged and status will be set to WEBCLNT_CANNOT_SEND_REQUEST
  //
  // send_req will temporary change webclient status to WEBCLNT_WAITING_RESPONSE
  // and will end up into one of the following:
  // WEBCLNT_CONNECT_FAILURE: espconn_connect failed (sigh!)
  // WEBCLNT_CONNECTED
  // WEBCLNT_CONNECT_TIMEOUT
  // WEBCLNT_DISCONNECTED (??) not sure so just in case
  void send_req(char *msg, int msg_len, void (*completed_func)(void *), void *param);

  Webclnt_status_type get_status(void);

  void update_status(Webclnt_status_type);

  void call_completed_func(void);

  void print_status(void);
};

/* 

EXAMPLE EXAMPLE EXAMPLE EXAMPLE EXAMPLE EXAMPLE EXAMPLE EXAMPLE EXAMPLE

code structure using callbacks
1) create web client
2) connect (once connected or timeout call get_info)
3) get_info: send request (on answer or timeout call check_info)
4) check_info: on completion disconnect (one disconnected delete web client)

static Webclnt *espclient;

void free_client(void *)
{
    delete espclient;
}

void check_info(void *param)
{
    switch (espclient->get_status())
    {
    case WEBCLNT_RESPONSE_READY:
        if (espclient->parsed_response->body)
        {
            Server responded: espclient->parsed_response->body
            do something ...
        }
        break;
    default:
        Ops ... webclient status is not what expected [espclient->get_status()]
        os_printf("wc_get_version: Ops ... webclient status is %d\n", espclient->get_status());
        break;
    }
    espclient->disconnect(free_client, NULL);
}

void get_info(void *param)
{
    switch (espclient->get_status())
    {
    case WEBCLNT_CONNECTED:
        espclient->send_req(<client_request>, check_info, NULL);
        break;
    default:
        Ops ... webclient status is not what expected [espclient->get_status()]
        os_printf("wc_get_version: Ops ... webclient status is %d\n", espclient->get_status());
        espclient->disconnect(free_client, NULL);
        break;
    }
}

void whatever(void)
{
    ...
    espclient = new Webclnt;
    espclient->connect(<host_ip>, <host_port>, get_info, NULL);
    
    ...
}

*/
#endif