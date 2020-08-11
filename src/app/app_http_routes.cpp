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
#include "espbot_cron.hpp"
#include "espbot_debug.hpp"
#include "espbot_global.hpp"
#include "espbot_http_routes.hpp"
#include "espbot_json.hpp"
#include "espbot_logger.hpp"
#include "espbot_utils.hpp"
#include "espbot_webserver.hpp"
#include "library.hpp"
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
    // {"device_name":"","chip_id":"","app_name":"","app_version":"","espbot_version":"","api_version":"","library_version":"","sdk_version":"","boot_version":""}
    ALL("get_api_info");
    int str_len = 155 +
                  os_strlen(espbot.get_name()) +
                  10 +
                  os_strlen(app_name) +
                  os_strlen(app_release) +
                  os_strlen(espbot.get_version()) +
                  os_strlen(f_str(API_RELEASE)) +
                  os_strlen(library_release) +
                  os_strlen(system_get_sdk_version()) +
                  10 +
                  1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GET_API_INFO_HEAP_EXHAUSTED, str_len);
        ERROR("get_api_info heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"device_name\":\"%s\",\"chip_id\":\"%d\",\"app_name\":\"%s\",",
               espbot.get_name(),
               system_get_chip_id(),
               app_name);
    fs_sprintf((msg.ref + os_strlen(msg.ref)),
               "\"app_version\":\"%s\",\"espbot_version\":\"%s\",",
               app_release,
               espbot.get_version());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"api_version\":\"%s\",\"library_version\":\"%s\",",
               f_str(API_RELEASE),
               library_release);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"sdk_version\":\"%s\",\"boot_version\":\"%d\"}",
               system_get_sdk_version(),
               system_get_boot_version());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void getCtrlVars(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlVars");
    // {
    //   "timestamp": uint32,                 11 digits
    //   "timezone": int                       3 digits
    //   "current_temp": int,                  5 digits
    //   "current_humi": int,                  5 digits
    //   "heater_status": int,                 1 digit
    //   "auto_setpoint": int,                 4 digits
    //   "ctrl_mode": int,                     1 digit
    //   "program_name": int,                 32 digit
    //   "pwr_off_timer_started_on": uint32,  11 digits
    //   "pwr_off_timer": int                  4 digits
    // }
    int str_len = 152 + 11 + 3 + 5 + 5 + 1 + 4 + 1 + 32 + 11 + 4 + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETCTRLVARS_HEAP_EXHAUSTED, str_len);
        ERROR("getCtrlVars heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
        return;
    }
    struct date *current_time = get_current_time();
    // {"timestamp":,"timezone":,"current_temp":,"heater_status":,"auto_setpoint":,"ctrl_mode":,"program_name":"","pwr_off_timer_started_on":,"pwr_off_timer":}
    fs_sprintf(msg.ref,
               "{\"timestamp\":%d,"
               "\"timezone\":%d,",
               (current_time->timestamp), // sending UTC time
               esp_time.get_timezone());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"current_temp\":%d,"
               "\"current_humi\":%d,",
               get_temp(0),
               get_humi(0));
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"heater_status\":%d,"
               "\"auto_setpoint\":%d,",
               (is_heater_on() ? 1 : 0),
               get_auto_setpoint());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"ctrl_mode\":%d,"
               "\"program_name\":\"%s\",",
               get_current_mode(),
               get_cur_program_name(get_program_id()));
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"pwr_off_timer_started_on\":%d,"
               "\"pwr_off_timer\":%d}",
               (get_pwr_off_timer_started_on()),
               get_pwr_off_timer());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void getCtrlSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlSettings");
    //  {"ctrl_mode": ,"manual_pulse_on": ,"manual_pulse_off": ,"auto_setpoint": ,"program_id": ,"program_name": "","pwr_off_timer": }

    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    program_id: int,        2 digits
    //    program_name: string,  32 digits
    //    pwr_off_timer: int      4 digits
    //  }
    int str_len = 126 + 1 + 4 + 4 + 4 + 2 + 32 + 4 + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETCTRLSETTINGS_HEAP_EXHAUSTED, str_len);
        ERROR("getCtrlSettings heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref,
               "{\"ctrl_mode\":%d,"
               "\"manual_pulse_on\":%d,"
               "\"manual_pulse_off\":%d,",
               get_current_mode(),
               get_manual_pulse_on(),
               get_manual_pulse_off());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"auto_setpoint\":%d,"
               "\"program_id\":%d,",
               get_auto_setpoint(),
               get_program_id());
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"program_name\":\"%s\","
               "\"pwr_off_timer\":%d}",
               get_cur_program_name(get_program_id()),
               get_pwr_off_timer());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
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
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    //    ctrl_mode: int,         1 digits
    int settings_ctrl_mode;
    if (settings.find_pair(f_str("ctrl_mode")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'ctrl_mode'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'ctrl_mode' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_ctrl_mode(settings.get_cur_pair_value_len());
    if (str_ctrl_mode.ref == NULL)
    {
        esp_diag.error(APP_SETCTRLSETTINGS_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setCtrlSettings heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_ctrl_mode.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_ctrl_mode = atoi(str_ctrl_mode.ref);

    // settings_manual_pulse_on
    int settings_manual_pulse_on;
    if (settings.find_pair(f_str("manual_pulse_on")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'manual_pulse_on'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'manual_pulse_on' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_manual_pulse_on(settings.get_cur_pair_value_len());
    if (str_manual_pulse_on.ref == NULL)
    {
        esp_diag.error(APP_SETCTRLSETTINGS_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setCtrlSettings heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_manual_pulse_on.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_manual_pulse_on = atoi(str_manual_pulse_on.ref);

    //    manual_pulse_off: int,  4 digits
    int settings_manual_pulse_off;
    if (settings.find_pair(f_str("manual_pulse_off")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'manual_pulse_off'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'manual_pulse_off' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_manual_pulse_off(settings.get_cur_pair_value_len());
    if (str_manual_pulse_off.ref == NULL)
    {
        esp_diag.error(APP_SETCTRLSETTINGS_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setCtrlSettings heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_manual_pulse_off.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_manual_pulse_off = atoi(str_manual_pulse_off.ref);

    //    auto_setpoint: int,     4 digits
    int settings_auto_setpoint;
    if (settings.find_pair(f_str("auto_setpoint")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'auto_setpoint'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'auto_setpoint' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_auto_setpoint(settings.get_cur_pair_value_len());
    if (str_auto_setpoint.ref == NULL)
    {
        esp_diag.error(APP_SETCTRLSETTINGS_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setCtrlSettings heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_auto_setpoint.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_auto_setpoint = atoi(str_auto_setpoint.ref);

    //    program_id: int,     2 digits
    int settings_program_id;
    if (settings.find_pair(f_str("program_id")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'program_id'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'program_id' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_program_id(settings.get_cur_pair_value_len());
    if (str_program_id.ref == NULL)
    {
        esp_diag.error(APP_SETCTRLSETTINGS_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setCtrlSettings heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_program_id.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_program_id = atoi(str_program_id.ref);

    //    pwr_off_timer: int      4 digits
    int settings_power_off_timer;
    if (settings.find_pair(f_str("power_off_timer")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'power_off_timer'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'power_off_timer' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_power_off_timer(settings.get_cur_pair_value_len());
    if (str_power_off_timer.ref == NULL)
    {
        esp_diag.error(APP_SETCTRLSETTINGS_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setCtrlSettings heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_power_off_timer.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_power_off_timer = atoi(str_power_off_timer.ref);

    switch (settings_ctrl_mode)
    {
    case MODE_OFF:
        ctrl_off();
        break;
    case MODE_MANUAL:
        ctrl_manual(settings_manual_pulse_on, settings_manual_pulse_off, settings_power_off_timer);
        break;
    case MODE_AUTO:
        ctrl_auto(settings_auto_setpoint, settings_power_off_timer);
        break;
    case MODE_PROGRAM:
        ctrl_program(settings_program_id);
        break;
    default:
        break;
    }
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
}

static void getCtrlAdvSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlAdvSettings");
    // {
    //   kp: int,             6 digit (5 digit and sign)
    //   kd: int,             6 digit (5 digit and sign)
    //   ki: int,             6 digit (5 digit and sign)
    //   u_max: int,          6 digit (5 digit and sign)
    //   heater_on_min: int,  5 digit
    //   heater_on_max: int,  5 digit
    //   heater_on_off: int,  5 digit
    //   heater_cold: int,    5 digit
    //   warm_up_period: int, 5 digit
    //   wup_heater_on: int,  5 digit
    //   wup_heater_off: int  5 digit
    // }
    // {"kp": ,"kd": ,"ki": ,"u_max": ,"heater_on_min": ,"heater_on_max": ,"heater_on_off": ,"heater_cold": ,"warm_up_period": ,"wup_heater_on": ,"wup_heater_off": }
    int str_len = 158 + 6 + 6 + 6 + 6 + 5 + 5 + 5 + 5 + 5 + 5 + 5 + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETCTRLADVSETTINGS_HEAP_EXHAUSTED, str_len);
        ERROR("getCtrlAdvSettings heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
        return;
    }
    struct _adv_ctrl_settings *adv_settings = get_adv_ctrl_settings();
    fs_sprintf(msg.ref,
               "{\"kp\": %d,"
               "\"kd\": %d,"
               "\"ki\": %d,"
               "\"u_max\": %d,"
               "\"heater_on_min\": %d,",
               adv_settings->kp,
               adv_settings->kd,
               adv_settings->ki,
               adv_settings->u_max,
               adv_settings->heater_on_min);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"heater_on_max\": %d,"
               "\"heater_on_off\": %d,"
               "\"heater_cold\": %d,",
               adv_settings->heater_on_max,
               adv_settings->heater_on_off,
               adv_settings->heater_cold);
    fs_sprintf(msg.ref + os_strlen(msg.ref),
               "\"warm_up_period\": %d,"
               "\"wup_heater_on\": %d,"
               "\"wup_heater_off\": %d}",
               adv_settings->warm_up_period,
               adv_settings->wup_heater_on,
               adv_settings->wup_heater_off);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void setCtrlAdvSettings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("setCtrlAdvSettings");
    // {
    //   kp: int,             6 digit (5 digit and sign)
    //   kd: int,             6 digit (5 digit and sign)
    //   ki: int,             6 digit (5 digit and sign)
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
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }

    struct _adv_ctrl_settings adv_ctrl_settings;
    char tmp_str[8];

    //   kp: int,             6 digit (5 digit and sign)
    if (settings.find_pair(f_str("kp")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'kp'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'kp' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.kp = atoi(tmp_str);

    //   kd: int,             6 digit (5 digit and sign)
    if (settings.find_pair(f_str("kd")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'kd'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'kd' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.kd = atoi(tmp_str);

    //   ki: int,             6 digit (5 digit and sign)
    if (settings.find_pair(f_str("ki")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'ki'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'ki' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.ki = atoi(tmp_str);

    //   u_max: int,          6 digit (5 digit and sign)
    if (settings.find_pair(f_str("u_max")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'u_max'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'u_max' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.u_max = atoi(tmp_str);

    //   heater_on_min: int,  5 digit
    if (settings.find_pair(f_str("heater_on_min")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'heater_on_min'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'heater_on_min' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_on_min = atoi(tmp_str);

    //   heater_on_max: int,  5 digit
    if (settings.find_pair(f_str("heater_on_max")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'heater_on_max'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'heater_on_max' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_on_max = atoi(tmp_str);

    //   heater_on_off: int,  5 digit
    if (settings.find_pair(f_str("heater_on_off")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'heater_on_off'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'heater_on_off' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_on_off = atoi(tmp_str);

    //   heater_cold: int,  5 digit
    if (settings.find_pair(f_str("heater_cold")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'heater_cold'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'heater_cold' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_cold = atoi(tmp_str);

    //   warm_up_period: int,  5 digit
    if (settings.find_pair(f_str("warm_up_period")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'warm_up_period'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'warm_up_period' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.warm_up_period = atoi(tmp_str);

    //   wup_heater_on: int,  5 digit
    if (settings.find_pair(f_str("wup_heater_on")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'wup_heater_on'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'wup_heater_on' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.wup_heater_on = atoi(tmp_str);

    //   wup_heater_off: int,  5 digit
    if (settings.find_pair(f_str("wup_heater_off")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'wup_heater_off'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'wup_heater_off' does not have an INTEGER value type"), false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("number bad format"), false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.wup_heater_off = atoi(tmp_str);

    set_adv_ctrl_settings(&adv_ctrl_settings);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
}

static void getRemoteLog(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getRemoteLog");
    //  {
    //    enabled: int,   1 digits
    //    host: string,   ...
    //    port: int,      5 digits
    //    path: string    ...
    //  }
    int str_len = 44 + 1 + os_strlen(get_remote_log_host()) + 5 + os_strlen(get_remote_log_path()) + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETREMOTELOG_HEAP_EXHAUSTED, str_len);
        ERROR("getRemoteLog heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted!"), false);
        return;
    }
    // "{"enabled": ,"host": "","port": ,"path": ""}",
    fs_sprintf(msg.ref,
               "{\"enabled\": %d,\"host\": \"%s\",\"port\": %d,\"path\": \"%s\"}",
               get_remote_log_enabled(),
               get_remote_log_host(),
               get_remote_log_port(),
               get_remote_log_path());

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
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
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    //    enabled: int,         1 digits
    int settings_enabled;
    if (settings.find_pair(f_str("enabled")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'enabled'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'enabled' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_enabled(settings.get_cur_pair_value_len());
    if (str_enabled.ref == NULL)
    {
        esp_diag.error(APP_SETREMOTELOG_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setRemoteLog heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_enabled.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_enabled = atoi(str_enabled.ref);

    // settings_host
    if (settings.find_pair(f_str("host")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'host'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'host' does not have an STRING value type"), false);
        return;
    }
    Heap_chunk settings_host(settings.get_cur_pair_value_len());
    if (settings_host.ref == NULL)
    {
        esp_diag.error(APP_SETREMOTELOG_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setRemoteLog heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(settings_host.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    //    port: int,  5 digits
    int settings_port;
    if (settings.find_pair(f_str("port")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'port'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'port' does not have an INTEGER value type"), false);
        return;
    }
    Heap_chunk str_port(settings.get_cur_pair_value_len());
    if (str_port.ref == NULL)
    {
        esp_diag.error(APP_SETREMOTELOG_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setRemoteLog heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(str_port.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_port = atoi(str_port.ref);

    // settings_path
    if (settings.find_pair(f_str("path")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'path'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'path' does not have an STRING value type"), false);
        return;
    }
    Heap_chunk settings_path(settings.get_cur_pair_value_len());
    if (settings_path.ref == NULL)
    {
        esp_diag.error(APP_SETREMOTELOG_HEAP_EXHAUSTED, settings.get_cur_pair_value_len());
        ERROR("setRemoteLog heap exhausted %d", settings.get_cur_pair_value_len());
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    os_strncpy(settings_path.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    // save remote log cfg
    set_remote_log(settings_enabled, settings_host.ref, settings_port, settings_path.ref);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, f_str("{\"msg\":\"Settings saved\"}"), false);
}

static void getCtrlEvents(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getCtrlEvents");
    // { ctrl_events:[]}
    // {"ts":4294967295,"tp":1,"vl":-1234},
    // int events_count(void);
    // struct activity_event *get_event(int idx);
    int idx;
    int ev_num = events_count();
    int str_len = 17 + ev_num * 36 + 1;
    bool first_time = true;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETCTRLEVENTS_HEAP_EXHAUSTED, str_len);
        ERROR("getCtrlEvents heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref, "{\"ctrl_events\":[");
    struct activity_event *ev_i;
    for (idx = 0; idx < ev_num; idx++)
    {
        if (first_time)
            first_time = false;
        else
            fs_sprintf(msg.ref + os_strlen(msg.ref), ",");
        ev_i = get_event(idx);
        if (ev_i)
            fs_sprintf(msg.ref + os_strlen(msg.ref),
                       "{\"ts\":%d,\"tp\":%d,\"vl\":%d}",
                       ev_i->timestamp,
                       ev_i->type,
                       ev_i->value);
    }
    fs_sprintf(msg.ref + os_strlen(msg.ref), "]}");
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void getProgramList(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    ALL("getProgramList");
    // {"prg_count":,"prg_headings":[]}
    // {"id":,"desc":""},
    // {
    //     "prg_count": 3,
    //     "prg_headings": [
    //         {"id": 1,"desc":"first"},
    //         {"id": 2,"desc":"second"},
    //         {"id": 3,"desc":"third"}
    //     ]
    // }
    int str_len = 32 + 1 + 2 + (18 + 2 + 32) * program_lst->size();
    DEBUG("program list JSON len: %d", str_len);
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETPROGRAMLIST_HEAP_EXHAUSTED, str_len);
        ERROR("getProgramList heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
        return;
    }
    fs_sprintf(msg.ref, "{\"prg_count\":%d,\"prg_headings\":[", program_lst->size());
    struct prgm_headings *ptr = program_lst->front();
    bool first_time = true;
    while (ptr)
    {
        if (first_time)
        {
            fs_sprintf(msg.ref + os_strlen(msg.ref), "{\"id\":%d,\"desc\":\"%s\"}",
                       ptr->id,
                       ptr->desc);
            first_time = false;
        }
        else
        {
            fs_sprintf(msg.ref + os_strlen(msg.ref), ",{\"id\":%d,\"desc\":\"%s\"}",
                       ptr->id,
                       ptr->desc);
        }
        ptr = program_lst->next();
    }
    fs_sprintf(msg.ref + os_strlen(msg.ref), "]}");
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
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
    struct prgm *program = load_program(program_id);
    // will need to free the memory allocated by load_program
    if (program == NULL)
    {
        http_response(ptr_espconn, HTTP_NOT_FOUND, HTTP_CONTENT_JSON, f_str("Program not found"), false);
        return;
    }
    // {"id":,"min_temp":,"period_count":,"periods":[]}
    // {"wd":,"b":,"e":,"sp":},
    // {
    //     "id": 1,
    //     "min_temp": 100,
    //     "period_count": 3,
    //     "periods": [
    //         {"wd": 8,"b":1880,"e":1880,"sp":200},
    //         {"wd": 8,"b":1880,"e":1880,"sp":200},
    //         {"wd": 8,"b":1880,"e":1880,"sp":200}
    //     ]
    // }
    int str_len = 48 + 1 + 2 + 5 + 4 + (24 + 1 + 4 + 4 + 5) * program->period_count;
    DEBUG("program JSON len: %d", str_len);
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esp_diag.error(APP_GETPROGRAM_HEAP_EXHAUSTED, str_len);
        ERROR("getProgram heap exhausted %d", str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("Heap memory exhausted"), false);
        delete_program(program);
        return;
    }
    fs_sprintf(msg.ref, "{\"id\":%d,\"min_temp\":%d,\"period_count\":%d,\"periods\":[",
               program->id,
               program->min_temp,
               program->period_count);
    bool first_time = true;
    int idx;
    for (idx = 0; idx < program->period_count; idx++)
    {
        if (first_time)
        {
            fs_sprintf(msg.ref + os_strlen(msg.ref), "{\"wd\":%d,\"b\":%d,\"e\":%d,\"sp\":%d}",
                       program->period[idx].day_of_week,
                       program->period[idx].mm_start,
                       program->period[idx].mm_end,
                       program->period[idx].setpoint);
            first_time = false;
        }
        else
        {
            fs_sprintf(msg.ref + os_strlen(msg.ref), ",{\"wd\":%d,\"b\":%d,\"e\":%d,\"sp\":%d}",
                       program->period[idx].day_of_week,
                       program->period[idx].mm_start,
                       program->period[idx].mm_end,
                       program->period[idx].setpoint);
        }
    }
    fs_sprintf(msg.ref + os_strlen(msg.ref), "]}");
    delete_program(program);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
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
    //     "period_count" : 2,
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
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    //    name: string,         32 digits
    if (settings.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'name'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'name' does not have an STRING value type"), false);
        return;
    }
    char program_name[32];
    os_memset(program_name, 0, 32);
    if (settings.get_cur_pair_value_len() > 31)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    os_strncpy(program_name, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    //    min_temp: int,         1 digits
    int settings_min_temp;
    if (settings.find_pair(f_str("min_temp")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'min_temp'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'min_temp' does not have an INTEGER value type"), false);
        return;
    }
    char str_min_temp[6];
    os_memset(str_min_temp, 0, 6);
    if (settings.get_cur_pair_value_len() > 5)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    os_strncpy(str_min_temp, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_min_temp = atoi(str_min_temp);

    // settings_period_count
    if (settings.find_pair(f_str("period_count")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'period_count'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'period_count' does not have an INTEGER value type"), false);
        return;
    }
    char str_settings_period_count[4];
    os_memset(str_settings_period_count, 0, 4);
    if (settings.get_cur_pair_value_len() > 3)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    os_strncpy(str_settings_period_count, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    struct prgm program;
    program.min_temp = settings_min_temp;
    program.period_count = atoi(str_settings_period_count);
    Heap_chunk periods_mem(sizeof(struct prgm_period) * program.period_count);
    if (periods_mem.ref == NULL)
    {
        esp_diag.error(APP_CREATEPROGRAM_HEAP_EXHAUSTED, (sizeof(struct prgm_period) * program.period_count));
        ERROR("createProgram heap exhausted %d", (sizeof(struct prgm_period) * program.period_count));
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    program.period = (struct prgm_period *)periods_mem.ref;

    // periods
    if (settings.find_pair(f_str("periods")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'periods'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_ARRAY)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'periods' does not have an ARRAY value type"), false);
        return;
    }
    Json_array_str periods(settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    if (periods.size() != program.period_count)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }

    int idx;
    for (idx = 0; idx < program.period_count; idx++)
    {
        Json_str period(periods.get_elem(idx), periods.get_elem_len(idx));
        // {
        //     "wd" : 8,
        //     "b" : 100,
        //     "e" : 200,
        //     "sp" : 200
        // }
        //    wd: int,         1 digits
        if (period.find_pair(f_str("wd")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'wd'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'wd' does not have an INTEGER value type"), false);
            return;
        }
        char str_wd[2];
        os_memset(str_wd, 0, 2);
        if (period.get_cur_pair_value_len() > 1)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_wd, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].day_of_week = (week_days)atoi(str_wd);
        //    b: int,         4 digits
        if (period.find_pair(f_str("b")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'b'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'b' does not have an INTEGER value type"), false);
            return;
        }
        char str_mm_start[5];
        os_memset(str_mm_start, 0, 5);
        if (period.get_cur_pair_value_len() > 4)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_mm_start, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].mm_start = atoi(str_mm_start);
        //    e: int,         4 digits
        if (period.find_pair(f_str("e")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'e'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'e' does not have an INTEGER value type"), false);
            return;
        }
        char str_mm_end[5];
        os_memset(str_mm_end, 0, 5);
        if (period.get_cur_pair_value_len() > 4)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_mm_end, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].mm_end = atoi(str_mm_end);
        //    sp: int,         5 digits
        if (period.find_pair(f_str("sp")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'sp'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'sp' does not have an INTEGER value type"), false);
            return;
        }
        char str_setpoint[6];
        os_memset(str_setpoint, 0, 6);
        if (period.get_cur_pair_value_len() > 5)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_setpoint, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].setpoint = atoi(str_setpoint);
    }
    int result = add_program(program_name, &program);
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
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    //    name: string,         32 digits
    if (settings.find_pair(f_str("name")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'name'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'name' does not have an STRING value type"), false);
        return;
    }
    char program_name[32];
    os_memset(program_name, 0, 32);
    if (settings.get_cur_pair_value_len() > 31)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    os_strncpy(program_name, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    //    min_temp: int,         1 digits
    int settings_min_temp;
    if (settings.find_pair(f_str("min_temp")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'min_temp'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'min_temp' does not have an INTEGER value type"), false);
        return;
    }
    char str_min_temp[6];
    os_memset(str_min_temp, 0, 6);
    if (settings.get_cur_pair_value_len() > 5)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    os_strncpy(str_min_temp, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_min_temp = atoi(str_min_temp);

    // settings_period_count
    if (settings.find_pair(f_str("period_count")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'period_count'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'period_count' does not have an INTEGER value type"), false);
        return;
    }
    char str_settings_period_count[4];
    os_memset(str_settings_period_count, 0, 4);
    if (settings.get_cur_pair_value_len() > 3)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }
    os_strncpy(str_settings_period_count, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    struct prgm program;
    program.min_temp = settings_min_temp;
    program.period_count = atoi(str_settings_period_count);
    Heap_chunk periods_mem(sizeof(struct prgm_period) * program.period_count);
    if (periods_mem.ref == NULL)
    {
        esp_diag.error(APP_UPDATEPROGRAM_HEAP_EXHAUSTED, (sizeof(struct prgm_period) * program.period_count));
        ERROR("updateProgram heap exhausted %d", (sizeof(struct prgm_period) * program.period_count));
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, f_str("not enough heap memory"), false);
        return;
    }
    program.period = (struct prgm_period *)periods_mem.ref;

    // periods
    if (settings.find_pair(f_str("periods")) != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'periods'"), false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_ARRAY)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'periods' does not have an ARRAY value type"), false);
        return;
    }
    Json_array_str periods(settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    if (periods.size() != program.period_count)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
        return;
    }

    int idx;
    for (idx = 0; idx < program.period_count; idx++)
    {
        Json_str period(periods.get_elem(idx), periods.get_elem_len(idx));
        // {
        //     "wd" : 8,
        //     "b" : 100,
        //     "e" : 200,
        //     "sp" : 200
        // }
        //    wd: int,         1 digits
        if (period.find_pair(f_str("wd")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'wd'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'wd' does not have an INTEGER value type"), false);
            return;
        }
        char str_wd[2];
        os_memset(str_wd, 0, 2);
        if (period.get_cur_pair_value_len() > 1)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_wd, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].day_of_week = (week_days)atoi(str_wd);
        //    b: int,         4 digits
        if (period.find_pair(f_str("b")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'b'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'b' does not have an INTEGER value type"), false);
            return;
        }
        char str_mm_start[5];
        os_memset(str_mm_start, 0, 5);
        if (period.get_cur_pair_value_len() > 4)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_mm_start, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].mm_start = atoi(str_mm_start);
        //    e: int,         4 digits
        if (period.find_pair(f_str("e")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'e'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'e' does not have an INTEGER value type"), false);
            return;
        }
        char str_mm_end[5];
        os_memset(str_mm_end, 0, 5);
        if (period.get_cur_pair_value_len() > 4)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_mm_end, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].mm_end = atoi(str_mm_end);
        //    sp: int,         5 digits
        if (period.find_pair(f_str("sp")) != JSON_NEW_PAIR_FOUND)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Cannot find JSON string 'sp'"), false);
            return;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("JSON pair with string 'sp' does not have an INTEGER value type"), false);
            return;
        }
        char str_setpoint[6];
        os_memset(str_setpoint, 0, 6);
        if (period.get_cur_pair_value_len() > 5)
        {
            http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, f_str("Json bad syntax"), false);
            return;
        }
        os_strncpy(str_setpoint, period.get_cur_pair_value(), period.get_cur_pair_value_len());
        program.period[idx].setpoint = atoi(str_setpoint);
    }
    int result = mod_program(program_id, program_name, &program);
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
        getCtrlEvents(ptr_espconn, parsed_req);
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