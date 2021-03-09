/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

// SDK includes
extern "C"
{
#include "mem.h"
#include "drivers_dio_task.h"
#include "esp8266_io.h"
#include "user_interface.h"
}

#include "app.hpp"
#include "app_event_codes.h"
#include "app_remote_log.hpp"
#include "espbot_cfgfile.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_gpio.hpp"
#include "espbot_http_client.hpp"
#include "espbot_mem_macros.h"
#include "espbot_mem_mon.hpp"
#include "espbot_utils.hpp"
#include "espbot_wifi.hpp"

//
// remote host configuration
//

static struct
{
    bool enabled;
    char host[16];
    int port;
    char path[128];
} remote_log_cfg;

#define REMOTE_LOG_FILENAME ((char *)f_str("remote_log.cfg"))
// {"enabled":,"host":"","port":,"path":""}

static int remote_log_restore_cfg(void)
{
    ALL("remote_log_restore_cfg");
    if (!Espfile::exists(REMOTE_LOG_FILENAME))
        return CFG_cantRestore;
    Cfgfile cfgfile(REMOTE_LOG_FILENAME);
    bool enabled = (bool)cfgfile.getInt(f_str("enabled"));
    char host[16];
    os_memset(host, 0, 16);
    cfgfile.getStr(f_str("host"), host, 16);
    int port = cfgfile.getInt(f_str("port"));
    char path[128];
    os_memset(path, 0, 128);
    cfgfile.getStr(f_str("path"), host, 128);
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(REMOTELOG_RESTORE_CFG_ERROR);
        ERROR("remote_log_restore_cfg error");
        return CFG_error;
    }
    remote_log_cfg.enabled = enabled;
    os_memset(remote_log_cfg.host, 0, 16);
    os_strncpy(remote_log_cfg.host, host, 15);
    remote_log_cfg.port = port;
    os_memset(remote_log_cfg.path, 0, 128);
    os_strncpy(remote_log_cfg.path, path, 127);
    mem_mon_stack();
    return CFG_ok;
}

static int remote_log_saved_cfg_updated(void)
{
    ALL("remote_log_saved_cfg_updated");
    if (!Espfile::exists(REMOTE_LOG_FILENAME))
    {
        return CFG_notUpdated;
    }
    Cfgfile cfgfile(REMOTE_LOG_FILENAME);
    bool enabled = (bool)cfgfile.getInt(f_str("enabled"));
    char host[16];
    os_memset(host, 0, 16);
    cfgfile.getStr(f_str("host"), host, 16);
    int port = cfgfile.getInt(f_str("port"));
    char path[128];
    os_memset(path, 0, 128);
    cfgfile.getStr(f_str("path"), host, 128);
    mem_mon_stack();
    if (cfgfile.getErr() != JSON_noerr)
    {
        // no need to raise an error, the cfg file will be overwritten
        // dia_error_evnt(REMOTELOG_SAVED_CFG_UPDATED_ERROR);
        // ERROR("remote_log_saved_cfg_updated error");
        return CFG_error;
    }
    if ((remote_log_cfg.enabled != enabled) ||
        os_strncmp(remote_log_cfg.host, host, 16) ||
        (remote_log_cfg.port != port) ||
        os_strncmp(remote_log_cfg.path, path, 128))
    {
        return CFG_notUpdated;
    }
    return CFG_ok;
}

char *remote_log_cfg_json_stringify(char *dest, int len)
{
    // {"enabled":,"host":"","port":,"path":""}
    int msg_len = 40 + 1 + 16 + 6 + 128 + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(REMOTELOG_CFG_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("remote_log_cfg_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"enabled\":%d,\"host\":\"%s\",\"port\":%d,\"path\":\"%s\"}",
               remote_log_cfg.enabled,
               remote_log_cfg.host,
               remote_log_cfg.port,
               remote_log_cfg.path);
    mem_mon_stack();
    return msg;
}

int remote_log_cfg_save(void)
{
    ALL("remote_log_cfg_save");
    if (remote_log_saved_cfg_updated() == CFG_ok)
        return CFG_ok;
    Cfgfile cfgfile(REMOTE_LOG_FILENAME);
    if (cfgfile.clear() != SPIFFS_OK)
        return CFG_error;
    char str[192];
    remote_log_cfg_json_stringify(str, 192);
    int res = cfgfile.n_append(str, os_strlen(str));
    if (res < SPIFFS_OK)
        return CFG_error;
    mem_mon_stack();
    return CFG_ok;
}

int set_remote_log(bool enabled, char *host, int port, char *path)
{
    remote_log_cfg.enabled = enabled;
    os_memset(remote_log_cfg.host, 0, 16);
    os_strncpy(remote_log_cfg.host, host, 15);
    remote_log_cfg.port = port;
    os_memset(remote_log_cfg.path, 0, 128);
    os_strncpy(remote_log_cfg.path, path, 127);
    return remote_log_cfg_save();
}

//
// event log mngmt
//

static struct activity_event events[REMOTE_LOG_LENGTH];
static bool event_sent[REMOTE_LOG_LENGTH];
static int last_event_idx;

void print_last_events(void);

static Http_clt *espclient;

void init_remote_logger(void)
{
    ALL("init_remote_logger");
    int idx;
    for (idx = 0; idx < REMOTE_LOG_LENGTH; idx++)
    {
        events[idx].timestamp = 0;
        events[idx].type = none;
        events[idx].value = -500;
        // mark all events as sent
        event_sent[idx] = true;
    }
    last_event_idx = 0;

    if (remote_log_restore_cfg() != CFG_ok)
    {
        remote_log_cfg.enabled = false;
        os_memset(remote_log_cfg.host, 0, 16);
        remote_log_cfg.port = 0;
        os_memset(remote_log_cfg.path, 0, 128);
        dia_info_evnt(REMOTELOG_INIT_DEFAULT_CFG);
        INFO("init_remote_logger no cfg available");
    }

    espclient = new Http_clt;
}

void log_event(uint32 timestamp, activity_event_t type, int value)
{
    ALL("log_event");
    static bool overwriting_unreported_events = false;
    last_event_idx++;
    if (last_event_idx >= REMOTE_LOG_LENGTH)
        last_event_idx = 0;
    events[last_event_idx].timestamp = timestamp;
    events[last_event_idx].type = type;
    events[last_event_idx].value = value;
    // raise an error when overwriting not reported events
    // but just on the first occurrence
    if ((event_sent[last_event_idx] == false) &&
        remote_log_cfg.enabled &&
        !overwriting_unreported_events)
    {
        overwriting_unreported_events = true;
        dia_error_evnt(REMOTELOG_OVERWRITING_UNREPORTED_EVENTS);
        ERROR("log_event overwriting not yet reported events");
    }
    // clear error when no longer overwriting reported events
    // but just on the first occurrence
    if ((event_sent[last_event_idx] == true) &&
        remote_log_cfg.enabled &&
        overwriting_unreported_events)
    {
        overwriting_unreported_events = false;
        dia_info_evnt(REMOTELOG_NO_MORE_OVERWRITING_UNREPORTED_EVENTS);
        INFO("log_event no longer overwriting not yet reported events");
    }
    event_sent[last_event_idx] = false;
    // print_last_events();
}

int events_count(void)
{
    ALL("events_count");
    int count = 0;
    int idx = last_event_idx;
    while (events[idx].type != none)
    {
        count++;
        // move to previous event
        idx--;
        // check if event array boundary have been reached
        if (idx < 0)
            idx = REMOTE_LOG_LENGTH - 1;
        // check if it's back to last recorded event
        if (idx == last_event_idx)
            break;
    }
    return count;
}

struct activity_event *get_event(int idx)
{
    ALL("get_event");
    if (idx >= REMOTE_LOG_LENGTH)
        return NULL;
    int tmp_index = last_event_idx;
    int ii;
    idx %= REMOTE_LOG_LENGTH;
    for (ii = 0; ii < idx; ii++)
    {
        tmp_index--;
        if (tmp_index < 0)
            tmp_index = REMOTE_LOG_LENGTH - 1;
    }
    return &events[tmp_index];
}

//
// Logging activity to external host
//

os_timer_t delay_post;

static int get_next_event_to_be_sent(void)
{
    ALL("get_next_event_to_be_sent");
    int event_idx = last_event_idx;
    while (event_sent[event_idx])
    {
        event_idx -= 1;
        if (event_idx < 0)
            event_idx = REMOTE_LOG_LENGTH - 1;
        if (event_idx == last_event_idx)
            return -1;
    }
    return event_idx;
}

// remote_host_errors

typedef enum
{
    success = 0,
    host_not_reachable,
    host_bad_answer,
    host_req_error
} comm_res_type;

static void comm_outcome(comm_res_type result)
{
    static int comm_error_counter = 0;

    switch (result)
    {
    case success:
        if (comm_error_counter > 0)
        {
            dia_info_evnt(REMOTELOG_SERVER_AVAILABLE, comm_error_counter);
        }
        comm_error_counter = 0;
        break;
    case host_not_reachable:
        if (comm_error_counter == 0)
            dia_info_evnt(REMOTELOG_SERVER_NOT_AVAILABLE, espclient->get_status());
        comm_error_counter++;
        INFO("remote_log_post_info unexpected webclient status %d", espclient->get_status());
        espclient->disconnect(NULL, NULL);
        break;
    case host_bad_answer:
        dia_error_evnt(REMOTELOG_CHECK_ANSWER_UNEXPECTED_HOST_ANSWER, espclient->parsed_response->http_code);
        ERROR("remote_log_check_answer unexpected host answer %d, %s", espclient->parsed_response->http_code, espclient->parsed_response->body);
        espclient->disconnect(NULL, NULL);
        break;
    case host_req_error:
        dia_info_evnt(REMOTELOG_SERVER_DIDNT_ANSWER, espclient->get_status());
        INFO("remote_log_check_answer unexpected webclient status %d", espclient->get_status());
        espclient->disconnect(NULL, NULL);
        break;
    default:
        break;
    }
}

static void post_info(void *param);

static void check_answer(void *param)
{
    ALL("remote_log_check_answer");
    switch (espclient->get_status())
    {
    case HTTP_CLT_RESPONSE_READY:
        if (espclient->parsed_response->http_code != HTTP_OK)
        {
            // POST failed
            comm_outcome(host_bad_answer);
        }
        else
        {
            // POST succeeded
            // mark event as sent
            int event_idx = (int)param;
            event_sent[event_idx] = true;
            TRACE("remote_log_check_answer marking event %d as sent", event_idx);
            // check if there are any unsent events
            event_idx = get_next_event_to_be_sent();
            TRACE("remote_log_check_answer next event to be sent: %d", event_idx);
            if (event_idx > 0)
            {
                // to avoid unnecessary stack growth
                // using a timer instead of direct call
                os_timer_disarm(&delay_post);
                os_timer_setfn(&delay_post, (os_timer_func_t *)post_info, (void *)event_idx);
                os_timer_arm(&delay_post, 100, 0);
            }
            else
            {
                // no more unsent events, disconnecting
                espclient->disconnect(NULL, NULL);
            }
        }
        break;
    default:
        comm_outcome(host_req_error);
        break;
    }
}

static void post_info(void *param)
{
    ALL("remote_log_post_info");
    switch (espclient->get_status())
    {
    case HTTP_CLT_CONNECTED:
    case HTTP_CLT_RESPONSE_READY:
    {
        comm_outcome(success);

        int event_idx = (int)param;

        // {"timestamp":4294967295,"type":1,"value":-1234}
        char event_str[48];
        fs_sprintf(event_str,
                   "{\"timestamp\":%d,\"type\":%d,\"value\":%d}",
                   events[event_idx].timestamp,
                   events[event_idx].type,
                   events[event_idx].value);
        TRACE("remote_log_post_info event str: %s", event_str);

        // "POST  HTTP/1.1rnHost: 111.111.111.111rnContent-Type: application/jsonrnAccept: */*rnConnection: keep-alivernContent-Length: 48rnrn{"timestamp":4294967295,"type":1,"value":-1234}rn"
        int msg_len = 179 + os_strlen(remote_log_cfg.path);
        char *msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(REMOTELOG_POST_INFO_HEAP_EXHAUSTED, msg_len);
            ERROR("remote_log_post_info - heap exausted [%d]", msg_len);
            espclient->disconnect(NULL, NULL);
            break;
        }
        fs_sprintf(msg,
                   "POST %s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Content-Type: application/json\r\n",
                   remote_log_cfg.path,
                   remote_log_cfg.host);
        fs_sprintf((msg + os_strlen(msg)),
                   "Accept: */*\r\n"
                   "Connection: keep-alive\r\n"
                   "Content-Length: %d\r\n\r\n"
                   "%s\r\n",
                   os_strlen(event_str),
                   event_str);
        TRACE("remote_log_post_info POSTing str: %s", msg);
        TRACE("remote_log_post_info event %d was sent", event_idx);
        espclient->send_req(msg, os_strlen(msg), check_answer, (void *)event_idx);
    }
    break;
    default:
        comm_outcome(host_not_reachable);
        break;
    }
}

void send_events_to_external_host(void)
{
    ALL("send_events_to_external_host");

    if (!remote_log_cfg.enabled)
        return;
    if (!espwifi_is_connected())
        return;
    struct ip_addr host_ip;
    atoipaddr(&host_ip, remote_log_cfg.host);

    int event_idx = get_next_event_to_be_sent();
    TRACE("send_events_to_external_host event to be sent: %d", event_idx);
    if (event_idx < 0)
        // there are no unsent events
        return;
    espclient->connect(host_ip, remote_log_cfg.port, post_info, (void *)event_idx);
}

void print_last_events(void)
{
    // this is just for debug, never called
    int idx;
    struct activity_event *event_ptr;
    fs_printf("EVENTS BEGIN");
    for (idx = 0; idx < 10; idx++)
    {
        event_ptr = get_event(idx);
        fs_printf("%d - %d - %d - %d - [%d] - (%d)\n",
                  idx,
                  event_ptr->timestamp,
                  event_ptr->type,
                  event_ptr->value,
                  ((event_ptr - &events[0]) / sizeof(struct activity_event)),
                  event_sent[idx]);
    }
    fs_printf("EVENTS END\n");
}