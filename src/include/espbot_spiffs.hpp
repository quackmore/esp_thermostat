/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __ESPBOT_SPIFFS_HPP__
#define __ESPBOT_SPIFFS_HPP__

extern "C"
{
#include "user_config.h"
}

/**
 * @brief ESP8266 and SDK references
 * 
 * using macros SPI_FLASH_SIZE_MAP and SYSTEM_PARTITION_RF_CAL_ADDR
 * for calculating SPIFFS basics (below)
 * 
 */
#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "SPIFFS not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#error "SPIFFS not supported"
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_DATA 0x101000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_DATA 0x101000
#elif (SPI_FLASH_SIZE_MAP == 5)
#error "SPIFFS not supported"
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_DATA 0x201000
#else
#error "The flash map is not supported"
#endif

/**
 * @brief SPIFFS basics
 * 
 * logical page buffer:     512 bytes
 * file descriptor buffer:  128 bytes =>  768 bytes total without cache
 * cache:                  1152 bytes => 1792 bytes total with cache
 * Note: to get the exact amount of bytes needed on your specific target,
 * enable SPIFFS_BUFFER_HELP in spiffs_config.h, rebuild and call:
 *   SPIFFS_buffer_bytes_for_filedescs
 *   SPIFFS_buffer_bytes_for_cache
 * 
 */
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
#include "spiffs.h"

  // flash memory functions (checkout SPIFFS doumentation)
  s32_t esp_spiffs_read(u32_t t_addr, u32_t t_size, u8_t *t_dst);
  s32_t esp_spiffs_write(u32_t t_addr, u32_t t_size, u8_t *t_src);
  s32_t esp_spiffs_erase(u32_t t_addr, u32_t t_size);
}

/**
 * @brief Espbot file systems methods
 * 
 */

/**
 * @brief mount the file system
 * 
 */
void esp_spiffs_mount(void);
/**
 * @brief get spiffs total size
 * 
 * @return u32_t the FS total size in in bytes
 */
u32_t esp_spiffs_total_size();
/**
 * @brief get spiffs used size
 * 
 * @return u32_t the FS used size in in bytes
 */
u32_t esp_spiffs_used_size();
/**
 * @brief check the file system
 * 
 * @return s32_t result of the check
 */
s32_t esp_spiffs_check();
/**
 * @brief list FS files
 * 
 * SPIFFS does not have a directory structure.
 * All files are into root.
 * The function returns a file descriptor at a time, so it is required
 * to call it multiple times to scan the entire directory (until it returns NULL)
 * 
 * @param file_idx This function works as an iterator:
 *                 at first time call it with file_idx 0 (first file)
 *                 next time call it with file_idx 1 (next file)
 * 
 * @return struct spiffs_dirent* a file descriptor
 */
struct spiffs_dirent *esp_spiffs_list(int file_idx);

#define SPIFFSESP_ERR_NOTCONSISTENT -10500
/**
 * @brief Espbot file class
 * 
 */
class Espfile
{
public:
  /**
   * @brief Construct a new Espfile object
   * will open 'filename' or create if it does not exist
   * ready for reading or writing (append)
   *
   * @param filename 
   */
  Espfile(char *filename);

  /**
   * @brief Destroy the Espfile object
   * will close the file, according to documentation 
   * closing a file will flush the file cache
   * 
   */
  ~Espfile();

  /**
   * @brief read from file
   * same as SPIFFS_read
   * will read len bytes from file to buffer
   * @param buffer, where the read bytes are copied
   * @param len, how many bytes to be read
   * @return s32_t, on success: the read bytes count
   *                on failure: the SPIFFS error code (negative number)
   */
  s32_t n_read(char *buffer, int len);

  /**
   * @brief read from file with offset
   * same as SPIFFS_lseek + SPIFFS_read
   * will read len bytes from file, starting from offset position, to buffer
   * @param buffer, where the read bytes are copied
   * @param offset, where to start reading from file
   * @param len, how many bytes to be read
   * @return s32_t, on success: the read bytes count
   *                on failure: the SPIFFS error code (negative number)
   */
  s32_t n_read(char *buffer, int offset, int len);

  /**
   * @brief read from file with offset
   * same as SPIFFS_write
   * will write len bytes from buffer to file
   * @param buffer, where the bytes are read
   * @param len, how many bytes to be read
   * @return s32_t, on success: the written bytes count
   *                on failure: the SPIFFS error code (negative number)
   */
  s32_t n_append(char *buffer, int len);

  /**
   * @brief clear file content
   * file length truncated to zero using SPIFFS_TRUNC
   * @return s32_t, on success: SPIFFS_OK
   *                on failure: the SPIFFS error code (negative number)
   */
  s32_t clear();

  /**
   * @brief read from file with offset
   * same as SPIFFS_fremove
   * @return s32_t, on success: SPIFFS_OK
   *                on failure: the SPIFFS error code (negative number)
   */
  s32_t remove();

  /**
   * @brief check if 'filename' exists
   * making it static does not require creating an object
   * @param filename 
   * @return true when the file exists 
   * @return false when the file does not exists or on error
   */
  static bool exists(char *filename);

  /**
   * @brief get the file size
   * making it static does not require creating an object
   * @param filename 
   * @return int, the file size in bytes 
   */
  static int size(char *filename);

private:
  spiffs_file _handler;
  char _name[32]; // SPIFFS filename max length is 32 (including the terminating \0)
  s32_t _err;
};

/**
 * @brief Espfile EXAMPLE
 * 
 * 
 * char *line = "this is a log line";
 * Espfile logfile("logfile.log");
 * s32_t res = logfile.n_append(line, os_strlen(line));
 * 
 * if (res < SPIFFS_OK)
 *     .. there was an error ..
 * 
 * 
 */

#endif