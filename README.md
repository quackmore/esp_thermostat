# ESP8266 digital I/O and common sensors library for NON_OS SDK

## Summary

Library for ESP8266 digital I/O and compatible sensors.
Based on Espressif NON-OS SDK.
No other dependencies.

The repository contains a full environment to compile, run and test the library.

To import the library in your project as a library use the following file:

+ lib/liblibrary.a

To import the library source files use files into src/library.

The library include files are:

+ esp8266_io.h
+ library_common_types.hpp
+ library_dht.hpp
+ library_di_sequence.h
+ library_dio_task.h
+ library_do_sequence.h
+ library_max6675.hpp
+ library_sensor.hpp
+ library.h
+ library.hpp

Features:

+ digital output pulse sequences
+ digital input pulse sequence acquisition
+ DHT temperature and humidity sensors
+ MAX6675 temperature sensor

more to come ...

## Building the binaries and flashing ESP8266

Needed:
+[Espressif NON-OS SDK](https://github.com/espressif/ESP8266_NONOS_SDK) in a separate repository.
+[esp-open-sdk toolchain](https://github.com/pfalcon/esp-open-sdk) in a separate repository; build the bare Xtensa toolchain and leave ESP8266 SDK separate using:

      $ make STANDALONE=n
 

Build steps (linux)
+Clone the repository.
+Customize build variables according to your ESP8266 module: 
      
      $ cd <your path>/espbot
      $ ./gen_env.sh

      this will generate a env.sh file
      for instance a WEMOS D1 mini file will look like this:
      
      export SDK_DIR=<your path to ESP8266_NONOS_SDK>
      export COMPILE=gcc
      export BOOT=new
      export APP=1
      export SPI_SPEED=40
      export SPI_MODE=DIO
      export SPI_SIZE_MAP=4

+Build with
  
      $ . env.sh
      $ make

+Flash ESP8266 using esptool.py (checkout you distribution packages or [github repository](https://github.com/espressif/esptool))
  
      this is an example that works for WEMOS D1 mini, customize memory addresses according to your flash size
      
      Obtaining a clean starting point
      $ esptool.py erase_flash
      
      Initizalizing flash for ESP SDK
      $ esptool.py --port /dev/ttyUSB0 write_flash -fm dio -fs 32m -ff 40m 0x00000 <your path to ESP NONOS SDK>/bin/boot_v1.7.bin 
      $ esptool.py --port /dev/ttyUSB0 write_flash -fm dio -fs 32m -ff 40m 0x3FB000 <your path to ESP NONOS SDK>/bin/blank.bin
      $ esptool.py --port /dev/ttyUSB0 write_flash -fm dio -fs 32m -ff 40m 0x3FC000 <your path to ESP NONOS SDK>/bin/esp_init_data_default_v08.bin
      $ esptool.py --port /dev/ttyUSB0 write_flash -fm dio -fs 32m -ff 40m 0x3FE000 <your path to ESP NONOS SDK>/bin/blank.bin
      
      Flashing espbot code
      $ cd <your path>/espbot
      $ esptool.py --port /dev/ttyUSB0 write_flash -fm dio -fs 32m -ff 40m 0x01000 bin/upgrade/user1.4096.new.4.bin

## License

The library comes with a [BEER-WARE] license.

Enjoy.
