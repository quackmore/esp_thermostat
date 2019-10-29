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

#include "app_logger.hpp"
#include "espbot_global.hpp"
#include "espbot_utils.hpp"
#include "espbot_config.hpp"
#include "espbot_webclient.hpp"
#include "library_dht.hpp"
#include "library_max6675.hpp"

Dht *dht22;
D_logger *dht22_temp_logger;
D_logger *dht22_humi_logger;

Max6675 *max6675;
D_logger *max6675_logger;

List<sensors_event_t> *event_log;
// List<char> *unsent_dweets;

sensors_event_t *get_log_event(int idx)
{
    esplog.all("%s\n", __FUNCTION__);
    sensors_event_t *ptr = event_log->front();
    while ((ptr != NULL) && (idx > 0))
    {
        ptr = event_log->next();
        idx--;
    }
    return ptr;
}

void data_log_to_serial(D_logger *ptr)
{
    os_printf("-----> data logger debug print\n");
    sensors_event_t event;
    os_memset(&event, 0, sizeof(sensors_event_t));
    ptr->_sensor->getEvent(&event, 0);
    char value_str[11];
    get_sensor_value_str(&event, value_str);
    os_printf("{\n"
              "  \"description\":\"%s\","
              "  \"sensor_id\":%d,"
              "  \"sensor_type\":\"%s\","
              "  \"time\":\"%s\","
              "  \"value\":%s,"
              "  \"validity\":\"%s\"\n"
              "}\n",
              ptr->_desc,
              event.sensor_id,
              get_sensor_type_str(event.type),
              esp_sntp.get_timestr(event.timestamp),
              value_str,
              (event.invalid) ? "I" : "V");
}

void data_log_to_mem(D_logger *ptr)
{
    esplog.all("%s\n", __FUNCTION__);
    sensors_event_t *event = new sensors_event_t;
    os_memset(event, 0, sizeof(sensors_event_t));
    ptr->_sensor->getEvent(event, 0);
    List_err result = event_log->push_front(event, override_when_full);
    if (result != list_ok)
        esplog.error("%s - event_log->push_front error %d\n", __FUNCTION__, result);
}

void data_log_to_file(D_logger *ptr)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    // get the filename
    char filename[32];
    sensor_t sensor;
    os_memset(&sensor, 0, sizeof(sensor_t));
    ptr->_sensor->getSensor(&sensor);
    os_sprintf(filename, "%d.log", sensor.sensor_id);
    Ffile logfile(&espfs, filename);
    if (!logfile.is_available())
    {
        esplog.error("%s - [%s] file not available\n", __FUNCTION__, filename);
        return;
    }
    sensors_event_t event;
    os_memset(&event, 0, sizeof(sensors_event_t));
    ptr->_sensor->getEvent(&event, 0);
    char value_str[11];
    get_sensor_value_str(&event, value_str);
    int log_str_len = 96 +
                      os_strlen(ptr->_desc) +
                      6 +
                      12 +
                      24 +
                      11 +
                      1;

    Heap_chunk log_str(log_str_len);
    if (log_str.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, log_str_len);
        return;
    }
    os_sprintf(log_str.ref,
               "{"
               "\"description\":\"%s\","
               "\"sensor_id\":%d,"
               "\"sensor_type\":\"%s\","
               "\"time\":\"%s\","
               "\"value\":%s,"
               "\"validity\":\"%s\""
               "}\n",
               ptr->_desc,
               event.sensor_id,
               get_sensor_type_str(event.type),
               esp_sntp.get_timestr(event.timestamp),
               value_str,
               (event.invalid) ? "I" : "V");
    logfile.n_append(log_str.ref, os_strlen(log_str.ref));
}

static struct ip_addr dweet_ip;
static bool valid_dweet_ip;
static struct espconn dweet_espconn;
struct dweet_info
{
    Webclnt *espclient;
    char *sensor_info;
};

static void got_dweet_ip(const char *name, ip_addr_t *ipaddr, void *arg)
{
    esplog.all("%s\n", __FUNCTION__);
    if (ipaddr == NULL)
    {
        valid_dweet_ip = false;
        esplog.error("%s - cannot get dweet.io ip address\n", __FUNCTION__);
        return;
    }
    // os_printf("------ >ip:" IPSTR, IP2STR(ipaddr));
    // os_printf("\n");
    os_memcpy(&dweet_ip, ipaddr, sizeof(ipaddr));
    valid_dweet_ip = true;
}

void get_dweet_ip(void)
{
    valid_dweet_ip = false;
    espconn_gethostbyname(&dweet_espconn, "dweet.io", (ip_addr_t *)&dweet_ip, got_dweet_ip);
}

void free_client(void *param)
{
    struct dweet_info *info = (struct dweet_info *)param;

    delete info->sensor_info;
    delete info->espclient;
    delete info;
}

void check_dweet_answer(void *param)
{
    struct dweet_info *info = (struct dweet_info *)param;
    switch (info->espclient->get_status())
    {
    case WEBCLNT_RESPONSE_READY:
        if (info->espclient->parsed_response->http_code != HTTP_OK)
        {
            // there was an error
            esplog.error("dweet.io answered %d, %s\n", info->espclient->parsed_response->http_code, info->espclient->parsed_response->body);
        }
        break;
    default:
        esplog.error("%s - Ops ... webclient status is %d\n", __FUNCTION__, info->espclient->get_status());
        break;
    }
    info->espclient->disconnect(free_client, param);
}

void post_info(void *param)
{
    struct dweet_info *info = (struct dweet_info *)param;
    switch (info->espclient->get_status())
    {
    case WEBCLNT_CONNECTED:
    {
        int msg_len = 176 + 6 + os_strlen(info->sensor_info);
        Heap_chunk msg(msg_len);
        if (msg.ref == NULL)
        {
            esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, msg_len);
            info->espclient->disconnect(free_client, param);
            return;
        }

        os_sprintf(msg.ref,
                   "POST /dweet/for/37ced2c9-c2d8-4434-aa2a-9d77c2bb0a0e HTTP/1.1\r\n"
                   "Host: dweet.io\r\n"
                   "Content-Type: application/json\r\n"
                   "Accept: */*\r\n"
                   "Cache-Control: no-cache\r\n"
                   "Content-Length: %d\r\n\r\n%s\r\n",
                   os_strlen(info->sensor_info),
                   info->sensor_info);
        // os_printf("------> %s\n", msg.ref);
        info->espclient->send_req(msg.ref, check_dweet_answer, param);
        // info->espclient->disconnect(free_client, param);
    }
    break;
    default:
    {
        esplog.error("%s - Ops ... webclient status is %d\n", __FUNCTION__, info->espclient->get_status());
        // List_err result = unsent_dweets->push_front(info->sensor_info, override_when_full);
        // if (result != list_ok)
        //     esplog.error("%s - unsent_dweets->push_front error %d\n", __FUNCTION__, result);
        info->espclient->disconnect(free_client, param);
    }
    break;
    }
}

void data_log_to_dweet(D_logger *ptr)
{
    esplog.all("%s\n", __FUNCTION__);
    sensors_event_t event;
    os_memset(&event, 0, sizeof(sensors_event_t));
    ptr->_sensor->getEvent(&event, 0);
    char value_str[11];
    get_sensor_value_str(&event, value_str);
    int log_str_len = 94 +
                      os_strlen(ptr->_desc) +
                      6 +
                      12 +
                      24 +
                      11 +
                      1;

    Heap_chunk log_str(log_str_len, dont_free);
    if (log_str.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, log_str_len);
        return;
    }
    os_sprintf(log_str.ref,
               "{"
               "\"description\":\"%s\","
               "\"sensor_id\":%d,"
               "\"sensor_type\":\"%s\","
               "\"time\":\"%s\","
               "\"value\":%s,"
               "\"validity\":\"%s\""
               "}",
               ptr->_desc,
               event.sensor_id,
               get_sensor_type_str(event.type),
               esp_sntp.get_timestr(event.timestamp),
               value_str,
               (event.invalid) ? "I" : "V");
    //
    // POST /dweet/for/37ced2c9-c2d8-4434-aa2a-9d77c2bb0a0e HTTP/1.1
    // Host: dweet.io
    // Content-Type: application/json
    // Content-Length: 12
    //
    // {JSON oject}
    //

    // check dweet_ip_ptr and eventually log error
    if (!valid_dweet_ip)
    {
        esplog.error("%s - cannot post to dweet.io (missing ip address)\n", __FUNCTION__);
        return;
    }
    // start webclient
    Webclnt *espclient = new Webclnt;
    if (espclient == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, sizeof(Webclnt));
        return;
    }

    struct dweet_info *info = new struct dweet_info;
    if (info == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, sizeof(struct dweet_info));
        return;
    }

    info->espclient = espclient;
    info->sensor_info = log_str.ref;

    // os_printf("------ >ip:" IPSTR, IP2STR(&dweet_ip));
    // os_printf("\n");
    espclient->connect(dweet_ip, 80, post_info, (void *)info);
}

static struct ip_addr node_ip;
static struct espconn node_espconn;
struct node_info
{
    Webclnt *espclient;
    char *sensor_info;
};

void free_node_client(void *param)
{
    struct node_info *info = (struct node_info *)param;

    delete info->sensor_info;
    delete info->espclient;
    delete info;
}

void check_node_answer(void *param)
{
    struct node_info *info = (struct node_info *)param;
    switch (info->espclient->get_status())
    {
    case WEBCLNT_RESPONSE_READY:
        if (info->espclient->parsed_response->http_code != HTTP_OK)
        {
            // there was an error
            esplog.error("node-red answered %d, %s\n", info->espclient->parsed_response->http_code, info->espclient->parsed_response->body);
        }
        break;
    default:
        esplog.error("%s - Ops ... webclient status is %d\n", __FUNCTION__, info->espclient->get_status());
        break;
    }
    info->espclient->disconnect(free_node_client, param);
}

void post_node_info(void *param)
{
    struct node_info *info = (struct node_info *)param;
    switch (info->espclient->get_status())
    {
    case WEBCLNT_CONNECTED:
    {
        int msg_len = 176 + 6 + os_strlen(info->sensor_info);
        Heap_chunk msg(msg_len);
        if (msg.ref == NULL)
        {
            esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, msg_len);
            info->espclient->disconnect(free_node_client, param);
            return;
        }

        os_sprintf(msg.ref,
                   "POST /dweet HTTP/1.1\r\n"
                   "Host: 192.16.1.102\r\n"
                   "Content-Type: application/json\r\n"
                   "Accept: */*\r\n"
                   "Cache-Control: no-cache\r\n"
                   "Content-Length: %d\r\n\r\n%s\r\n",
                   os_strlen(info->sensor_info),
                   info->sensor_info);
        // os_printf("------> %s\n", msg.ref);
        info->espclient->send_req(msg.ref, check_node_answer, param);
        // info->espclient->disconnect(free_node_client, param);
    }
    break;
    default:
    {
        esplog.error("%s - Ops ... webclient status is %d\n", __FUNCTION__, info->espclient->get_status());
        // List_err result = unsent_dweets->push_front(info->sensor_info, override_when_full);
        // if (result != list_ok)
        //     esplog.error("%s - unsent_dweets->push_front error %d\n", __FUNCTION__, result);
        info->espclient->disconnect(free_client, param);
    }
    break;
    }
}

void data_log_to_node(D_logger *ptr)
{
    esplog.all("%s\n", __FUNCTION__);
    sensors_event_t event;
    os_memset(&event, 0, sizeof(sensors_event_t));
    ptr->_sensor->getEvent(&event, 0);
    char value_str[11];
    get_sensor_value_str(&event, value_str);
    int log_str_len = 94 +
                      os_strlen(ptr->_desc) +
                      6 +
                      12 +
                      24 +
                      11 +
                      1;

    Heap_chunk log_str(log_str_len, dont_free);
    if (log_str.ref == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, log_str_len);
        return;
    }
    os_sprintf(log_str.ref,
               "{"
               "\"description\":\"%s\","
               "\"sensor_id\":%d,"
               "\"sensor_type\":\"%s\","
               "\"time\":\"%s\","
               "\"value\":%s,"
               "\"validity\":\"%s\""
               "}",
               ptr->_desc,
               event.sensor_id,
               get_sensor_type_str(event.type),
               esp_sntp.get_timestr(event.timestamp),
               value_str,
               (event.invalid) ? "I" : "V");
    //
    // POST /dweet
    // Host: 192.168.1.102
    // Content-Type: application/json
    // Content-Length: 12
    //
    // {JSON oject}
    //

    // start webclient
    Webclnt *espclient = new Webclnt;
    if (espclient == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, sizeof(Webclnt));
        return;
    }

    struct node_info *info = new struct node_info;
    if (info == NULL)
    {
        esplog.error("%s - not enough heap memory %d\n", __FUNCTION__, sizeof(struct node_info));
        return;
    }

    info->espclient = espclient;
    info->sensor_info = log_str.ref;
    struct ip_addr host_ip;
    atoipaddr(&host_ip, "192.168.1.102");
    espclient->connect(host_ip, 1880, post_node_info, (void *)info);
}

void (*data_logger_functions[])(D_logger *) = {
    data_log_to_serial,
    data_log_to_mem,
    data_log_to_file,
    data_log_to_dweet,
    data_log_to_node};

app_log_action get_action_id(char *str)
{
    if (os_strcmp(str, "none") == 0)
        return none;
    if (os_strcmp(str, "log_to_memory") == 0)
        return log_to_memory;
    if (os_strcmp(str, "log_to_file") == 0)
        return log_to_file;
    if (os_strcmp(str, "log_to_dweet") == 0)
        return log_to_dweet;
    if (os_strcmp(str, "log_to_node") == 0)
        return log_to_node;
    return error;
}

static void start_logger_at_boot(int sensor_id)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    char filename[32];
    os_sprintf(filename, "%d_log.cfg", sensor_id);

    File_to_json cfgfile(filename);

    // {
    //   "sensor_id": 12000,
    //   "period": 60000,
    //   "desc": "test",
    //   "action_id": 1,
    //   "start_at_boot": 1
    // }

    if (cfgfile.exists())
    {
        // sensor_id
        if (cfgfile.find_string("sensor_id"))
        {
            esplog.error("%s - cannot find sensor_id\n", __FUNCTION__);
            return;
        }
        if (sensor_id != atoi(cfgfile.get_value()))
        {
            esplog.error("%s - unconsistent sensor_id value\n", __FUNCTION__);
            return;
        }
        // period
        if (cfgfile.find_string("period"))
        {
            esplog.error("%s - cannot find period\n", __FUNCTION__);
            return;
        }
        int period = atoi(cfgfile.get_value());
        // desc
        if (cfgfile.find_string("desc"))
        {
            esplog.error("%s - cannot find desc\n", __FUNCTION__);
            return;
        }
        char desc[33];
        os_strncpy(desc, cfgfile.get_value(), 33);
        // action_id
        if (cfgfile.find_string("action_id"))
        {
            esplog.error("%s - cannot find action_id\n", __FUNCTION__);
            return;
        }
        app_log_action action_id = (app_log_action)atoi(cfgfile.get_value());
        // start_at_boot
        if (cfgfile.find_string("start_at_boot"))
        {
            esplog.error("%s - cannot find start_at_boot\n", __FUNCTION__);
            return;
        }
        bool start_at_boot = atoi(cfgfile.get_value());
        os_printf("------->     sensor_id: %d\n", sensor_id);
        os_printf("------->        period: %d\n", period);
        os_printf("------->          desc: %s\n", desc);
        os_printf("------->     action_id: %d\n", action_id);
        os_printf("-------> start_at_boot: %d\n", start_at_boot);
        app_logger_start(sensor_id,
                         period,
                         desc,
                         action_id,
                         start_at_boot);
    }
    espmem.stack_mon();
}

void logger_init(void)
{
    esplog.all("logger_init\n");
    dht22 = NULL;
    dht22_temp_logger = NULL;
    dht22_humi_logger = NULL;
    max6675 = NULL;
    max6675_logger = NULL;
    event_log = new List<sensors_event_t>(40, delete_content);
    // unsent_dweets = new List<char>(5, delete_content);
    valid_dweet_ip = false;
    os_memset(&dweet_ip, 0, sizeof(ip_addr_t));
}

static os_timer_t logger_start_timer;

int sensor_id_list[] = {
    DHT_TEMP_ID,
    DHT_HUMI_ID};

static void start_logger(void)
{
    esplog.all("%s\n", __FUNCTION__);
    static int idx = 0;
    if (idx < 2)
    {
        start_logger_at_boot(sensor_id_list[idx]);
        idx++;
        os_timer_arm(&logger_start_timer, 2000, 0);
    }
}

void logger_start_at_boot(void)
{
    esplog.all("%s\n", __FUNCTION__);
    os_timer_disarm(&logger_start_timer);
    os_timer_setfn(&logger_start_timer, (os_timer_func_t *)start_logger, NULL);
    os_timer_arm(&logger_start_timer, 2000, 0);

    // start_logger_at_boot(DHT_TEMP_ID);
    // start_logger_at_boot(DHT_HUMI_ID);
}

char *get_sensor_type_str(sensors_type_t type)
{
    esplog.all("get_sensor_type_str\n");
    // max 12 chars
    switch (type)
    {
    case SENSOR_TYPE_TEMPERATURE:
        return "Temperature";
    case SENSOR_TYPE_RELATIVE_HUMIDITY:
        return "Humidity";
    default:
        return "Unknown";
    }
}

void get_sensor_value_str(sensors_event_t *event, char *str)
{
    esplog.all("get_sensor_value_str\n");
    // max 12 chars
    switch (event->type)
    {
    case SENSOR_TYPE_TEMPERATURE:
        switch (event->sensor_id)
        {
        case MAX6675_ID:
            f2str(str, event->temperature, 2);
            break;
        case DHT_TEMP_ID:
            f2str(str, event->temperature, 1);
            break;
        default:
            os_strcpy(str, "0.00");
            break;
        }
        break;
    case SENSOR_TYPE_RELATIVE_HUMIDITY:
        f2str(str, event->relative_humidity, 1);
        break;
    default:
        os_strcpy(str, "0.00");
        break;
    }
}

app_log_res app_logger_start(int sensor_id,
                             int poll_period,
                             char *desc,
                             app_log_action action,
                             bool start_at_boot)
{
    esplog.all("app_logger_start\n");
    switch (sensor_id)
    {
    case MAX6675_ID:
        // check if the sensor is in use
        if (max6675 == NULL)
        {
            max6675 = new Max6675(MAX6675_CS, MAX6675_SCK, MAX6675_SO, MAX6675_ID, 0, MAX6675_BUFFERS);
            if (max6675 == NULL)
            {
                esplog.error("api_data_logger_start - not enough heap memory %d\n", sizeof(Max6675));
                return app_log_not_enough_heap;
            }
        }
        // check if another logger is running
        if (max6675_logger == NULL)
        {
            max6675_logger = new D_logger(max6675,
                                          poll_period,
                                          desc,
                                          action,
                                          start_at_boot);
            if (max6675_logger == NULL)
            {
                delete max6675;
                max6675 == NULL;
                return app_log_not_enough_heap;
            }
        }
        else
        {
            return app_log_already_running;
        }
        break;
    case DHT_TEMP_ID:
    case DHT_HUMI_ID:
        // check if the sensor is in use
        if (dht22 == NULL)
        {
            dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, poll_period, DHT_BUFFERS);
            if (dht22 == NULL)
            {
                esplog.error("api_data_logger_start - not enough heap memory %d\n", sizeof(Dht));
                return app_log_not_enough_heap;
            }
        }
        if (sensor_id == DHT_TEMP_ID)
        {
            // check if another logger is running
            if (dht22_temp_logger == NULL)
            {
                dht22_temp_logger = new D_logger(&dht22->temperature,
                                                 poll_period,
                                                 desc,
                                                 action,
                                                 start_at_boot);
                if (dht22_temp_logger == NULL)
                {
                    if (dht22_humi_logger == NULL)
                    {
                        delete dht22;
                        dht22 = NULL;
                    }
                    return app_log_not_enough_heap;
                }
            }
            else
            {
                return app_log_already_running;
            }
        }
        if (sensor_id == DHT_HUMI_ID)
        {
            // check if another logger is running
            if (dht22_humi_logger == NULL)
            {
                dht22_humi_logger = new D_logger(&dht22->humidity,
                                                 poll_period,
                                                 desc,
                                                 action,
                                                 start_at_boot);
                if (dht22_humi_logger == NULL)
                {
                    if (dht22_temp_logger == NULL)
                    {
                        delete dht22;
                        dht22 = NULL;
                    }
                    return app_log_not_enough_heap;
                }
            }
            else
            {
                return app_log_already_running;
            }
        }
        break;
    default:
        return app_log_bad_id;
    }
    return app_log_ok;
}

void remove_cfg(int sensor_id)
{
    esplog.all("%s\n", __FUNCTION__);
    // the file containing the logger configuration is named <sensor_id>_log.cfg
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    char filename[32];
    os_memset(filename, 0, 32);
    os_sprintf(filename, "%d_log.cfg", sensor_id);

    if (Ffile::exists(&espfs, filename))
    {
        Ffile cfgfile(&espfs, filename);
        cfgfile.remove();
    }
}

app_log_res app_logger_stop(int sensor_id)
{
    esplog.all("app_logger_stop\n");
    remove_cfg(sensor_id);
    switch (sensor_id)
    {
    case MAX6675_ID:
        if (max6675_logger)
        {
            delete max6675_logger;
            max6675_logger = NULL;
        }
        if (max6675)
        {
            delete max6675;
            max6675 = NULL;
        }
        break;
    case DHT_TEMP_ID:
        if (dht22_temp_logger)
        {
            delete dht22_temp_logger;
            dht22_temp_logger = NULL;
        }
        if (dht22)
        {
            if (dht22_humi_logger == NULL)
            {
                delete dht22;
                dht22 = NULL;
            }
        }
        break;
    case DHT_HUMI_ID:
        if (dht22_humi_logger)
        {
            delete dht22_humi_logger;
            dht22_humi_logger = NULL;
        }
        if (dht22)
        {
            if (dht22_temp_logger == NULL)
            {
                delete dht22;
                dht22 = NULL;
            }
        }
        break;
    default:
        return app_log_bad_id;
    }
    return app_log_ok;
}

app_log_res app_logger_is_active(int sensor_id)
{
    esplog.all("app_logger_is_active\n");
    switch (sensor_id)
    {
    case MAX6675_ID:
        if (max6675_logger)
            return app_log_active;
        else
            return app_log_idle;
    case DHT_TEMP_ID:
        if (dht22_temp_logger)
            return app_log_active;
        else
            return app_log_idle;
    case DHT_HUMI_ID:
        if (dht22_humi_logger)
            return app_log_active;
        else
            return app_log_idle;
    default:
        return app_log_bad_id;
    }
}

app_log_res app_logger_get_sensor_info(int sensor_id, sensor_t *sensor)
{
    esplog.all("app_logger_get_sensor_info\n");
    bool delete_sensor_class = false;
    switch (sensor_id)
    {
    case MAX6675_ID:
        if (max6675 == NULL)
        {
            max6675 = new Max6675(MAX6675_CS, MAX6675_SCK, MAX6675_SO, MAX6675_ID, 0, MAX6675_BUFFERS);
            if (max6675 == NULL)
                return app_log_not_enough_heap;
            delete_sensor_class = true;
        }
        max6675->getSensor(sensor);
        if (delete_sensor_class)
        {
            delete max6675;
            max6675 = NULL;
        }
        break;
    case DHT_TEMP_ID:
        if (dht22 == NULL)
        {
            dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
            if (dht22 == NULL)
                return app_log_not_enough_heap;
            delete_sensor_class = true;
        }
        dht22->temperature.getSensor(sensor);
        if (delete_sensor_class)
        {
            delete dht22;
            dht22 = NULL;
        }
        break;
    case DHT_HUMI_ID:
        if (dht22 == NULL)
        {
            dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
            if (dht22 == NULL)
                return app_log_not_enough_heap;
            delete_sensor_class = true;
        }
        dht22->humidity.getSensor(sensor);
        if (delete_sensor_class)
        {
            delete dht22;
            dht22 = NULL;
        }
        break;
    default:
        return app_log_bad_id;
    }
    return app_log_ok;
}

app_log_res app_logger_async_read(int sensor_id, struct espconn *ptr_espconn)
{
    esplog.all("app_logger_read\n");
    struct sensor_rd_completed *param = new (struct sensor_rd_completed);
    if (param == NULL)
        return app_log_not_enough_heap;
    param->sensor_id = sensor_id;
    param->p_espconn = ptr_espconn;
    param->delete_sensor_class = false;
    switch (sensor_id)
    {
    case MAX6675_ID:
        if (max6675 == NULL)
        {
            max6675 = new Max6675(MAX6675_CS, MAX6675_SCK, MAX6675_SO, MAX6675_ID, 0, MAX6675_BUFFERS);
            if (max6675 == NULL)
                return app_log_not_enough_heap;
            param->delete_sensor_class = true;
        }
        param->sensor = max6675;
        max6675->force_reading(app_logger_read_completed, param);
        break;
    case DHT_TEMP_ID:
        if (dht22 == NULL)
        {
            dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
            if (dht22 == NULL)
                return app_log_not_enough_heap;
            param->delete_sensor_class = true;
        }
        param->sensor = &dht22->temperature;
        dht22->temperature.force_reading(app_logger_read_completed, param);
        break;
    case DHT_HUMI_ID:
        if (dht22 == NULL)
        {
            dht22 = new Dht(DHT_DATA, DHT22, DHT_TEMP_ID, DHT_HUMI_ID, 0, DHT_BUFFERS);
            if (dht22 == NULL)
                return app_log_not_enough_heap;
            param->delete_sensor_class = true;
        }
        param->sensor = &dht22->humidity;
        dht22->humidity.force_reading(app_logger_read_completed, param);
        break;
    default:
        return app_log_bad_id;
    }
    return app_log_ok;
}

// sensor_t sensor;
// max6675->getSensor(&sensor);
// print_sensor(&sensor);
// max6675->force_reading(get_and_print_max6675_event, NULL);

void poll_timer_cb(class D_logger *logger_ptr)
{
    esplog.all("poll_timer_cb\n");
    logger_ptr->_sensor->force_reading((void (*)(void *))data_logger_functions[logger_ptr->_read_completed_func_id], logger_ptr);
    if (logger_ptr->_first_call)
    {
        logger_ptr->_first_call = false;
        os_timer_arm(&logger_ptr->_poll_timer, logger_ptr->_poll_period, 1);
    }
}

void app_logger_delete_class(int sensor_id)
{
    switch (sensor_id)
    {
    case MAX6675_ID:
        if (max6675)
        {
            delete max6675;
            max6675 = NULL;
        }
        break;
    case DHT_TEMP_ID:
        if (dht22)
        {
            if (dht22_humi_logger == NULL)
            {
                delete dht22;
                dht22 = NULL;
            }
        }
        break;
    case DHT_HUMI_ID:
        if (dht22)
        {
            if (dht22_temp_logger == NULL)
            {
                delete dht22;
                dht22 = NULL;
            }
        }
        break;
    default:
        break;
    }
}

void get_cfg_filename(D_logger *logger, char *filename)
{
    esplog.all("%s\n", __FUNCTION__);
    os_memset(filename, 0, 32);
    sensor_t sensor;
    logger->_sensor->getSensor(&sensor);
    os_sprintf(filename, "%d_log.cfg", sensor.sensor_id);
}

bool saved_cfg_not_updated(D_logger *logger)
{
    esplog.all("%s\n", __FUNCTION__);
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return true;
    }
    char filename[32];
    get_cfg_filename(logger, filename);

    File_to_json cfgfile(filename);
    if (cfgfile.exists())
    {
        // sensor_id
        if (cfgfile.find_string("sensor_id"))
        {
            esplog.error("%s - cannot find sensor_id\n", __FUNCTION__);
            return true;
        }
        sensor_t sensor;
        logger->_sensor->getSensor(&sensor);
        if (sensor.sensor_id != atoi(cfgfile.get_value()))
            return true;
        // period
        if (cfgfile.find_string("period"))
        {
            esplog.error("%s - cannot find period\n", __FUNCTION__);
            return true;
        }
        if (logger->_poll_period != atoi(cfgfile.get_value()))
            return true;
        // desc
        if (cfgfile.find_string("desc"))
        {
            esplog.error("%s - cannot find desc\n", __FUNCTION__);
            return true;
        }
        if (os_strcmp(logger->_desc, cfgfile.get_value()))
            return true;
        // action_id
        if (cfgfile.find_string("action_id"))
        {
            esplog.error("%s - cannot find action_id\n", __FUNCTION__);
            return true;
        }
        if (logger->_read_completed_func_id != atoi(cfgfile.get_value()))
            return true;
        // start_at_boot
        if (cfgfile.find_string("start_at_boot"))
        {
            esplog.error("%s - cannot find start_at_boot\n", __FUNCTION__);
            return true;
        }
        if (sensor.sensor_id != atoi(cfgfile.get_value()))
            return true;
        return false;
    }
    espmem.stack_mon();
    return true;
    // {
    //   "sensor_id": 12000,
    //   "period": 60000,
    //   "desc": "test",
    //   "action_id": 1,
    //   "start_at_boot": 1
    // }
}

void remove_cfg(D_logger *logger)
{
    esplog.all("%s\n", __FUNCTION__);
    // the file containing the logger configuration is named <sensor_id>_log.cfg
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    char filename[32];
    get_cfg_filename(logger, filename);
    if (Ffile::exists(&espfs, filename))
    {
        Ffile cfgfile(&espfs, filename);
        cfgfile.remove();
    }
}

void save_cfg(D_logger *logger)
{
    esplog.all("%s\n", __FUNCTION__);
    if (saved_cfg_not_updated(logger))
        remove_cfg(logger);
    else
        return;
    if (!espfs.is_available())
    {
        esplog.error("%s - file system not available\n", __FUNCTION__);
        return;
    }
    char filename[32];
    get_cfg_filename(logger, filename);
    Ffile cfgfile(&espfs, filename);
    if (!cfgfile.is_available())
    {
        esplog.error("%s - cannot open %s\n", __FUNCTION__, filename);
        return;
    }
    int file_len = 80 + 5 + 9 + os_strlen(logger->_desc) + 5 + 1;
    Heap_chunk buffer(file_len);
    if (buffer.ref == NULL)
    {
        esplog.error("%s - not enough heap memory available (%d)\n", __FUNCTION__, file_len);
        return;
    }
    sensor_t sensor;
    logger->_sensor->getSensor(&sensor);

    os_sprintf(buffer.ref,
               "{\"sensor_id\": %d,\"period\": %d,\"desc\": \"%s\",\"action_id\": %d,\"start_at_boot\": %d}",
               sensor.sensor_id,
               logger->_poll_period,
               logger->_desc,
               logger->_read_completed_func_id,
               logger->_start_at_boot);
    cfgfile.n_append(buffer.ref, os_strlen(buffer.ref));
    espmem.stack_mon();
}

// D_logger::D_logger(class Esp8266_Sensor *sensor, int period, char *desc,
//                    void (*read_completed_func)(D_logger *),
//                    bool start_at_boot)
D_logger::D_logger(class Esp8266_Sensor *sensor, int period, char *desc,
                   app_log_action read_completed_func_id,
                   bool start_at_boot)
{
    esplog.all("D_logger::D_logger\n");
    _first_call = true;
    _sensor = sensor;
    _poll_period = period;
    if (_poll_period <= 0)
        _poll_period = 900000;
    _read_completed_func_id = read_completed_func_id;
    _start_at_boot = start_at_boot;
    os_memset(_desc, 0, sizeof(_desc));
    os_strncpy(_desc, desc, 32);
    os_timer_disarm(&_poll_timer);
    os_timer_setfn(&_poll_timer, (os_timer_func_t *)poll_timer_cb, this);
    os_timer_arm(&_poll_timer, 1000, 0);
    if (start_at_boot)
        save_cfg(this);
    else
        remove_cfg(this);
}

D_logger::~D_logger()
{
    esplog.all("D_logger::~D_logger\n");
    os_timer_disarm(&_poll_timer);
}