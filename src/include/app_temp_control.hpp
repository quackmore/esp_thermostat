/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_TEMP_CONTROL_HPP__
#define __APP_TEMP_CONTROL_HPP__

#define MODE_OFF 0
#define MODE_MANUAL 1
#define MODE_AUTO 2

void temp_control_init(void);
void ctrl_off(void);
void ctrl_manual(int heater_on_period, int heater_off_period, int stop_after);

void temp_control_run(struct date *);

#endif