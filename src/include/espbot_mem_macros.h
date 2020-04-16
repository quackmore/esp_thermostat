/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __MEM_MACROS_H__
#define __MEM_MACROS_H__

#define IRAM __attribute__((section(".iram.text")))
#define IROM_TEXT __attribute__((section(".irom.text")))
#define ALIGNED_4 __attribute__((aligned(4)))

// MACROS FOR PLACING STRINGS INTO FLASH MEMORY

// using fs_sprintf keep fmt len under 70 chars
// otherwise a read exception will occur
#define fs_sprintf(buf, fmt, ...)                            \
  do                                                         \
  {                                                          \
    static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt; \
    os_sprintf_plus(buf, flash_str, ##__VA_ARGS__);          \
  } while (0)

#define fs_snprintf(buf, len, fmt, ...)                            \
  do                                                         \
  {                                                          \
    static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt; \
    os_snprintf_plus(buf, len, flash_str, ##__VA_ARGS__);          \
  } while (0)


#define fs_printf(fmt, ...)                                  \
  do                                                         \
  {                                                          \
    static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt; \
    os_printf_plus(flash_str, ##__VA_ARGS__);                \
  } while (0)

#define f_str(str) ({ static const char flash_str[] IROM_TEXT ALIGNED_4 = str; &flash_str[0]; })

#endif