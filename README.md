# ESP THERMOSTAT

## Summary

Thermostat app based on [ESPBOT](https://github.com/quackmore/espbot_2.0) and [esp8266-library](https://github.com/quackmore/esp8266-library).

The thermostat APP require a DHT-22 for temperature measurement (on pin D5) and a relay (on pin D2) for controlling an external heater.

The thermostat APP has three working modes:

- OFF
- MANUAL: when working in manual mode the thermostat can keep the heater always ON or can cycle it ON and OFF regardless of the temperature readings.
- AUTO: when working in auto mode the thermostat will drive the heater trying to reach the temperature setpoint. A PID algorithm is used.

The thermostat APP can log any change in temperature reading, working mode and heater status to an external host using the following format:

    POST your_path HTTP/1.1
    Host: your_host_ip
    Content-Type: application/json
    Accept: */*
    Connection: keep-alive
    Content-Length: 47
    {"timestamp":4294967295,"type":1,"value":1234}

  event types are:

    1 - temperature value changed
    2 - heater on/off
    3 - control mode change
    4 - temperature setpoint_change

The thermostat APP hosts a web server and can be controlled by a web interface connecting your browser to your_device_IP_address.

## Building the APP

(build commands are available as VS tasks)

- Configure the environment running the command

      ./gen_env.sh
- Build user1.bin running the command

      source ./env.sh && make -e APP=1 all
- Build user2.bin running the command

      source ./env.sh && make -e APP=2 all

## Setup the device

### Flashing

(flash commands are also available as VS tasks)

- Erase the flash running the command

      source ./env.sh && make flash_erase
- Init the flash (flashing SDK params) running the command

      source ./env.sh && make flash_init
- Flash the boot running the command

      source ./env.sh && make flash_boot
- Flash the application running the command

      source ./env.sh && make -e APP=1 flash
      source ./env.sh && make -e APP=2 flash

### Create a minimum configuration (and checkout some useful commands)

- wifi connection: without configuration the ESP device will work as a Wifi AP with SSID=ESPBOT-chip_id and password=espbot123456

      curl --location --request POST "http://{{host}}/api/wifi/cfg" \
      --data "{
          "station_ssid": "your_Wifi_SSID",
          "station_pwd": "your_Wifi_password"
      }"
  this will make the device stop working as AP and connect to your Wifi AP
- device name (and SSID)

      curl --location --request POST "http://{{host}}/api/espbot/cfg" \
      --header "Content-Type: application/json" \
      --data-raw '{
          "espbot_name": "your_device_name"
      }'
- enable cron (will run temperature reading and control)

      curl --location --request POST "http://{{host}}/api/cron" \
      --header "Content-Type: application/json" \
      --data-raw '{
          "cron_enabled": 1
      }'
- enable mDns (if you want to access your device as <http://your_device_name.local>)

      curl --location --request POST 'http://{{host}}/api/mdns' \
      --header 'Content-Type: application/json' \
      --data-raw '{
          "mdns_enabled": 1
      }'
- disable mDns (if you don't care)

      curl --location --request POST 'http://{{host}}/api/mdns' \
      --header 'Content-Type: application/json' \
      --data-raw '{
          "mdns_enabled": 0
      }'
- setup SNTP and timezone

      curl --location --request POST 'http://{{host}}/api/sntp' \
      --header 'Content-Type: application/json' \
      --data-raw '{
          "sntp_enabled": 1,
          "timezone": your_time_zone
      }'
- setup diagnostic reporting

      curl --location --request POST 'http://{{host}}/api/diag/cfg' \
      --header 'Content-Type: application/json' \
      --data-raw '{
          "diag_led_mask": 7,
          "serial_log_mask": 31
      }'

  examples:

      "diag_led_mask": FATAL | ERROR | WARN
      will report that a fatal, error or warning event occurred by lighting the esp led (D4)
      (acknowledging the events throught the web interface will turn off the led)

  logger levels:
  
      0 OFF    no logging
      1 FATAL  fatal events
      2 ERROR  error events
      4 WARN   warning events
      8 INFO   information events
      16 DEBUG debug events
      32 TRACE trace events
      64 ALL   all the events
  
  the serial_log_mask define which information is printed to the serial line (BIT_RATE_460800)
  you can checkout the diagnostic configuration using

      curl --location --request GET "http://{{host}}/api/debug/log"
- OTA upgrade

      curl --location --request POST 'http://{{host}}/api/ota/cfg' \
      --header 'Content-Type: application/json' \
      --data-raw '{
          "host": "your_server",
          "port": your_port,
          "path": "your_path",
          "check_version": "true",
          "reboot_on_completion": "true"
      }

  you will need a webserver where to place the user1.bin and user2.bin files
  the easiest way is using docker:

      docker run -d --name esp8266-http-upgrade -p 80:80 -v $(pwd)/bin/upgrade/www:/usr/share/nginx/html:ro nginx:alpine
  
  start an OTA upgrade with the command

      curl --location --request POST "http://{{host}}/api/ota/upgrade"
- useful commands:
  - get device information:

        curl --location --request GET "http://{{host}}/api/info"
        answer example:
        {
          "app_name": "APP_NAME",
          "app_version": "v0.2-5-g54311a5",
          "espbot_name": "DEVICE_NAME",
          "espbot_version": "v1.0-77-gf5ecd69",
          "library_version": "v1.0-19-gcbb0044",
          "chip_id": "2426995",
          "sdk_version": "3.1.0-dev(b1f42da)",
          "boot_version": "7"
        }
  - get memory information:

        curl --location --request GET "http://{{host}}/api/debug/meminfo"
        answer example:
        {
          "stack_max_addr": "3FFFFE00",
          "stack_min_addr": "3FFFF5F0",
          "heap_start_addr": "3FFF9000",
          "heap_free_size": 8784,
          "heap_max_size": 12288,
          "heap_min_size": 4880,
          "heap_objs": 39,
          "heap_max_objs": 50
        }
  - get last reset information:

        curl --location --request GET "http://{{host}}/api/debug/last_rst"
        answer example:
        {
          "date": "Wed Dec 11 09:53:32 2019",
          "reason": 4,
          "exccause": 0,
          "epc1": 0,
          "epc2": 0,
          "epc3": 0,
          "evcvaddr": 0,
          "depc": 0
        }

### Uploading web server files

Use the [espUploadFile](https://github.com/quackmore/esp_utils) bash script to upload files to the ESP8266 device

- cd to web directory
- run "espUploadFile 'your device IP address' index.html"
- run "espUploadFile 'your device IP address' html_dyn.js"
- run "espUploadFile 'your device IP address' event_codes.js"

## License

The app comes with a [BEER-WARE] license.

Enjoy.
