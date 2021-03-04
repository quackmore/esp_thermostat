/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __DIAGNOSTIC_HPP__
#define __DIAGNOSTIC_HPP__

extern "C"
{
#include "c_types.h"
#include "osapi.h"
#include "espbot_mem_macros.h"
}

#define DIAG_LED_DISABLED 0x00

#define EVNT_NONE 0x00
#define EVNT_FATAL 0x01
#define EVNT_ERROR 0x02
#define EVNT_WARN 0x04
#define EVNT_INFO 0x08
#define EVNT_DEBUG 0x10
#define EVNT_TRACE 0x20
#define EVNT_ALL 0x40

#define EVNT_QUEUE_SIZE 100 // 100 * sizeof(strut dia_event) => 1200 bytes

#define DIA_LED ESPBOT_D4 // D4 will be configured as output and will be in use by espbot \
                          // unless _diag_led_mask is set to 0

struct dia_event
{
  uint32 timestamp;
  char ack;
  char type;
  int code;
  uint32 value;
};

void dia_init_essential(void);
void dia_init_custom(void);

// DIAGNOSTIC EVENTS
// diagnostic events are kept in memory and available through REST API
//
// ADDING EVENTS
void dia_fatal_evnt(int code, uint32 value = 0); // calling one of these functions
void dia_error_evnt(int code, uint32 value = 0); // will add a new event to the event queue
void dia_warn_evnt(int code, uint32 value = 0);  // the new event will be marked as
void dia_info_evnt(int code, uint32 value = 0);  // not acknoledged
void dia_debug_evnt(int code, uint32 value = 0); // and the diagnostic led will be turned on
void dia_trace_evnt(int code, uint32 value = 0); // (if enabled)
//
// GETTING AND ACKNOWLEDGING EVENTS
void dia_ack_events(void); // acknoledge all the saved events
int dia_get_max_events_count(void);
struct dia_event *dia_get_event(int idx);
int dia_get_unack_events(void);

// DIAGNOSTIC CONFIG
void dia_set_led_mask(char); //
void dia_set_serial_log_mask(char);
bool dia_set_uart_0_bitrate(uint32); // return false when input is a wrong bitrate
void dia_set_sdk_print_enabled(bool);
char *dia_cfg_json_stringify(char *dest = NULL, int len = 0);
int dia_cfg_save(void); // return CFG_OK on success, otherwise CFG_ERROR

// DIAGNOSTIC SERIAL LOG MACROS
bool diag_log_err_type(int);

#define FATAL(fmt, ...)                                             \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_FATAL))                              \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[F] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#define ERROR(fmt, ...)                                             \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_ERROR))                              \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[E] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#define WARN(fmt, ...)                                              \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_WARN))                               \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[W] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#define INFO(fmt, ...)                                              \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_INFO))                               \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[I] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#define DEBUG(fmt, ...)                                             \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_DEBUG))                              \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[D] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#define TRACE(fmt, ...)                                             \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_TRACE))                              \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[T] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#define ALL(fmt, ...)                                               \
  do                                                                \
  {                                                                 \
    if (diag_log_err_type(EVNT_ALL))                                \
    {                                                               \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "[A] "; \
        os_printf_plus(flash_str);                                  \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = fmt;    \
        os_printf_plus(flash_str, ##__VA_ARGS__);                   \
      }                                                             \
      {                                                             \
        static const char flash_str[] IROM_TEXT ALIGNED_4 = "\n";   \
        os_printf_plus(flash_str);                                  \
      }                                                             \
    }                                                               \
  } while (0)

#endif