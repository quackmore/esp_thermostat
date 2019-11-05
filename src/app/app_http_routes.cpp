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

#include "espbot_webserver.hpp"
#include "espbot_http_routes.hpp"
#include "espbot.hpp"
#include "espbot_global.hpp"
#include "espbot_logger.hpp"
#include "espbot_json.hpp"
#include "espbot_utils.hpp"
#include "espbot_debug.hpp"
#include "library.hpp"
#include "app.hpp"
#include "app_http_routes.hpp"
#include "app_temp_control.hpp"

static void api_debug_last_rst(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
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
        esplog.error("Websvr::webserver_recv - not enough heap memory %d\n", __FUNCTION__, str_len);
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

static void api_info(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    int str_len = os_strlen(app_name) +
                  os_strlen(app_release) +
                  os_strlen(espbot.get_name()) +
                  os_strlen(espbot.get_version()) +
                  os_strlen(library_release) +
                  10 +
                  os_strlen(system_get_sdk_version()) +
                  10;
    Heap_chunk msg(155 + str_len, dont_free);
    if (msg.ref)
    {
        os_sprintf(msg.ref, "{\"app_name\":\"%s\","
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
    else
    {
        esplog.error("Websvr::webserver_recv - not enough heap memory %d\n", 350);
    }
}

void run_test(int);

static void post_test(struct espconn *ptr_espconn, Http_parsed_req *parsed_req)
{
    int test_number;
    Json_str test_cfg(parsed_req->req_content, parsed_req->content_len);
    if (test_cfg.syntax_check() != JSON_SINTAX_OK)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Json bad syntax", false);
        return;
    }
    if (test_cfg.find_pair("test_number") != JSON_NEW_PAIR_FOUND)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "Cannot find JSON string 'test_number'", false);
        return;
    }
    if (test_cfg.get_cur_pair_value_type() != JSON_INTEGER)
    {
        http_response(ptr_espconn, HTTP_BAD_REQUEST, HTTP_CONTENT_JSON, "JSON pair with string 'test_number' does not have a INTEGER value type", false);
        return;
    }
    Heap_chunk tmp_test_number(test_cfg.get_cur_pair_value_len());
    if (tmp_test_number.ref == NULL)
    {
        esplog.error("Websvr::webserver_recv - not enough heap memory %d\n", test_cfg.get_cur_pair_value_len() + 1);
        http_response(ptr_espconn, HTTP_SERVER_ERROR, HTTP_CONTENT_JSON, "not enough heap memory", false);
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
        api_info(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/debug/last_rst")) && (parsed_req->req_method == HTTP_GET))
    {
        api_debug_last_rst(ptr_espconn, parsed_req);
        return true;
    }
    if ((0 == os_strcmp(parsed_req->url, "/api/test")) && (parsed_req->req_method == HTTP_POST))
    {
        post_test(ptr_espconn, parsed_req);
        return true;
    }
    return false;
}