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

// ESP8266 and SDK references
extern "C"
{
#include "spiffs_flash_functions.hpp"
#include "spiffs.h"
}

// file system possible status
typedef enum
{
  FFS_AVAILABLE = 222,
  FFS_UNMOUNTED,   // gracefully unmounted
  FFS_UNAVAILABLE, // some FATAL error occurred while mounting of formatting
  FFS_ERRORS,      // a file system check found errors
  FFS_NOT_INIT
} flashfs_status;

// The file system class
class Flashfs
{
protected:
  u8_t m_work[LOG_PAGE_SIZE * 2]; // ram memory buffer being double the size of the logical page size
  u8_t m_fd_space[32 * 4];        // 4 file descriptors => 4 file opened simultaneously
  // To enable/disable cache change <#define SPIFFS_CACHE 1> in spiffs_config.h
#if SPIFFS_CACHE
  u8_t m_cache[(LOG_PAGE_SIZE + 32) * 4];
#else
  u8_t m_cache[1];
#endif
  // logical page buffer:     512 bytes
  // file descriptor buffer:  128 bytes =>  768 bytes total without cache
  // cache:                  1152 bytes => 1792 bytes total with cache
  // TIP: To get the exact amount of bytes needed on your specific target,
  // enable SPIFFS_BUFFER_HELP in spiffs_config.h, rebuild and call:
  //   SPIFFS_buffer_bytes_for_filedescs
  //   SPIFFS_buffer_bytes_for_cache

  spiffs m_fs;            // file system handler
  spiffs_config m_config; // file system configuration

  flashfs_status status;

public:
  Flashfs();
  ~Flashfs(){};
  void init(void);    // will mount the file system (eventually formatting it)
  void format(void);  // will clean everything, a new init is required after this
  void unmount(void); // thinking about a controlled reset of the target
                      // when you want to gracefully shutdown ...
  s32_t check();      // same as SPIFFS_check

  spiffs *get_handler();
  struct spiffs_dirent *list(int); // (0) => return first file information
                                   // (1) => return next file information

  flashfs_status get_status();
  bool is_available();

  s32_t last_error();
  u32_t get_total_size();
  u32_t get_used_size();
};

// file possible status
typedef enum
{
  FFS_F_UNAVAILABLE = 444, // not open yet or fatal error while opening
  FFS_F_OPEN,
  FFS_F_MODIFIED_UNSAVED,
  FFS_F_REMOVED
} flashfs_file_status;

// the file class
class Ffile
{
private:
  Flashfs *m_fs;
  spiffs_file m_fd;
  char m_name[32];
  flashfs_file_status status;

public:
  Ffile(Flashfs *);                       // not sure if really useful but just in case ...
  Ffile(Flashfs *t_fs, char *t_filename); // will open 'filename' from filesytem t_fs
                                          // (or create if it does not exist)
                                          // ready for reading or writing (append)
  ~Ffile();                               // will close the file
                                          // according to documentation closing a file
                                          // will flush the file cache

  char *get_name();
  void open(char *); // not sure if really useful but just in case ...
                     // if the file was originally opened with a different filename
                     // it will be closed
                     // then the new filename will be open

  flashfs_file_status get_status(); // who cares if you have is_available() ...
  bool is_available();

  int n_read(char *t_buffer, int t_len);   // same as SPIFFS_read
                                           // will read t_len bytes from file to t_buffer
                                           // returns number of bytes read, or -1 if error
  int n_read(char *t_buffer, int offset, int t_len);   // same as SPIFFS_lseek + SPIFFS_read
                                                       // will read t_len bytes 
                                                       // from file + offset to t_buffer
                                                       // returns number of bytes read, or -1 if error
  int n_append(char *t_buffer, int t_len); // same as SPIFFS_write
                                           // will write t_len bytes from t_buffer to file
                                           // returns number of bytes written, or -1 if error
  

  void clear(); // clear the whole file content
  void remove();
  void flush_cache();         // not sure if really useful but just in case ...
  static bool exists(Flashfs *, char *); // making it static does not require creating an object before
  static int size(Flashfs *, char *);    // making it static does not require creating an object before 
};

/*  EXAMPLE EXAMPLE EXAMPLE 
    ...
    Flashfs fs;
    fs.init();
    if (fs.is_available)
    {
      Ffile cfg(&fs, "config.cfg"); // constructor will open or create the file
      if (cfg.is_available())
      {
        cfg.n_append("this configuration",19);
      }
    } // destructor will close the file
    ...
*/

#endif