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
#include "osapi.h"
#include "user_interface.h"
#include "mem.h"
#include "ip_addr.h"
}

#include "espbot.hpp"
#include "espbot_cfgfile.hpp"
#include "espbot_cron.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_http.hpp"
#include "espbot_http_routes.hpp"
#include "espbot_http_server.hpp"
#include "espbot_json.hpp"
#include "espbot_mem_mon.hpp"
#include "espbot_utils.hpp"
#include "drivers.hpp"
#include "app.hpp"
#include "app_remote_log.hpp"
#include "app_event_codes.h"
#include "app_heater.hpp"
#include "app_http_routes.hpp"
#include "app_remote_log.hpp"
#include "app_temp_control.hpp"
#include "app_temp_ctrl_program.hpp"
#include "app_temp_log.hpp"

static void get_api_info(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("get_api_info");
    char *msg = app_info_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void getCtrlVars(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlVars");
    char *msg = ctrl_vars_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void getCtrlSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlSettings");
    char *msg = ctrl_settings_full_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void setCtrlSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("setCtrlSettings");
    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    program_id: int,        2 digits
    //    powrr_off_timer: int    4 digits
    //  }
    //
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    int ctrl_mode = settings.getInt(f_str("ctrl_mode"));
    int manual_pulse_on = settings.getInt(f_str("manual_pulse_on"));
    int manual_pulse_off = settings.getInt(f_str("manual_pulse_off"));
    int auto_setpoint = settings.getInt(f_str("auto_setpoint"));
    int program_id = settings.getInt(f_str("program_id"));
    int power_off_timer = settings.getInt(f_str("power_off_timer"));
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    int res = CFG_ok;
    switch (ctrl_mode)
    {
    case MODE_OFF:
        res = ctrl_off();
        break;
    case MODE_MANUAL:
        res = ctrl_manual(manual_pulse_on, manual_pulse_off, power_off_timer);
        break;
    case MODE_AUTO:
        res = ctrl_auto(auto_setpoint, power_off_timer);
        break;
    case MODE_PROGRAM:
        res = ctrl_program(program_id);
        break;
    default:
        break;
    }
    if (res != CFG_ok)
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Cannot save to flash"), false);
    else
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
}

static void getCtrlAdvSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlAdvSettings");
    char *msg = adv_settings_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void setCtrlAdvSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("setCtrlAdvSettings");
    // {
    //   kp: int,             6 digit (5 digit and sign)
    //   kd: int,             6 digit (5 digit and sign)
    //   ki: int,             6 digit (5 digit and sign)
    //   kd_dt: int,          6 digit
    //   u_max: int,          6 digit (5 digit and sign)
    //   heater_on_min: int,  5 digit
    //   heater_on_max: int,  5 digit
    //   heater_on_off: int,  5 digit
    //   heater_cold: int,    5 digit
    //   warm_up_period: int, 5 digit
    //   wup_heater_on: int,  5 digit
    //   wup_heater_off: int  5 digit
    // }
    //
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    struct _adv_ctrl_settings adv_ctrl_settings;
    adv_ctrl_settings.kp = settings.getInt(f_str("kp"));
    adv_ctrl_settings.kd = settings.getInt(f_str("kd"));
    adv_ctrl_settings.ki = settings.getInt(f_str("ki"));
    adv_ctrl_settings.kd_dt = settings.getInt(f_str("kd_dt"));
    adv_ctrl_settings.u_max = settings.getInt(f_str("u_max"));
    adv_ctrl_settings.heater_on_min = settings.getInt(f_str("heater_on_min"));
    adv_ctrl_settings.heater_on_max = settings.getInt(f_str("heater_on_max"));
    adv_ctrl_settings.heater_on_off = settings.getInt(f_str("heater_on_off"));
    adv_ctrl_settings.heater_cold = settings.getInt(f_str("heater_cold"));
    adv_ctrl_settings.warm_up_period = settings.getInt(f_str("warm_up_period"));
    adv_ctrl_settings.wup_heater_on = settings.getInt(f_str("wup_heater_on"));
    adv_ctrl_settings.wup_heater_off = settings.getInt(f_str("wup_heater_off"));
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    int res = set_adv_ctrl_settings(&adv_ctrl_settings);
    if (res != CFG_ok)
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Cannot save to flash"), false);
    else
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
    mem_mon_stack();
}

static void getReadingCal(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getReadingCal");
    char *msg = temp_log_cfg_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void setReadingCal(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("setReadingCal");
    // {
    //   temp_cal_offset:  int         5 digits
    //   humi_cal_offset": int         5 digits
    // }
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    int temp_cal_offset = settings.getInt(f_str("temp_cal_offset"));
    int humi_cal_offset = settings.getInt(f_str("humi_cal_offset"));
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    // input validation
    if ((temp_cal_offset < -1000) ||
        (temp_cal_offset > 1000) ||
        (humi_cal_offset < -1000) ||
        (humi_cal_offset > 1000))
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Calibration Offsets out of range"), false);
        return;
    }
    int res = set_cal_offset(temp_cal_offset, humi_cal_offset);
    if (res != CFG_ok)
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Cannot save to flash"), false);
    else
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
    mem_mon_stack();
}

static void getRemoteLog(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getRemoteLog");
    char *msg = remote_log_cfg_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void setRemoteLog(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("setRemoteLog");
    //  {
    //    enabled: int,   1 digits
    //    host: string,   ...
    //    port: int,      5 digits
    //    path: string    ...
    //  }
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    bool enabled = (bool)settings.getInt(f_str("enabled"));
    char host[16];
    os_memset(host, 0, 16);
    settings.getStr(f_str("host"), host, 16);
    int port = settings.getInt(f_str("port"));
    char path[128];
    os_memset(path, 0, 128);
    settings.getStr(f_str("path"), path, 128);
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    int res = set_remote_log(enabled, host, port, path);
    if (res != CFG_ok)
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Cannot save to flash"), false);
    else
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
}

static void getCtrlEvents_next(struct http_split_send *p_sr)
{
    ALL("getctrlevents_next");
    if (!http_espconn_in_use(p_sr->p_espconn))
    {
        TRACE("getctrlevents_next espconn %X state %d, abort", p_sr->p_espconn, p_sr->p_espconn->state);
        // there will be no send, so trigger a check of pending send
        system_os_post(USER_TASK_PRIO_0, SIG_http_checkPendingResponse, '0');
        return;
    }
    int remaining_size = (p_sr->content_size - p_sr->content_transferred) * 36 + 3;
    if (remaining_size > get_http_msg_max_size())
    {
        // the remaining content size is bigger than response_max_size
        // will split the remaining content over multiple messages
        int buffer_size = get_http_msg_max_size();
        char *buffer = new char[buffer_size];
        if (buffer == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_NEXT_HEAP_EXHAUSTED, buffer_size);
            ERROR("getctrlevents_next heap exhausted %d", buffer_size);
            http_response(p_sr->p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            return;
        }
        struct http_split_send *p_pending_response = new struct http_split_send;
        if (p_pending_response == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_NEXT_HEAP_EXHAUSTED, sizeof(struct http_split_send));
            ERROR("getctrlevents_next not heap exhausted %dn", sizeof(struct http_split_send));
            http_response(p_sr->p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            delete[] buffer;
            return;
        }
        struct activity_event *ev_i;
        int ev_count = buffer_size / 36 + p_sr->content_transferred;
        int idx;
        for (idx = p_sr->content_transferred; idx < ev_count; idx++)
        {
            fs_sprintf(buffer + os_strlen(buffer), ",");
            ev_i = get_event(idx);
            if (ev_i)
                fs_sprintf(buffer + os_strlen(buffer),
                           "{\"ts\":%u,\"tp\":%d,\"vl\":%d}",
                           ev_i->timestamp,
                           ev_i->type,
                           ev_i->value);
        }
        // setup the remaining message
        p_pending_response->p_espconn = p_sr->p_espconn;
        p_pending_response->order = p_sr->order + 1;
        p_pending_response->content = p_sr->content;
        p_pending_response->content_size = p_sr->content_size;
        p_pending_response->content_transferred = ev_count;
        p_pending_response->action_function = getCtrlEvents_next;
        Queue_err result = pending_split_send->push(p_pending_response);
        if (result == Queue_full)
        {
            delete[] buffer;
            delete p_pending_response;
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_NEXT_PENDING_RES_QUEUE_FULL);
            ERROR("getctrlevents_next full pending res queue");
            return;
        }
        TRACE("getctrlevents_next: *p_espconn: %X, msg (splitted) len: %d",
              p_sr->p_espconn, buffer_size);
        http_send_buffer(p_sr->p_espconn, p_sr->order, buffer, os_strlen(buffer));
    }
    else
    {
        // this is the last piece of the message
        char *buffer = new char[remaining_size];
        if (buffer == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_NEXT_HEAP_EXHAUSTED, remaining_size);
            ERROR("getctrlevents_next heap exhausted %d", remaining_size);
            http_response(p_sr->p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            return;
        }
        struct activity_event *ev_i;
        int idx;
        for (idx = p_sr->content_transferred; idx < p_sr->content_size; idx++)
        {
            fs_sprintf(buffer + os_strlen(buffer), ",");
            ev_i = get_event(idx);
            if (ev_i)
                fs_sprintf(buffer + os_strlen(buffer),
                           "{\"ts\":%u,\"tp\":%d,\"vl\":%d}",
                           ev_i->timestamp,
                           ev_i->type,
                           ev_i->value);
        }
        fs_sprintf(buffer + os_strlen(buffer), "]}");
        TRACE("getctrlevents_next: *p_espconn: %X, msg (splitted) len: %d",
              p_sr->p_espconn, remaining_size);
        http_send_buffer(p_sr->p_espconn, p_sr->order, buffer, os_strlen(buffer));
    }
}

void getCtrlEvents_first(struct espconn *p_espconn, Http_parsed_req *parsed_req)
{
    // {"ctrl_events":[]}
    // {"ts":4294967295,"tp":1,"vl":-1234},
    ALL("getCtrlEvents_first");
    // let's start with the header
    Http_header header;
    header.m_code = HTTP_OK;
    header.m_content_type = HTTP_CONTENT_JSON;
    int ev_num = events_count();
    // calculate the effective content_len
    int content_len = 18;
    {
        int idx;
        char buffer[40];
        struct activity_event *ev_i;
        for (idx = 0; idx < ev_num; idx++)
        {
            ev_i = get_event(idx);
            if (ev_i)
                fs_sprintf(buffer,
                           "{\"ts\":%u,\"tp\":%d,\"vl\":%d},",
                           ev_i->timestamp,
                           ev_i->type,
                           ev_i->value);
            content_len += os_strlen(buffer);
        }
        // when events array is not empty
        // subtract 1 comma (last array element)
        if (content_len > 18)
            content_len -= 1;
    }
    header.m_content_length = content_len;
    header.m_content_range_start = 0;
    header.m_content_range_end = 0;
    header.m_content_range_total = 0;
    if (parsed_req->origin)
    {
        header.m_origin = new char[(os_strlen(parsed_req->origin) + 1)];
        if (header.m_origin == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_FIRST_HEAP_EXHAUSTED, (os_strlen(parsed_req->origin) + 1));
            ERROR("getctrlevents_first heap exhausted %d", (os_strlen(parsed_req->origin) + 1));
            http_response(p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            return;
        }
        os_strcpy(header.m_origin, parsed_req->origin);
    }
    char *header_str = http_format_header(&header);
    if (header_str == NULL)
    {
        dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_FIRST_HEAP_EXHAUSTED, (os_strlen(header_str)));
        ERROR("getctrlevents_first heap exhausted %d", (os_strlen(header_str)));
        http_response(p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    // ok send the header
    http_send_buffer(p_espconn, 0, header_str, os_strlen(header_str));

    // and now the content
    if (content_len > get_http_msg_max_size())
    {
        // will split the content over multiple messages
        // each the size of http_msg_max_size
        int buffer_size = get_http_msg_max_size();
        char *buffer = new char[buffer_size];
        if (buffer == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_FIRST_HEAP_EXHAUSTED, buffer_size);
            ERROR("getctrlevents_first heap exhausted %d", buffer_size);
            http_response(p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            return;
        }
        struct http_split_send *p_pending_response = new struct http_split_send;
        if (p_pending_response == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_FIRST_HEAP_EXHAUSTED, sizeof(struct http_split_send));
            ERROR("getctrlevents_first heap exhausted %d", sizeof(struct http_split_send));
            http_response(p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            delete[] buffer;
            return;
        }
        fs_sprintf(buffer, "{\"ctrl_events\":[");
        bool first_time = true;
        struct activity_event *ev_i;
        int ev_count = (buffer_size - 18) / 36;
        int idx;
        for (idx = 0; idx < ev_count; idx++)
        {
            if (first_time)
                first_time = false;
            else
                fs_sprintf(buffer + os_strlen(buffer), ",");
            ev_i = get_event(idx);
            if (ev_i)
                fs_sprintf(buffer + os_strlen(buffer),
                           "{\"ts\":%u,\"tp\":%d,\"vl\":%d}",
                           ev_i->timestamp,
                           ev_i->type,
                           ev_i->value);
        }
        // setup the next message
        p_pending_response->p_espconn = p_espconn;
        p_pending_response->order = 2;
        p_pending_response->content = "";
        p_pending_response->content_size = ev_num;
        p_pending_response->content_transferred = ev_count;
        p_pending_response->action_function = getCtrlEvents_next;
        Queue_err result = pending_split_send->push(p_pending_response);
        if (result == Queue_full)
        {
            delete[] buffer;
            delete p_pending_response;
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_FIRST_PENDING_RES_QUEUE_FULL);
            ERROR("getctrlevents_first full pending response queue");
            return;
        }
        // send the content fragment
        TRACE("getctrlevents_first *p_espconn: %X, msg (splitted) len: %d", p_espconn, buffer_size);
        http_send_buffer(p_espconn, 1, buffer, os_strlen(buffer));
        mem_mon_stack();
    }
    else
    {
        // no need to split the content over multiple messages
        char *buffer = new char[content_len + 1];
        if (buffer == NULL)
        {
            dia_error_evnt(APP_ROUTES_GETCTRLEVENTS_FIRST_HEAP_EXHAUSTED, content_len);
            ERROR("getctrlevents_first heap exhausted %d", content_len);
            http_response(p_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
            return;
        }
        fs_sprintf(buffer, "{\"ctrl_events\":[");
        struct activity_event *ev_i;
        bool first_time = true;
        int idx;
        for (idx = 0; idx < ev_num; idx++)
        {
            if (first_time)
                first_time = false;
            else
                fs_sprintf(buffer + os_strlen(buffer), ",");
            ev_i = get_event(idx);
            if (ev_i)
                fs_sprintf(buffer + os_strlen(buffer),
                           "{\"ts\":%u,\"tp\":%d,\"vl\":%d}",
                           ev_i->timestamp,
                           ev_i->type,
                           ev_i->value);
        }
        fs_sprintf(buffer + os_strlen(buffer), "]}");
        TRACE("getctrlevents_first *p_espconn: %X, msg (full) len: %d", p_espconn, content_len);
        TRACE("getctrlevents_first msg: %s", buffer);
        http_send_buffer(p_espconn, 1, buffer, os_strlen(buffer));
    }
}

static void getCtrlPause(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlPause");
    char *msg = ctrl_paused_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void setCtrlPause(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("setCtrlPause");
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    bool ctrl_paused = (bool)settings.getInt(f_str("ctrl_paused"));
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    set_ctrl_paused(ctrl_paused);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
}

static void getProgramList(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getProgramList");
    char *msg = prg_list_json_stringify();
    if (msg)
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
    else
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    mem_mon_stack();
}

static void getProgram(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getProgram");
    char *str_program_id = parsed_req->url + os_strlen(f_str("/api/ctrl/program/"));
    if (os_strlen(str_program_id) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No program id provided"), false);
        return;
    }
    int program_id = atoi(str_program_id);
    struct prgm *program = NULL;
    int res = load_program(program_id, program);
    // will need to free the memory allocated by load_program
    if (res == ERR_PRG_NOT_FOUND)
        http_response(ptr_espconn, HTTP_NOT_FOUND, HTTP_CONTENT_JSON, f_str("Program not found"), false);
    else if (res == ERR_MEM_EXHAUSTED)
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
    else if (res == ERR_PRG_BAD_SYNTAX)
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Program bad syntax"), false);
    else
    {
        char *msg = program_json_stringify(program);
        if (msg)
            http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg, true);
        else
            http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
    }
    delete_program(program);
    mem_mon_stack();
}

static void deleteProgram(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("deleteProgram");
    char *str_program_id = parsed_req->url + os_strlen(f_str("/api/ctrl/program/"));
    if (os_strlen(str_program_id) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No program id provided"), false);
        return;
    }
    int program_id = atoi(str_program_id);
    int result = del_program(program_id);
    switch (result)
    {
    case ERR_PRG_NOT_FOUND:
        http_response(ptr_espconn, HTTP_NOT_FOUND, HTTP_CONTENT_JSON, f_str("Program not found"), false);
        break;
    default:
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Program deleted\"}"), false);
    }
}

static void createProgram(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("createProgram");
    // {
    //     "name" : "program_name",
    //     "min_temp" : 100,
    //     "periods" : [
    //         {
    //             "wd" : 8,
    //             "b" : 100,
    //             "e" : 200,
    //             "sp" : 200
    //         },
    //         {
    //             "wd" : 8,
    //             "b" : 200,
    //             "e" : 400,
    //             "sp" : 200
    //         }
    //     ]
    // }
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    char name[32];
    os_memset(name, 0, 32);
    settings.getStr(f_str("name"), name, 32);
    int min_temp = settings.getInt(f_str("min_temp"));
    JSONP_ARRAY periods = settings.getArray(f_str("periods"));
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    if (periods.len() > MAX_PROGRAM_PERIODS)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Too many 'periods'"), false);
        return;
    }
    struct prgm program;
    program.min_temp = min_temp;
    program.period_count = periods.len();
    if (program.period_count > 0)
    {
        program.periods = (struct prgm_period *)new struct prgm_period[program.period_count];
        if (program.periods == NULL)
        {
            dia_error_evnt(APP_ROUTES_CREATEPROGRAM_HEAP_EXHAUSTED, (sizeof(struct prgm_period) * program.period_count));
            ERROR("createProgram heap exhausted %d", (sizeof(struct prgm_period) * program.period_count));
            http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
            return;
        }
        int idx;
        for (idx = 0; idx < program.period_count; idx++)
        {
            JSONP period = periods.getObj(idx);
            // {
            //     "wd" : 8,
            //     "b" : 100,
            //     "e" : 200,
            //     "sp" : 200
            // }
            week_days wd = (week_days)period.getInt(f_str("wd"));
            int b = period.getInt(f_str("b"));
            int e = period.getInt(f_str("e"));
            int sp = period.getInt(f_str("sp"));
            if (period.getErr() != JSON_noerr)
            {
                http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
                delete program.periods;
                return;
            }
            program.periods[idx].day_of_week = wd;
            program.periods[idx].mm_start = b;
            program.periods[idx].mm_end = e;
            program.periods[idx].setpoint = sp;
        }
    }
    else
    {
        program.periods = NULL;
    }

    int result = add_program(name, &program);
    if (program.periods)
        delete program.periods;
    switch (result)
    {
    case MAX_PRG_COUNT_REACHED:
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Cannot add more programs"), false);
        break;
    case ERR_SAVING_PRG:
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Error creating program"), false);
        break;
    case ERR_MEM_EXHAUSTED:
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Not enough heap memory"), false);
        break;
    default:
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Program created\"}"), false);
    }
    mem_mon_stack();
}

static void updateProgram(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("updateProgram");
    char *str_program_id = parsed_req->url + os_strlen(f_str("/api/ctrl/program/"));
    if (os_strlen(str_program_id) == 0)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("No program id provided"), false);
        return;
    }
    int program_id = atoi(str_program_id);
    JSONP settings(parsed_req->req_content, parsed_req->content_len);
    char name[32];
    os_memset(name, 0, 32);
    settings.getStr(f_str("name"), name, 32);
    int min_temp = settings.getInt(f_str("min_temp"));
    JSONP_ARRAY periods = settings.getArray(f_str("periods"));
    if (settings.getErr() != JSON_noerr)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    if (periods.len() > MAX_PROGRAM_PERIODS)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Too many 'periods'"), false);
        return;
    }
    struct prgm program;
    program.min_temp = min_temp;
    program.period_count = periods.len();
    if (program.period_count > 0)
    {
        program.periods = (struct prgm_period *)new struct prgm_period[program.period_count];
        if (program.periods == NULL)
        {
            dia_error_evnt(APP_ROUTES_UPDATEPROGRAM_HEAP_EXHAUSTED, (sizeof(struct prgm_period) * program.period_count));
            ERROR("createProgram heap exhausted %d", (sizeof(struct prgm_period) * program.period_count));
            http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
            return;
        }
        int idx;
        for (idx = 0; idx < program.period_count; idx++)
        {
            JSONP period = periods.getObj(idx);
            // {
            //     "wd" : 8,
            //     "b" : 100,
            //     "e" : 200,
            //     "sp" : 200
            // }
            week_days wd = (week_days)period.getInt(f_str("wd"));
            int b = period.getInt(f_str("b"));
            int e = period.getInt(f_str("e"));
            int sp = period.getInt(f_str("sp"));
            if (period.getErr() != JSON_noerr)
            {
                http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
                delete program.periods;
                return;
            }
            program.periods[idx].day_of_week = wd;
            program.periods[idx].mm_start = b;
            program.periods[idx].mm_end = e;
            program.periods[idx].setpoint = sp;
        }
    }
    else
    {
        program.periods = NULL;
    }
    int result = mod_program(program_id, name, &program);
    if (program.periods)
        delete program.periods;
    switch (result)
    {
    case ERR_PRG_NOT_FOUND:
        http_response(ptr_espconn, HTTP_NOT_FOUND, HTTP_CONTENT_JSON, f_str("Program not found"), false);
        break;
    case ERR_SAVING_PRG:
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Error modifying program"), false);
        break;
    default:
        http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Program modified\"}"), false);
        // check if the running program was modified
        if ((get_current_mode() == MODE_PROGRAM) && (get_program_id() == program_id))
            ctrl_program(program_id);
    }
    mem_mon_stack();
}

bool app_http_routes(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("app_http_routes");

    if ((0 == os_strcmp(parsed_req->url, f_str("/api/info"))) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_info(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/advSettings"))) && (parsed_req->req_method == HTTP_GET))
    {
        getCtrlAdvSettings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/advSettings"))) && (parsed_req->req_method == HTTP_POST))
    {
        setCtrlAdvSettings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/log"))) && (parsed_req->req_method == HTTP_GET))
    {
        getCtrlEvents_first(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/pause"))) && (parsed_req->req_method == HTTP_GET))
    {
        getCtrlPause(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/pause"))) && (parsed_req->req_method == HTTP_POST))
    {
        setCtrlPause(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/program"))) && (parsed_req->req_method == HTTP_GET))
    {
        getProgramList(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/ctrl/program/"), os_strlen(f_str("/api/ctrl/program/")))) && (parsed_req->req_method == HTTP_GET))
    {
        getProgram(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/ctrl/program/"), os_strlen(f_str("/api/ctrl/program")))) && (parsed_req->req_method == HTTP_DELETE))
    {
        deleteProgram(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strncmp(parsed_req->url, f_str("/api/ctrl/program/"), os_strlen(f_str("/api/ctrl/program")))) && (parsed_req->req_method == HTTP_PUT))
    {
        updateProgram(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/program"))) && (parsed_req->req_method == HTTP_POST))
    {
        createProgram(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/reading_cal"))) && (parsed_req->req_method == HTTP_GET))
    {
        getReadingCal(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/reading_cal"))) && (parsed_req->req_method == HTTP_POST))
    {
        setReadingCal(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/remoteLog"))) && (parsed_req->req_method == HTTP_GET))
    {
        getRemoteLog(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/remoteLog"))) && (parsed_req->req_method == HTTP_POST))
    {
        setRemoteLog(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/settings"))) && (parsed_req->req_method == HTTP_GET))
    {
        getCtrlSettings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/settings"))) && (parsed_req->req_method == HTTP_POST))
    {
        setCtrlSettings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, f_str("/api/ctrl/vars"))) && (parsed_req->req_method == HTTP_GET))
    {
        getCtrlVars(ptr_espconn, parsed_req);
        return true;
    }
    return false;
}