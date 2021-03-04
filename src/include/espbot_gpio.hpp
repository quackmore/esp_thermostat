/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __GPIO_HPP__
#define __GPIO_HPP__

extern "C"
{
#include "c_types.h"
#include "esp8266_io.h"
}

#define ESPBOT_GPIO_OK -1
#define ESPBOT_GPIO_WRONG_IDX -2
#define ESPBOT_GPIO_WRONG_LVL -3
#define ESPBOT_GPIO_WRONG_TYPE -4
#define ESPBOT_GPIO_CANNOT_CHANGE_INPUT -5

#define ESPBOT_GPIO_INPUT 0
#define ESPBOT_GPIO_OUTPUT 1
#define ESPBOT_GPIO_UNPROVISIONED 2

void gpio_init(void);

int gpio_config(int t_idx, int t_type); // take the gpio index (1..8) and ESPBOT_GPIO_INPUT/ESPBOT_GPIO_INPUT
int gpio_unconfig(int);                 // return ESPBOT_GPIO_OK on success
                                        // return ESPBOT_GPIO_WRONG_IDX if idx < 1 or idx > 8
                                        // return ESPBOT_GPIO_WRONG_TYPE if t_type is different from
                                        //        ESPBOT_GPIO_INPUT/ESPBOT_GPIO_INPUT
                                        // gpio configuration is not persistent

int gpio_get_config(int); // takes the gpio index (1..8)
                          // returns ESPBOT_GPIO_UNPROVISIONED or
                          //         ESPBOT_GPIO_INPUT or
                          //         ESPBOT_GPIO_OUTPUT on success
                          // returns ESPBOT_GPIO_WRONG_IDX if idx < 1 or idx > 8
                          // does not set pull-up or pull-down

int gpio_read(int); // takes the gpio index (1..8)
                    // returns ESPBOT_LOW or ESPBOT_HIGH on success
                    // returns ESPBOT_GPIO_WRONG_IDX if idx < 1 or idx > 8
                    // returns ESPBOT_GPIO_UNPROVISIONED if gpio wasn't set as input/output

int gpio_set(int, int); // takes the gpio index (1..8) and the output level (ESPBOT_LOW or ESPBOT_HIGH)
                        // returns ESPBOT_GPIO_OK on success
                        // returns ESPBOT_GPIO_WRONG_IDX if idx < 1 or idx > 8
                        // returns ESPBOT_GPIO_UNPROVISIONED if gpio wasn't set as input/output
                        // returns ESPBOT_GPIO_CANNOT_SET_INPUT if gpio was set as input

bool gpio_valid_id(int);
char *gpio_cfg_json_stringify(int idx, char *dest = NULL, int len = 0);
char *gpio_state_json_stringify(int idx, char *dest = NULL, int len = 0);

#endif
