/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef ESPOT_HAL_H
#define ESPOT_HAL_H

#define STACK_TRACE_LEN 256  // uint32_t
#define RTC_STACKDUMP_LEN 20 // uint32_t

#ifdef __cplusplus
extern "C"
{
#endif

  void install_custom_exceptions();

  /*
  * system_restart_hook overrides the weak symbol in libmain.a 
  */
  void system_restart_hook();

  /*
  * get the stack pointer address recorded on last crash 
  * @return uint32_t - stack pointer address
  */
  uint32_t get_last_crash_SP();

  /*
  * get the stack dump content one value at a time
  * 
  * @idx - 0 -> the first stored value
  *      - 1 -> the next value (from last call)
  * @dest - the destination where to copy the value 
  * @return int - 0 - a valid value has been copied
  *             - 1 - no valid value has been copied
  *                   (went beyond the last value)
  */
  int get_last_crash_stack_dump(int idx, uint32_t *dest);

#ifdef __cplusplus
}
#endif

#endif
