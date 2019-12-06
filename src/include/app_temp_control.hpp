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

// control advanced setting
#define CTRL_KP 1
#define CTRL_KD -5
#define CTRL_KI 5
#define CTRL_U_MAX 46
#define CTRL_HEATER_ON_MIN 2
#define CTRL_HEATER_ON_MAX 13
#define CTRL_HEATER_ON_OFF_P 15
#define COLD_HEATER 120
#define WARM_UP_PERIOD 12
#define CTRL_HEATER_ON_WUP 3
#define CTRL_HEATER_OFF_WUP 3

struct _adv_ctrl_settings
{
    int kp;
    int kd;
    int ki;
    int u_max;
    int heater_on_min;
    int heater_on_max;
    int heater_on_off;
    int heater_cold;
    int warm_up_period;
    int wup_heater_on;
    int wup_heater_off;
};

void temp_control_init(void);
void ctrl_off(void);
void ctrl_manual(int heater_on_period, int heater_off_period, int stop_after);
void ctrl_auto(int set_point, int stop_after);


void temp_control_run(void);

//
// get control settings & vars
//
int get_current_mode(void);
int get_pwr_off_timer(void);
uint32 get_pwr_off_timer_started_on(void);
int get_auto_setpoint(void);
int get_manual_pulse_on(void);
int get_manual_pulse_off(void);
void set_adv_ctrl_settings(struct _adv_ctrl_settings *);
struct _adv_ctrl_settings *get_adv_ctrl_settings(void);

#endif