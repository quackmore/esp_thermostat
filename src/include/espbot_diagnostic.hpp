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
}

#include "espbot_mem_macros.h"

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

#define DIA_LED ESPBOT_D4 // D4 will be configured as output and will be in use by espbot
                          // unless _diag_led_mask is set to 0

struct dia_event
{
  uint32 timestamp;
  char ack;
  char type;
  int code;
  uint32 value;
};

class Espbot_diag
{
private:
  struct dia_event _evnt_queue[EVNT_QUEUE_SIZE];
  uint32 _uart_0_bitrate;
  bool _sdk_print_enabled;
  int _event_count;
  char _last_event;
  char _diag_led_mask; // bitmask 00?? ????
                       //           || ||||_ 1 -> show FATAL events on led
                       //           || |||__ 1 -> show ERROR events on led
                       //           || ||___ 1 -> show WARNING events on led
                       //           || |____ 1 -> show INFO events on led
                       //           ||______ 1 -> show DEBUG events on led
                       //           |_______ 1 -> show TRACE events on led
                       // set _diag_led_mask to 0 will avoid espbot using any led
  void add_event(char type, int code, uint32 value);

  int restore_cfg(void);           // return CFG_OK on success, otherwise CFG_ERROR
  int saved_cfg_not_updated(void); // return CFG_OK when cfg does not require update
                                   // return CFG_REQUIRES_UPDATE when cfg require update
                                   // return CFG_ERROR otherwise

public:
  Espbot_diag(){};
  ~Espbot_diag(){};

  // easy access
  char _serial_log_mask; // bitmask 0??? ????
                         //          ||| ||||_ 1 -> show FATAL events on UART_0
                         //          ||| |||__ 1 -> show ERROR events on UART_0
                         //          ||| ||___ 1 -> show WARNING events on UART_0
                         //          ||| |____ 1 -> show INFO events on UART_0
                         //          |||______ 1 -> show DEBUG events on UART_0
                         //          ||_______ 1 -> show TRACE events on UART_0
                         //          |________ 1 -> show ALL events on UART_0

  void init_essential(void);
  void init_custom(void);

  void fatal(int code, uint32 value = 0); // calling one of these functions
  void error(int code, uint32 value = 0); // will add a new event to the event queue
  void warn(int code, uint32 value = 0);  // the new event will be marked as
  void info(int code, uint32 value = 0);  // not acknoledged
  void debug(int code, uint32 value = 0); // and the diagnostic led will be turned on
  void trace(int code, uint32 value = 0); // (if enabled)

  int get_max_events_count(void);           // useful for iterations
  struct dia_event *get_event(int idx = 0); // return a pointer to an event (NULL if no event exists)
                                            // idx=0 -> last event
                                            // idx=1 -> previous event
  int get_unack_events(void);               // return the count of not acknoeledge events
  void ack_events(void);                    // acknoledge all the saved events
  char get_led_mask(void);                  //
  void set_led_mask(char);                  //
  char get_serial_log_mask(void);
  void set_serial_log_mask(char);
  uint32 get_uart_0_bitrate(void);
  bool set_uart_0_bitrate(uint32); // return false when input is a wrong bitrate
  bool get_sdk_print_enabled(void);
  void set_sdk_print_enabled(bool);
  int save_cfg(void); // return CFG_OK on success, otherwise CFG_ERROR
};

#define FATAL(fmt, ...)                                             \
  do                                                                \
  {                                                                 \
    if (esp_diag._serial_log_mask & EVNT_FATAL)                     \
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
    if (esp_diag._serial_log_mask & EVNT_ERROR)                     \
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
    if (esp_diag._serial_log_mask & EVNT_WARN)                      \
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
    if (esp_diag._serial_log_mask & EVNT_INFO)                      \
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
    if (esp_diag._serial_log_mask & EVNT_DEBUG)                     \
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
    if (esp_diag._serial_log_mask & EVNT_TRACE)                     \
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
    if (esp_diag._serial_log_mask & EVNT_ALL)                       \
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