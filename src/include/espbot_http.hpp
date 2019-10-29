/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __HTTP_HPP__
#define __HTTP_HPP__

extern "C"
{
#include "c_types.h"
#include "espconn.h"
}

#include "espbot_queue.hpp"

// HTTP status codes
#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_ACCEPTED 202
#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_CONFLICT 409
#define HTTP_SERVER_ERROR 500

#define HTTP_CONTENT_TEXT "text/html"
#define HTTP_CONTENT_JSON "application/json"

//
// HTTP ERROR MESSAGES
//
// get error text description from error code
char *http_error_msg(int code);
// get error message description formatted as json
// takes as input:
//    code: http error code
//    reason: custom error description
// example:
// {
//    "error" : {
//       "code"    : 500,
//       "message" : "Internal Server Error",
//       "reason"  : "Memory allocation failed"
//    }
// }
char *http_json_error_msg(int code, char *reason);

typedef enum
{
  HTTP_GET = 0,
  HTTP_POST,
  HTTP_PUT,
  HTTP_PATCH,
  HTTP_DELETE,
  HTTP_OPTIONS,
  HTTP_UNDEFINED
} Http_methods;

//
// HTTP REQUESTS
//

class Http_parsed_req
{
public:
  Http_parsed_req();
  ~Http_parsed_req();
  bool no_header_message; // tells if HTTP request contains only content with no header
                          // (a POST msg was splitted into two different messages
                          // the first with the header the second with the content)
  Http_methods req_method;
  char *url;
  char *acrh;
  char *origin;
  int h_content_len;
  int content_len;
  char *req_content;
};

// parse_request takes the incoming string req and fill in parsed_req
void http_parse_request(char *req, Http_parsed_req *parsed_req);

// espconn_send custom callback
void http_sentcb(void *arg);

// http requests can come in split into different messages
// if that is the case then the incomplete messages are queued
// and elaborated on completion
void http_save_pending_request(void *arg, char *precdata, unsigned short length, Http_parsed_req *parsed_req);

// will check for pending requests on p_espconn
// will add the new message part new_msg
// will call msg_complete function one the message is complete
void http_check_pending_requests(struct espconn *p_espconn, char *new_msg, void (*msg_complete)(void *, char *, unsigned short ));


//
// HTTP RESPONSE
//

// http responses are queued when espconn_send is busy

class Http_header
{
public:
  Http_header();
  ~Http_header();
  int m_code;
  char *m_content_type;
  char *m_acrh;
  char *m_origin;
  int m_content_length;
  int m_content_range_start;
  int m_content_range_end;
  int m_content_range_total;
};

struct http_send
{
  struct espconn *p_espconn;
  char *msg;
};

struct http_split_send
{
  struct espconn *p_espconn;
  char *content;
  int content_size;
  int content_transferred;
  void (*action_function)(struct http_split_send *);
};

extern Queue<struct http_split_send> *pending_split_send;

// check if there are pending http send
void http_check_pending_send(void);

// quick format an http response and send it
//    free_msg must be false when passing a "string" allocated into text or data segment
//    free_msg must be true when passing an heap allocated string
void http_response(struct espconn *p_espconn, int code, char *content_type, char *msg, bool free_msg);

// format header string
char *http_format_header(class Http_header *);

// sending http messages using espconn

// http_send will take care of splitting the message according to the buffer size
// and will repeadetely call http_send_buffer
// will free the msg buffer after it has been sent (msg must be allocated on heap)
void http_send(struct espconn *p_espconn, char *msg);

// http_send_buffer will manage calling espconn_send avoiding new calls before completion
void http_send_buffer(struct espconn *p_espconn, char *msg);

//
// incoming response for a client
//

class Http_parsed_response
{
public:
  Http_parsed_response();
  ~Http_parsed_response();
  
  bool no_header_message; // tells if HTTP response contains no header
                          // (a msg was splitted into two different messages
                          // the first with the header the second with the content)
  int http_code;
  int content_range_start;
  int content_range_end;
  int content_range_size;
  int h_content_len;
  int content_len;
  char *body;
};

// parse_response takes the incoming string response and fill in parsed_response
void http_parse_response(char *response, int length, Http_parsed_response *parsed_response);

// http responses can come in split into different messages
// if that is the case then the incomplete messages are queued
// and elaborated on completion
void http_save_pending_response(struct espconn *p_espconn, char *precdata, unsigned short length, Http_parsed_response *parsed_res);

// will check for pending responses on p_espconn
// will add the new message part new_msg
// will call msg_complete function one the message is complete
void http_check_pending_responses(struct espconn *p_espconn, char *new_msg, int length, void (*msg_complete)(void *, char *, unsigned short ));

//
//
void clean_pending_responses(struct espconn *p_espconn);

//
// init and clear http data structures
//

void set_http_msg_max_size(int size);
int get_http_msg_max_size(void);

void http_init(void);
void http_queues_clear(void);

#endif