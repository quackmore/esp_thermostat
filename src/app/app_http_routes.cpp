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
#include "espbot_debug.hpp"
#include "espbot_global.hpp"
#include "espbot_http_routes.hpp"
#include "espbot_json.hpp"
#include "espbot_logger.hpp"
#include "espbot_utils.hpp"
#include "espbot_webserver.hpp"
#include "library.hpp"
#include "app.hpp"
#include "app_activity_log.hpp"
#include "app_cron.hpp"
#include "app_heater.hpp"
#include "app_http_routes.hpp"
#include "app_temp_control.hpp"
#include "app_temp_log.hpp"

static void get_api_info(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
    int str_len = os_strlen(app_name) +
                  os_strlen(app_release) +
                  os_strlen(espbot.get_name()) +
                  os_strlen(espbot.get_version()) +
                  os_strlen(library_release) +
                  10 +
                  os_strlen(system_get_sdk_version()) +
                  10;
    Heap_chunk msg(155 + str_len, dont_free);
    if (msg.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "Heap memory exhausted!", false);
        return;
    }
    os_sprintf(msg.ref,
               "{\"app_name\":\"%s\","
               "\"app_version\":\"%s\","
               "\"espbot_name\":\"%s\","
               "\"espbot_version\":\"%s\","
               "\"library_version\":\"%s\","
               "\"chip_id\":\"%d\","
               "\"sdk_version\":\"%s\","
               "\"boot_version\":\"%d\"}",
               app_name,
               app_release,
               espbot.get_name(),
               espbot.get_version(),
               library_release,
               system_get_chip_id(),
               system_get_sdk_version(),
               system_get_boot_version());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void get_api_debug_last_rst(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
    // enum rst_reason
    // {
    //     REASON_DEFAULT_RST = 0,
    //     REASON_WDT_RST = 1,
    //     REASON_EXCEPTION_RST = 2,
    //     REASON_SOFT_WDT_RST = 3,
    //     REASON_SOFT_RESTART = 4,
    //     REASON_DEEP_SLEEP_AWAKE = 5,
    //     REASON_EXT_SYS_RST = 6
    // };
    // struct rst_info
    // {
    //     uint32 reason;
    //     uint32 exccause;
    //     uint32 epc1;
    //     uint32 epc2;
    //     uint32 epc3;
    //     uint32 excvaddr;
    //     uint32 depc;
    // };
    // {"date":"","reason": ,"exccause": ,"epc1": ,"epc2": ,"epc3": ,"evcvaddr": ,"depc": }
    int str_len = 84 + 24 + 7 * 10 + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "Heap memory exhausted!", false);
        return;
    }
    struct rst_info *last_rst = system_get_rst_info();
    os_sprintf(msg.ref,
               "{\"date\":\"%s\","
               "\"reason\": %X,"
               "\"exccause\": %X,"
               "\"epc1\": %X,"
               "\"epc2\": %X,"
               "\"epc3\": %X,"
               "\"evcvaddr\": %X,"
               "\"depc\": %X}",
               esp_sntp.get_timestr(get_last_reboot_date()),
               last_rst->reason,
               last_rst->exccause,
               last_rst->epc1,
               last_rst->epc2,
               last_rst->epc3,
               last_rst->excvaddr,
               last_rst->depc);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void get_api_temp_ctrl_vars(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
    // {
    //   "ctrl_date": uint32,                 11 digits
    //   "current_temp": int,                  4 digits
    //   "heater_status": int,                 1 digit
    //   "auto_setpoint": int,                 4 digits
    //   "ctrl_mode": int,                     1 digit
    //   "pwr_off_timer_started_on": uint32,  11 digits
    //   "pwr_off_timer": int                  4 digits
    // }
    int str_len = 137 + 11 + 4 + 1 + 4 + 1 + 11 + 4 + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "Heap memory exhausted!", false);
        return;
    }
    struct date *current_time = get_current_time();
    os_sprintf(msg.ref,
               "{\"ctrl_date\":%d,"
               "\"current_temp\":%d,"
               "\"heater_status\":%d,"
               "\"auto_setpoint\":%d,"
               "\"ctrl_mode\":%d,"
               "\"pwr_off_timer_started_on\":%d,"
               "\"pwr_off_timer\":%d}",
               (current_time->timestamp - 3600), // sending UTC time
               get_temp(0),
               (is_heater_on() ? 1 : 0),
               get_auto_setpoint(),
               get_current_mode(),
               (get_pwr_off_timer_started_on() - 3600),
               get_pwr_off_timer());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void get_api_temp_ctrl_settings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    pwr_off_timer: int      4 digits
    //  }
    int str_len = 98 + 1 + 4 + 4 + 4 + 4 + 1;
    Heap_chunk msg(str_len, dont_free);
    if (msg.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "Heap memory exhausted!", false);
        return;
    }
    os_sprintf(msg.ref,
               "{\"ctrl_mode\":%d,"
               "\"manual_pulse_on\":%d,"
               "\"manual_pulse_off\":%d,"
               "\"auto_setpoint\":%d,"
               "\"pwr_off_timer\":%d}",
               get_current_mode(),
               get_manual_pulse_on(),
               get_manual_pulse_off(),
               get_auto_setpoint(),
               get_pwr_off_timer());
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void post_api_temp_ctrl_settings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
    //  {
    //    ctrl_mode: int,         1 digits
    //    manual_pulse_on: int,   4 digits
    //    manual_pulse_off: int,  4 digits
    //    auto_setpoint: int,     4 digits
    //    pwr_off_timer: int      4 digits
    //  }
    //
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Json bad syntax", false);
        return;
    }
    //    ctrl_mode: int,         1 digits
    int settings_ctrl_mode;
    if (settings.find_pair("ctrl_mode") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'ctrl_mode'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'ctrl_mode' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_ctrl_mode(settings.get_cur_pair_value_len());
    if (str_ctrl_mode.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(str_ctrl_mode.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_ctrl_mode = atoi(str_ctrl_mode.ref);

    // settings_manual_pulse_on
    int settings_manual_pulse_on;
    if (settings.find_pair("manual_pulse_on") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'manual_pulse_on'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'manual_pulse_on' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_manual_pulse_on(settings.get_cur_pair_value_len());
    if (str_manual_pulse_on.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(str_manual_pulse_on.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_manual_pulse_on = atoi(str_manual_pulse_on.ref);

    //    manual_pulse_off: int,  4 digits
    int settings_manual_pulse_off;
    if (settings.find_pair("manual_pulse_off") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'manual_pulse_off'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'manual_pulse_off' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_manual_pulse_off(settings.get_cur_pair_value_len());
    if (str_manual_pulse_off.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(str_manual_pulse_off.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_manual_pulse_off = atoi(str_manual_pulse_off.ref);

    //    auto_setpoint: int,     4 digits
    int settings_auto_setpoint;
    if (settings.find_pair("auto_setpoint") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'auto_setpoint'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'auto_setpoint' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_auto_setpoint(settings.get_cur_pair_value_len());
    if (str_auto_setpoint.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(str_auto_setpoint.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_auto_setpoint = atoi(str_auto_setpoint.ref);

    //    pwr_off_timer: int      4 digits
    int settings_power_off_timer;
    if (settings.find_pair("power_off_timer") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'power_off_timer'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'power_off_timer' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_power_off_timer(settings.get_cur_pair_value_len());
    if (str_power_off_timer.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
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
    default:
        break;
    }
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_TEXT, "Settings saved!", false);
}

static void get_api_temp_ctrl_adv_settings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
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
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "Heap memory exhausted!", false);
        return;
    }
    struct _adv_ctrl_settings *adv_settings = get_adv_ctrl_settings();
    os_sprintf(msg.ref,
               "{\"kp\": %d,"
               "\"kd\": %d,"
               "\"ki\": %d,"
               "\"u_max\": %d,"
               "\"heater_on_min\": %d,"
               "\"heater_on_max\": %d,"
               "\"heater_on_off\": %d,"
               "\"heater_cold\": %d,"
               "\"warm_up_period\": %d,"
               "\"wup_heater_on\": %d,"
               "\"wup_heater_off\": %d}",
               adv_settings->kp,
               adv_settings->kd,
               adv_settings->ki,
               adv_settings->u_max,
               adv_settings->heater_on_min,
               adv_settings->heater_on_max,
               adv_settings->heater_on_off,
               adv_settings->heater_cold,
               adv_settings->warm_up_period,
               adv_settings->wup_heater_on,
               adv_settings->wup_heater_off);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void post_api_temp_ctrl_adv_settings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
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
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Json bad syntax", false);
        return;
    }

    struct _adv_ctrl_settings adv_ctrl_settings;
    char tmp_str[8];
    
    //   kp: int,             6 digit (5 digit and sign)
    if (settings.find_pair("kp") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'kp'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'kp' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.kp = atoi(tmp_str);

    //   kd: int,             6 digit (5 digit and sign)
    if (settings.find_pair("kd") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'kd'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'kd' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.kd = atoi(tmp_str);

    //   ki: int,             6 digit (5 digit and sign)
    if (settings.find_pair("ki") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'ki'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'ki' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.ki = atoi(tmp_str);

    //   u_max: int,          6 digit (5 digit and sign)
    if (settings.find_pair("u_max") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'u_max'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'u_max' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.u_max = atoi(tmp_str);

    //   heater_on_min: int,  5 digit
    if (settings.find_pair("heater_on_min") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'heater_on_min'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'heater_on_min' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_on_min = atoi(tmp_str);

    //   heater_on_max: int,  5 digit
    if (settings.find_pair("heater_on_max") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'heater_on_max'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'heater_on_max' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_on_max = atoi(tmp_str);

    //   heater_on_off: int,  5 digit
    if (settings.find_pair("heater_on_off") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'heater_on_off'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'heater_on_off' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_on_off = atoi(tmp_str);

    //   heater_cold: int,  5 digit
    if (settings.find_pair("heater_cold") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'heater_cold'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'heater_cold' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.heater_cold = atoi(tmp_str);

    //   warm_up_period: int,  5 digit
    if (settings.find_pair("warm_up_period") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'warm_up_period'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'warm_up_period' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.warm_up_period = atoi(tmp_str);

    //   wup_heater_on: int,  5 digit
    if (settings.find_pair("wup_heater_on") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'wup_heater_on'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'wup_heater_on' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.wup_heater_on = atoi(tmp_str);

    //   wup_heater_off: int,  5 digit
    if (settings.find_pair("wup_heater_off") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'wup_heater_off'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'wup_heater_off' does not have an INTEGER value type", false);
        return;
    }
    if (settings.get_cur_pair_value_len() > 6)
    {
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "number bad format", false);
        return;
    }
    os_memset(tmp_str, 0, 8);
    os_strncpy(tmp_str, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    adv_ctrl_settings.wup_heater_off = atoi(tmp_str);

    set_adv_ctrl_settings(&adv_ctrl_settings);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_TEXT, "Settings saved!", false);
}

static void get_api_remote_log_settings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
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
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, str_len);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "Heap memory exhausted!", false);
        return;
    }
    // "{"enabled": ,"host": "","port": ,"path": ""}",
    os_sprintf(msg.ref,
               "{\"enabled\": %d,\"host\": \"%s\",\"port\": %d,\"path\": \"%s\"}",
               get_remote_log_enabled(),
               get_remote_log_host(),
               get_remote_log_port(),
               get_remote_log_path());

    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_JSON, msg.ref, true);
}

static void post_api_remote_log_settings(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);
    //  {
    //    enabled: int,   1 digits
    //    host: string,   ...
    //    port: int,      5 digits
    //    path: string    ...
    //  }
    Json_str settings(parsed_req->req_content, parsed_req->content_len);
    if (settings.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Json bad syntax", false);
        return;
    }
    //    enabled: int,         1 digits
    int settings_enabled;
    if (settings.find_pair("enabled") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'enabled'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'enabled' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_enabled(settings.get_cur_pair_value_len());
    if (str_enabled.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(str_enabled.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_enabled = atoi(str_enabled.ref);

    // settings_host
    if (settings.find_pair("host") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'host'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'host' does not have an STRING value type", false);
        return;
    }
    Heap_chunk settings_host(settings.get_cur_pair_value_len());
    if (settings_host.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(settings_host.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    //    port: int,  5 digits
    int settings_port;
    if (settings.find_pair("port") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'port'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'port' does not have an INTEGER value type", false);
        return;
    }
    Heap_chunk str_port(settings.get_cur_pair_value_len());
    if (str_port.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(str_port.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());
    settings_port = atoi(str_port.ref);

    // settings_path
    if (settings.find_pair("path") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'path'", false);
        return;
    }
    if (settings.get_cur_pair_value_type() != JSON_STRING)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'path' does not have an STRING value type", false);
        return;
    }
    Heap_chunk settings_path(settings.get_cur_pair_value_len());
    if (settings_path.ref == NULL)
    {
        esplog.error("%s - not enough heap memory [%d]\n", __FUNCTION__, settings.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_strncpy(settings_path.ref, settings.get_cur_pair_value(), settings.get_cur_pair_value_len());

    // save remote log cfg
    set_remote_log(settings_enabled, settings_host.ref, settings_port, settings_path.ref);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_TEXT, "Settings saved!", false);
}

void run_test(int test_number)
{
    // void set_remote_log(bool enabled, char* host, int port, char* path);
    // bool get_remote_log_enabled(void);
    // char *get_remote_log_host(void);
    // int get_remote_log_port(void);
    // char *get_remote_log_path(void);

    switch (test_number)
    {
    case 10:
        os_printf("---> remote log cfg\n");
        os_printf("---> enabled: %d\n", get_remote_log_enabled());
        os_printf("--->    host: %s\n", get_remote_log_host());
        os_printf("--->    port: %d\n", get_remote_log_port());
        os_printf("--->    path: %s\n", get_remote_log_path());
        break;
    case 11:
        set_remote_log(false, "192.168.1.201", 21000, "/this/is/the/path");
        break;
    case 12:
        set_remote_log(true, "192.168.1.102", 1880, "/activity_log");
        break;

    case 20:
    {
        struct _adv_ctrl_settings *adv_settings = get_adv_ctrl_settings();

        os_printf("---> advanced settings cfg\n");
        os_printf("{\"kp\": %d,"
                  "\"kd\": %d,"
                  "\"ki\": %d,"
                  "\"u_max\": %d,"
                  "\"heater_on_min\": %d,"
                  "\"heater_on_max\": %d,"
                  "\"heater_on_off\": %d,"
                  "\"heater_cold\": %d,"
                  "\"warm_up_period\": %d,"
                  "\"wup_heater_on\": %d,"
                  "\"wup_heater_off\": %d}",
                  adv_settings->kp,
                  adv_settings->kd,
                  adv_settings->ki,
                  adv_settings->u_max,
                  adv_settings->heater_on_min,
                  adv_settings->heater_on_max,
                  adv_settings->heater_on_off,
                  adv_settings->heater_cold,
                  adv_settings->warm_up_period,
                  adv_settings->wup_heater_on,
                  adv_settings->wup_heater_off);
    }
    break;
    case 21:
    {
        struct _adv_ctrl_settings adv_settings;

        adv_settings.kp = 11;
        adv_settings.kd = 12;
        adv_settings.ki = 13;
        adv_settings.u_max = 14;
        adv_settings.heater_on_min = 15;
        adv_settings.heater_on_max = 16;
        adv_settings.heater_on_off = 17;
        adv_settings.heater_cold = 18;
        adv_settings.warm_up_period = 19;
        adv_settings.wup_heater_on = 20;
        adv_settings.wup_heater_off = 21;

        set_adv_ctrl_settings(&adv_settings);
    }
    break;
    case 22:
    {
        struct _adv_ctrl_settings adv_settings;
        
        adv_settings.kp = 31;
        adv_settings.kd = 32;
        adv_settings.ki = 33;
        adv_settings.u_max = 34;
        adv_settings.heater_on_min = 35;
        adv_settings.heater_on_max = 36;
        adv_settings.heater_on_off = 37;
        adv_settings.heater_cold = 38;
        adv_settings.warm_up_period = 39;
        adv_settings.wup_heater_on = 40;
        adv_settings.wup_heater_off = 41;

        set_adv_ctrl_settings(&adv_settings);
    }
        break;

    default:
        break;
    }
}

static void post_test(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    int test_number;
    os_printf("--- TEST ---\n");
    Json_str test_cfg(parsed_req->req_content, parsed_req->content_len);
    if (test_cfg.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Json bad syntax", false);
        return;
    }
    if (test_cfg.find_pair("test_number") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string string test_number'", false);
        return;
    }
    if (test_cfg.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn,
                      HTTP_BAD_REQUEST,
                      HTTP_CONTENT_JSON,
                      "JSON pair with string 'test_number' does not have a INTEGER value type",
                      false);
        return;
    }
    Heap_chunk tmp_test_number(test_cfg.get_cur_pair_value_len());
    if (tmp_test_number.ref == NULL)
    {
        esplog.error("Websvr::webserver_recv - not enough heap memory %d\n", test_cfg.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "notough heap memory", false);
        return;
    }
    os_strncpy(tmp_test_number.ref, test_cfg.get_cur_pair_value(), test_cfg.get_cur_pair_value_len());
    test_number = atoi(tmp_test_number.ref);
    espmem.stack_mon();
    Heap_chunk msg(36, dont_free);
    if (msg.ref == NULL)
    {
        esplog.error("Websvr::webserver_recv - not enough heap memory %d\n", 36);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
        return;
    }
    os_sprintf(msg.ref, "{\"test_number\": %d}", test_number);
    http_response(ptr_espconn, HTTP_OK, HTTP_CONTENT_TEXT, msg.ref, true);
    // esp_free(msg); // dont't free the msg buffer cause it could not have been used yet
    run_test(test_number);
    return;
}

bool app_http_routes(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    esplog.all("%s\n", __FUNCTION__);

    if ((0 == os_strcmp(parsed_req->url, "/api/info")) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_info(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/debug/last_rst")) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_debug_last_rst(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/temp_ctrl_vars")) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_temp_ctrl_vars(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/temp_ctrl_settings")) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_temp_ctrl_settings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/temp_ctrl_settings")) && (parsed_req->req_method == HTTP_POST))
    {
        post_api_temp_ctrl_settings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/temp_ctrl_adv_settings")) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_temp_ctrl_adv_settings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/temp_ctrl_adv_settings")) && (parsed_req->req_method == HTTP_POST))
    {
        post_api_temp_ctrl_adv_settings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/remote_log_settings")) && (parsed_req->req_method == HTTP_GET))
    {
        get_api_remote_log_settings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/remote_log_settings")) && (parsed_req->req_method == HTTP_POST))
    {
        post_api_remote_log_settings(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/test")) && (parsed_req->req_method == HTTP_POST))
    {
        post_test(ptr_espconn, parsed_req);
        return true;
    }
    return false;
}