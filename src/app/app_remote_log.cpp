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
#include "library_dio_task.h"
#include "esp8266_io.h"
}

#include "app.hpp"
#include "app_event_codes.h"
#include "app_remote_log.hpp"
#include "espbot_config.hpp"
#include "espbot_global.hpp"
#include "espbot_gpio.hpp"
#include "espbot_mem_macros.h"
#include "espbot_utils.hpp"
#include "espbot_webclient.hpp"

//
// remote host configuration
//

static struct
{
    bool enabled;
    char *host;
    int port;
    char *path;
} remote_log_vars;

#define REMOTE_LOG_FILENAME f_str("remote_log.cfg")

static bool restore_cfg(void)
{
    ALL("remote_log_restore_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(REMOTELOG_RESTORE_CFG_FS_NOT_AVAILABLE);
        ERROR("remote_log_restore_cfg FS not available");
        return false;
    }
    File_to_json cfgfile(REMOTE_LOG_FILENAME);
    if (cfgfile.exists())
    {
        // "{"enabled": ,"host": "","port": ,"path": ""}",
        // enabled
        if (cfgfile.find_string(f_str("enabled")))
        {
            esp_diag.error(REMOTELOG_RESTORE_CFG_INCOMPLETE);
            ERROR("remote_log_restore_cfg cannot find \"enabled\"");
            return false;
        }
        remote_log_vars.enabled = atoi(cfgfile.get_value());
        // host
        if (cfgfile.find_string(f_str("host")))
        {
            esp_diag.error(REMOTELOG_RESTORE_CFG_INCOMPLETE);
            ERROR("remote_log_restore_cfg cannot find \"host\"");
            return false;
        }
        if (remote_log_vars.host)
            delete[] remote_log_vars.host;
        remote_log_vars.host = new char[os_strlen(cfgfile.get_value()) + 1];
        os_strcpy(remote_log_vars.host, cfgfile.get_value());
        // port
        if (cfgfile.find_string(f_str("port")))
        {
            esp_diag.error(REMOTELOG_RESTORE_CFG_INCOMPLETE);
            ERROR("remote_log_restore_cfg cannot find \"port\"");
            return false;
        }
        remote_log_vars.port = atoi(cfgfile.get_value());
        // path
        if (cfgfile.find_string(f_str("path")))
        {
            esp_diag.error(REMOTELOG_RESTORE_CFG_INCOMPLETE);
            ERROR("remote_log_restore_cfg cannot find \"path\"");
            return false;
        }
        if (remote_log_vars.path)
            delete[] remote_log_vars.path;
        remote_log_vars.path = new char[os_strlen(cfgfile.get_value()) + 1];
        os_strcpy(remote_log_vars.path, cfgfile.get_value());
        return true;
    }
    return false;
}

static bool saved_cfg_not_updated(void)
{
    ALL("remote_log_saved_cfg_not_updated");
    if (!espfs.is_available())
    {
        esp_diag.error(REMOTELOG_SAVED_CFG_NOT_UPDATED_FS_NOT_AVAILABLE);
        ERROR("remote_log_saved_cfg_not_updated FS not available");
        return true;
    }
    File_to_json cfgfile(REMOTE_LOG_FILENAME);
    if (cfgfile.exists())
    {
        // "{"enabled": ,"host": "","port": ,"path": ""}",
        // enabled
        if (cfgfile.find_string(f_str("enabled")))
        {
            esp_diag.error(REMOTELOG_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("remote_log_saved_cfg_not_updated cannot find \"enabled\"");
            return true;
        }
        if (remote_log_vars.enabled != atoi(cfgfile.get_value()))
            return true;
        // host
        if (cfgfile.find_string(f_str("host")))
        {
            esp_diag.error(REMOTELOG_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("remote_log_saved_cfg_not_updated cannot find \"host\"");
            return true;
        }
        if (0 != os_strcmp(remote_log_vars.host, cfgfile.get_value()))
            return true;
        // port
        if (cfgfile.find_string(f_str("port")))
        {
            esp_diag.error(REMOTELOG_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("remote_log_saved_cfg_not_updated cannot find \"port\"");
            return true;
        }
        if (remote_log_vars.port != atoi(cfgfile.get_value()))
            return true;
        // path
        if (cfgfile.find_string(f_str("path")))
        {
            esp_diag.error(REMOTELOG_SAVED_CFG_NOT_UPDATED_INCOMPLETE);
            ERROR("remote_log_saved_cfg_not_updated cannot find \"path\"");
            return true;
        }
        if (0 != os_strcmp(remote_log_vars.path, cfgfile.get_value()))
            return true;
        return false;
    }
    espmem.stack_mon();
    return true;
}

static void remove_cfg(void)
{
    ALL("remote_log_remove_cfg");
    if (!espfs.is_available())
    {
        esp_diag.error(REMOTELOG_REMOVE_CFG_FS_NOT_AVAILABLE);
        ERROR("remote_log_remove_cfg FS not available");
        return;
    }
    if (Ffile::exists(&espfs, (char *)REMOTE_LOG_FILENAME))
    {
        Ffile cfgfile(&espfs, (char *)REMOTE_LOG_FILENAME);
        cfgfile.remove();
    }
}

static void save_cfg(void)
{
    ALL("remote_log_save_cfg");
    if (saved_cfg_not_updated())
        remove_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esp_diag.error(REMOTELOG_SAVE_CFG_FS_NOT_AVAILABLE);
        ERROR("remote_log_save_cfg FS not available");
        return;
    }
    Ffile cfgfile(&espfs, (char *)REMOTE_LOG_FILENAME);
    if (!cfgfile.is_available())
    {
        esp_diag.error(REMOTELOG_SAVE_CFG_CANNOT_OPEN_FILE);
        ERROR("remote_log_save_cfg cannot open %s", REMOTE_LOG_FILENAME);
        return;
    }
    int file_len = 44 + 1 + os_strlen(remote_log_vars.host) + 5 + os_strlen(remote_log_vars.path) + 1;
    Heap_chunk buffer(file_len);
    if (buffer.ref == NULL)
    {
        esp_diag.error(REMOTELOG_SAVE_CFG_HEAP_EXHAUSTED);
        ERROR("remote_log_save_cfg heap exausted %d", file_len);
        return;
    }

    // "{"enabled": ,"host": "","port": ,"path": ""}",
    fs_sprintf(buffer.ref,
               "{\"enabled\": %d,\"host\": \"%s\",\"port\": %d,\"path\": \"%s\"}",
               remote_log_vars.enabled,
               remote_log_vars.host,
               remote_log_vars.port,
               remote_log_vars.path);
    cfgfile.n_append(buffer.ref, os_strlen(buffer.ref));
    espmem.stack_mon();
}

void set_remote_log(bool enabled, char *host, int port, char *path)
{
    remote_log_vars.enabled = enabled;
    if (remote_log_vars.host)
        delete[] remote_log_vars.host;
    remote_log_vars.host = new char[os_strlen(host) + 1];
    os_strcpy(remote_log_vars.host, host);
    remote_log_vars.port = port;
    if (remote_log_vars.path)
        delete[] remote_log_vars.path;
    remote_log_vars.path = new char[os_strlen(path) + 1];
    os_strcpy(remote_log_vars.path, path);
    save_cfg();
}

bool get_remote_log_enabled(void)
{
    return remote_log_vars.enabled;
}

char *get_remote_log_host(void)
{
    return remote_log_vars.host;
}

int get_remote_log_port(void)
{
    return remote_log_vars.port;
}

char *get_remote_log_path(void)
{
    return remote_log_vars.path;
}

//
// event log mngmt
//

static struct activity_event events[REMOTE_LOG_LENGTH];
static bool event_sent[REMOTE_LOG_LENGTH];
static int last_event_idx;

void print_last_events(void);

static Webclnt *espclient;

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

    if (!restore_cfg())
    {
        remote_log_vars.enabled = false;
        remote_log_vars.host = (char *)f_str("");
        remote_log_vars.port = 0;
        remote_log_vars.path = (char *)f_str("");
        esp_diag.info(REMOTELOG_INIT_DEFAULT_CFG);
        INFO("init_remote_logger no cfg available");
    }

    espclient = new Webclnt;
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
        remote_log_vars.enabled &&
        !overwriting_unreported_events)
    {
        overwriting_unreported_events = true;
        esp_diag.error(REMOTELOG_OVERWRITING_UNREPORTED_EVENTS);
        ERROR("log_event overwriting not yet reported events");
    }
    // clear error when no longer overwriting reported events
    // but just on the first occurrence
    if ((event_sent[last_event_idx] == true) &&
        remote_log_vars.enabled &&
        overwriting_unreported_events)
    {
        overwriting_unreported_events = false;
        esp_diag.info(REMOTELOG_NO_MORE_OVERWRITING_UNREPORTED_EVENTS);
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
            esp_diag.info(REMOTELOG_SERVER_AVAILABLE, comm_error_counter);
        }
        comm_error_counter = 0;
        break;
    case host_not_reachable:
        if (comm_error_counter == 0)
            esp_diag.info(REMOTELOG_SERVER_NOT_AVAILABLE, espclient->get_status());
        comm_error_counter++;
        INFO("remote_log_post_info unexpected webclient status %d", espclient->get_status());
        espclient->disconnect(NULL, NULL);
        break;
    case host_bad_answer:
        esp_diag.error(REMOTELOG_CHECK_ANSWER_UNEXPECTED_HOST_ANSWER, espclient->parsed_response->http_code);
        ERROR("remote_log_check_answer unexpected host answer %d, %s", espclient->parsed_response->http_code, espclient->parsed_response->body);
        espclient->disconnect(NULL, NULL);
        break;
    case host_req_error:
        esp_diag.info(REMOTELOG_SERVER_DIDNT_ANSWER, espclient->get_status());
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
    case WEBCLNT_RESPONSE_READY:
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
    case WEBCLNT_CONNECTED:
    case WEBCLNT_RESPONSE_READY:
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
        int msg_len = 179 + os_strlen(remote_log_vars.path);
        Heap_chunk msg(msg_len);
        if (msg.ref == NULL)
        {
            esp_diag.error(REMOTELOG_POST_INFO_HEAP_EXHAUSTED, msg_len);
            ERROR("remote_log_post_info - heap exausted [%d]", msg_len);
            espclient->disconnect(NULL, NULL);
            break;
        }
        fs_sprintf(msg.ref,
                   "POST %s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Content-Type: application/json\r\n",
                   remote_log_vars.path,
                   remote_log_vars.host);
        fs_sprintf((msg.ref + os_strlen(msg.ref)),
                   "Accept: */*\r\n"
                   "Connection: keep-alive\r\n"
                   "Content-Length: %d\r\n\r\n"
                   "%s\r\n",
                   os_strlen(event_str),
                   event_str);
        TRACE("remote_log_post_info POSTing str: %s", msg.ref);
        TRACE("remote_log_post_info event %d was sent", event_idx);
        espclient->send_req(msg.ref, os_strlen(msg.ref), check_answer, (void *)event_idx);
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

    if (!remote_log_vars.enabled)
        return;
    if (!Wifi::is_connected())
        return;
    struct ip_addr host_ip;
    atoipaddr(&host_ip, remote_log_vars.host);

    int event_idx = get_next_event_to_be_sent();
    TRACE("send_events_to_external_host event to be sent: %d", event_idx);
    if (event_idx < 0)
        // there are no unsent events
        return;
    espclient->connect(host_ip, remote_log_vars.port, post_info, (void *)event_idx);
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