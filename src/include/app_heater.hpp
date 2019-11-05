/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_HEATER_HPP__
#define __APP_HEATER_HPP__

void heater_init(void);
void heater_start(void);
// void heater_start(int period); // will stop after period (milliseconds)
void heater_stop(void);
bool is_heater_on(void);

#endif