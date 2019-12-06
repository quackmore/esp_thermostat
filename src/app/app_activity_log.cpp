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
#include "espbot_config.hpp"
#include "espbot_global.hpp"
#include "espbot_gpio.hpp"
#include "espbot_utils.hpp"
#include "espbot_webclient.hpp"
#include "app_activity_log.hpp"

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

static char *filename = "remote_log.cfg";

static bool restore_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return false;
    }
    File_to_json cfgfile(filename);
    if (cfgfile.exists())
    {
        // "{"enabled": ,"host": "","port": ,"path": ""}",
        // enabled
        if (cfgfile.find_string("enabled"))
        {
            esplog.error("%s - cannot find \"enabled\"\n", __FUNCTION__);
            return false;
        }
        remote_log_vars.enabled = atoi(cfgfile.get_value());
        // host
        if (cfgfile.find_string("host"))
        {
            esplog.error("%s - cannot find \"host\"\n", __FUNCTION__);
            return false;
        }
        if (remote_log_vars.host)
            delete[] remote_log_vars.host;
        remote_log_vars.host = new char[os_strlen(cfgfile.get_value()) + 1];
        os_strcpy(remote_log_vars.host, cfgfile.get_value());
        // port
        if (cfgfile.find_string("port"))
        {
            esplog.error("%s - cannot find \"port\"\n", __FUNCTION__);
            return false;
        }
        remote_log_vars.port = atoi(cfgfile.get_value());
        // path
        if (cfgfile.find_string("path"))
        {
            esplog.error("%s - cannot find \"path\"\n", __FUNCTION__);
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
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return true;
    }
    File_to_json cfgfile(filename);
    if (cfgfile.exists())
    {
        // "{"enabled": ,"host": "","port": ,"path": ""}",
        // enabled
        if (cfgfile.find_string("enabled"))
        {
            esplog.error("%s - cannot find \"enabled\"\n", __FUNCTION__);
            return true;
        }
        if (remote_log_vars.enabled != atoi(cfgfile.get_value()))
            return true;
        // host
        if (cfgfile.find_string("host"))
        {
            esplog.error("%s - cannot find \"host\"\n", __FUNCTION__);
            return true;
        }
        if (0 != os_strcmp(remote_log_vars.host, cfgfile.get_value()))
            return true;
        // port
        if (cfgfile.find_string("port"))
        {
            esplog.error("%s - cannot find \"port\"\n", __FUNCTION__);
            return true;
        }
        if (remote_log_vars.port != atoi(cfgfile.get_value()))
            return true;
        // path
        if (cfgfile.find_string("path"))
        {
            esplog.error("%s - cannot find \"path\"\n", __FUNCTION__);
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
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    if (Ffile::exists(&espfs, filename))
    {
        Ffile cfgfile(&espfs, filename);
        cfgfile.remove();
    }
}

static void save_cfg(void)
{
    esplog.all("%s\n", __FUNCTION__);
    if (saved_cfg_not_updated())
        remove_cfg();
    else
        return;
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    Ffile cfgfile(&espfs, filename);
    if (!cfgfile.is_available())
    {
        esplog.error("%s - cannot open %s\n", __FUNCTION__, filename);
        return;
    }
    int file_len = 44 + 1 + os_strlen(remote_log_vars.host) + 5 + os_strlen(remote_log_vars.path) + 1;
    Heap_chunk buffer(file_len);
    if (buffer.ref == NULL)
    {
        esplog.error("%s - not enough heap memory available (%d)\n", __FUNCTION__, file_len);
        return;
    }

    // "{"enabled": ,"host": "","port": ,"path": ""}",
    os_sprintf(buffer.ref,
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

static struct activity_event events[ACTIVITY_LOG_LENGTH];
static bool event_sent[ACTIVITY_LOG_LENGTH];
static int last_event_idx;

void print_last_events(void);

static Webclnt *espclient;

void init_activity_logger(void)
{
    esplog.all("%s\n", __FUNCTION__);
    int idx;
    for (idx = 0; idx < ACTIVITY_LOG_LENGTH; idx++)
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
        remote_log_vars.host = NULL;
        remote_log_vars.port = 0;
        remote_log_vars.path = NULL;
    }

    espclient = new Webclnt;
}

void log_event(uint32 timestamp, activity_event_t type, int value)
{
    esplog.all("%s\n", __FUNCTION__);
    last_event_idx++;
    if (last_event_idx >= ACTIVITY_LOG_LENGTH)
        last_event_idx = 0;
    events[last_event_idx].timestamp = timestamp;
    events[last_event_idx].type = type;
    events[last_event_idx].value = value;
    event_sent[last_event_idx] = false;
    // print_last_events();
}

int events_count(void)
{
    esplog.all("%s\n", __FUNCTION__);
    int count = 0;
    int idx = last_event_idx;
    while (events[idx].type != none)
    {
        count++;
        // move to previous event
        idx--;
        // check if event array boundary have been reached
        if (idx < 0)
            idx = ACTIVITY_LOG_LENGTH - 1;
        // check if it's back to last recorded event
        if (idx == last_event_idx)
            break;
    }
    return count;
}

struct activity_event *get_event(int idx)
{
    esplog.all("%s\n", __FUNCTION__);
    int tmp_index = last_event_idx;
    int ii;
    idx %= ACTIVITY_LOG_LENGTH;
    for (ii = 0; ii < idx; ii++)
    {
        tmp_index--;
        if (tmp_index < 0)
            tmp_index = ACTIVITY_LOG_LENGTH - 1;
    }
    return &events[tmp_index];
}

//
// Logging activity to external host
//

os_timer_t delay_post;

static int get_next_event_to_be_sent(void)
{
    esplog.all("%s\n", __FUNCTION__);
    int event_idx = last_event_idx;
    while (event_sent[event_idx])
    {
        event_idx -= 1;
        if (event_idx < 0)
            event_idx = ACTIVITY_LOG_LENGTH - 1;
        if (event_idx == last_event_idx)
            return -1;
    }
    return event_idx;
}

void post_info(void *param);

void check_answer(void *param)
{
    esplog.all("%s\n", __FUNCTION__);
    switch (espclient->get_status())
    {
    case WEBCLNT_RESPONSE_READY:
        espclient->update_status(WEBCLNT_CONNECTED);
        if (espclient->parsed_response->http_code != HTTP_OK)
        {
            // POST failed
            esplog.error("POST failed (host answered %d, %s)\n", espclient->parsed_response->http_code, espclient->parsed_response->body);
        }
        else
        {
            // POST succeeded
            // mark event as sent
            int event_idx = (int)param;
            event_sent[event_idx] = true;
            esplog.trace("%s: marking event %d as sent\n", __FUNCTION__, event_idx);
            // check if there are any unsent events
            event_idx = get_next_event_to_be_sent();
            esplog.trace("%s: next event to be sent: %d\n", __FUNCTION__, event_idx);
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
        esplog.error("%s - Ops ... webclient status is %d\n", __FUNCTION__, espclient->get_status());
        espclient->disconnect(NULL, NULL);
        break;
    }
}

void post_info(void *param)
{
    esplog.all("%s\n", __FUNCTION__);
    switch (espclient->get_status())
    {
    case WEBCLNT_CONNECTED:
    {
        int event_idx = (int)param;

        // {"timestamp":4294967295,"type":1,"value":1234}
        char event_str[47];
        os_sprintf(event_str,
                   "{\"timestamp\":%d,\"type\":%d,\"value\":%d}",
                   events[event_idx].timestamp,
                   events[event_idx].type,
                   events[event_idx].value);
        esplog.trace("%s: event str: %s\n", __FUNCTION__, event_str);

        // "POST  HTTP/1.1rnHost: 111.111.111.111rnContent-Type: application/jsonrnAccept: */*rnConnection: keep-alivernContent-Length: 47rnrn{"timestamp":4294967295,"type":1,"value":1234}rn"
        char *msg = new char[178 + os_strlen(remote_log_vars.path)];
        os_sprintf(msg,
                   "POST %s HTTP/1.1\r\n"
                   "Host: %s\r\n"
                   "Content-Type: application/json\r\n"
                   "Accept: */*\r\n"
                   "Connection: keep-alive\r\n"
                   "Content-Length: %d\r\n\r\n"
                   "%s\r\n",
                   remote_log_vars.path,
                   remote_log_vars.host,
                   os_strlen(event_str),
                   event_str);
        esplog.trace("%s: POSTing str: %s\n", __FUNCTION__, msg);
        esplog.trace("%s: event %d was sent\n", __FUNCTION__, event_idx);
        espclient->send_req(msg, check_answer, (void *)event_idx);
        delete[] msg;
    }
    break;
    default:
    {
        esplog.error("%s - Ops ... webclient status is %d\n", __FUNCTION__, espclient->get_status());
        espclient->disconnect(NULL, NULL);
    }
    break;
    }
}

void send_events_to_external_host(void)
{
    esplog.all("%s\n", __FUNCTION__);

    if (remote_log_vars.enabled)
    {
        struct ip_addr host_ip;
        atoipaddr(&host_ip, remote_log_vars.host);

        int event_idx = get_next_event_to_be_sent();
        esplog.trace("%s: event to be sent: %d\n", __FUNCTION__, event_idx);
        if (event_idx < 0)
            // there are no unsent events
            return;
        espclient->connect(host_ip, remote_log_vars.port, post_info, (void *)event_idx);
    }
}

void print_last_events(void)
{
    // this is just for debug, never called
    esplog.all("%s\n", __FUNCTION__);
    int idx;
    struct activity_event *event_ptr;
    os_printf("EVENTS BEGIN\n");
    for (idx = 0; idx < 10; idx++)
    {
        event_ptr = get_event(idx);
        os_printf("%d - %d - %d - %d - [%d] - (%d)\n",
                  idx,
                  event_ptr->timestamp,
                  event_ptr->type,
                  event_ptr->value,
                  ((event_ptr - &events[0]) / sizeof(struct activity_event)),
                  event_sent[idx]);
    }
    os_printf("EVENTS END\n");
}