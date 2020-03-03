/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __SPIFFS_FLASH_FUNTIONS_H__
#define __SPIFFS_FLASH_FUNTIONS_H__

// ESP8266 and SDK references
#include "user_config.h" // just for macros SPI_FLASH_SIZE_MAP and SYSTEM_PARTITION_RF_CAL_ADDR
                         // used for calculating spiffs size and location

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "There is no room for spiffs"
#elif (SPI_FLASH_SIZE_MAP == 2)
#error "There is no room for spiffs"
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_DATA 0x101000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_DATA 0x101000
#elif (SPI_FLASH_SIZE_MAP == 5)
#error "There is no room for spiffs"
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_DATA 0x201000
#else
#error "The flash map is not supported"
#endif

#define LOG_PAGE_SIZE (256)
#define FLASH_SECT_SIZE (1024 * 4)
#define FS_START SYSTEM_PARTITION_DATA
#define FS_END SYSTEM_PARTITION_RF_CAL_ADDR
#define FS_ALIGN_BYTES 4
#define SPIFFS_FLASH_RESULT_ERR -10200
#define SPIFFS_FLASH_RESULT_TIMEOUT -10201
#define SPIFFS_FLASH_BOUNDARY_ERROR -10202

extern "C"
{
#include "spiffs.h" // for spiffs types

// flash memory functions (checkout SPIFFS doumentation)
s32_t esp_spiffs_read(u32_t t_addr, u32_t t_size, u8_t *t_dst);
s32_t esp_spiffs_write(u32_t t_addr, u32_t t_size, u8_t *t_src);
s32_t esp_spiffs_erase(u32_t t_addr, u32_t t_size);
}

#endif