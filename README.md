# ESP THERMOSTAT

## Summary

Thermostat app based on [ESPBOT](https://github.com/quackmore/espbot_2.0) and [esp8266-library](https://github.com/quackmore/esp8266-library).

The thermostat APP require a DHT-22 for temperature measurement (on pin D5) and a relay (on pin D2) for controlling an external heater.

The thermostat APP has three working modes:
-OFF
-MANUAL: when working in manual mode the thermostat can keep the heater always ON or can cycle it ON and OFF regardless of the temperature readings.
-AUTO: when working in auto mode the thermostat will drive the heater trying to reach the temperature setpoint. A PID algorithm is used.

The thermostat APP can log any change in temperature reading, working mode and heater status to an external host using the following format:
  POST your_path HTTP/1.1
  Host: your_host_ip
  Content-Type: application/json
  Accept: */*
  Connection: keep-alive
  Content-Length: 47
  {"timestamp":4294967295,"type":1,"value":1234}

The thermostat APP can be controlled using a web interface connecting to your_device_IP_address.

## Building the APP

(build commands are available as VS tasks)

- Configure the environment running the command "./gen_env.sh"
- Build user1.bin running the command "source ./env.sh && make -e APP=1 all"
- Build user2.bin running the command "source ./env.sh && make -e APP=2 all"

## Setup the device

### Flashing

(flash commands are available as VS tasks)

- Erase the flash running the command "source ./env.sh && make flash_erase"
- Init the flash (flashing SDK params) running the command "source ./env.sh && make flash_init"
- Flash the boot running the command "source ./env.sh && make flash_boot"

### Create a minimum configuration (and checkout some useful commands)

- wifi connection:
  (without configuration the ESP device will work as a Wifi AP with SSID=ESPBOT-chip_id and password=espbot123456)
  curl --location --request POST "http://{{host}}/api/wifi/cfg" \
  --data "{
      \"station_ssid\": \"your_Wifi_SSID\",
      \"station_pwd\": \"your_Wifi_password\"
  }"
  this will make the device stop working as AP and connect to your Wifi AP
- device name (and SSID)
  curl --location --request POST "http://{{host}}/api/espbot/cfg" \
  --header "Content-Type: application/json" \
  --data "{
      \"espbot_name\": \"your_device_name\"
  }"
- debug logger config
  curl --location --request POST "http://{{host}}/api/debug/cfg" \
  --header "Content-Type: application/json" \
  --data "{
      \"logger_serial_level\": your_level,
      \"logger_memory_level\": your_level
  }"
  logger levels:
  0 OFF    no logging
  1 FATAL  up to fatal events
  2 ERROR  up to error events
  3 WARN   up to warning events
  4 INFO   up to information events
  5 DEBUG  up to debug events
  6 TRACE  up to trace events
  7 ALL    all the events
  
  serial_levels state the information printed to the serial line (BIT_RATE_460800)
  memory_levels state the information kept in memory
  
  use following command to checkout the memory log:
  curl --location --request GET "http://{{host}}/api/debug/log"
- OTA upgrade
  curl --location --request POST "http://{{host}}/api/ota/cfg" \
  --header "Content-Type: application/json" \
  --data "{
      \"host\": \"your_server\",
      \"port\": your_port,
      \"path\": \"your_path\",
      \"check_version\": \"false\",
      \"reboot_on_completion\": \"true\"
  }"

  you will need a webserver where to place the user1.bin and user2.bin files
  the easiest way is using docker:
  docker run -d --name esp8266-http-upgrade -p 80:80 -v $(pwd)/bin/upgrade/www:/usr/share/nginx/html:ro nginx:alpine
  
  start an OTA upgrade with the command
  curl --location --request POST "http://{{host}}/api/ota/upgrade"
- useful commands:
  get device information:
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
  get memory information:
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
  get last reset information:
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
- run "espUploadFile 'your device IP address' esp_queries.js"

## License

The app comes with a [BEER-WARE] license.

Enjoy.
