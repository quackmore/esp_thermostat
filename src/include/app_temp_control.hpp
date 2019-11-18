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
void ctrl_auto(int set_point, int stop_after);


void temp_control_run(struct date *);

//
// get control settings & vars
//
int get_current_mode(void);
int get_pwr_off_timer(void);
uint32 get_pwr_off_timer_started_on(void);
int get_auto_setpoint(void);
int get_manual_pulse_on(void);
int get_manual_pulse_off(void);

#endif