/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __ESPBOT_IO_H__
#define __ESPBOT_IO_H__

#define ESPBOT_D1 1
#define ESPBOT_D2 2
#define ESPBOT_D3 3
#define ESPBOT_D4 4
#define ESPBOT_D5 5
#define ESPBOT_D6 6
#define ESPBOT_D7 7
#define ESPBOT_D8 8

#define ESPBOT_LOW 0
#define ESPBOT_HIGH 1

/*
 * Digital I/O map for MODEMCU, WEMOS D1 mini
 * 
 *    gpio   pin   mux
 * -----------------------
 * D1 GPIO5  pin24 GPIO5_U
 * D2 GPIO4  pin16 GPIO4_U
 * D3 GPIO0  pin15 GPIO0_U
 * D4 GPIO2  pin14 GPIO2_U
 * D5 GPIO14 pin9  MTMS_U
 * D6 GPIO12 pin10 MTDI_U
 * D7 GPIO13 pin12 MTCK_U
 * D8 GPIO15 pin13 MTDO_U
 */

#define ESPBOT_D1_NUM 5
#define ESPBOT_D2_NUM 4
#define ESPBOT_D3_NUM 0
#define ESPBOT_D4_NUM 2
#define ESPBOT_D5_NUM 14
#define ESPBOT_D6_NUM 12
#define ESPBOT_D7_NUM 13
#define ESPBOT_D8_NUM 15

#define ESPBOT_D1_MUX PERIPHS_IO_MUX_GPIO5_U
#define ESPBOT_D1_FUNC FUNC_GPIO5
#define ESPBOT_D2_MUX PERIPHS_IO_MUX_GPIO4_U
#define ESPBOT_D2_FUNC FUNC_GPIO4
#define ESPBOT_D3_MUX PERIPHS_IO_MUX_GPIO0_U
#define ESPBOT_D3_FUNC FUNC_GPIO0
#define ESPBOT_D4_MUX PERIPHS_IO_MUX_GPIO2_U
#define ESPBOT_D4_FUNC FUNC_GPIO2
#define ESPBOT_D5_MUX PERIPHS_IO_MUX_MTMS_U
#define ESPBOT_D5_FUNC FUNC_GPIO14
#define ESPBOT_D6_MUX PERIPHS_IO_MUX_MTDI_U
#define ESPBOT_D6_FUNC FUNC_GPIO12
#define ESPBOT_D7_MUX PERIPHS_IO_MUX_MTCK_U
#define ESPBOT_D7_FUNC FUNC_GPIO13
#define ESPBOT_D8_MUX PERIPHS_IO_MUX_MTDO_U
#define ESPBOT_D8_FUNC FUNC_GPIO15

int gpio_NUM(int);
int gpio_MUX(int);
int gpio_FUNC(int);

//
// Espressif SDK MACROS
// 
// PIN_PULLUP_DIS(PIN_NAME)             -> PIN_PULLUP_DIS(ESPBOT_D1_MUX)
// PIN_PULLUP_EN(PIN_NAME)              -> PIN_PULLUP_EN(ESPBOT_D1_MUX)
// PIN_FUNC_SELECT(PIN_NAME, FUNC)      -> PIN_FUNC_SELECT(ESPBOT_D1_MUX, ESPBOT_D1_FUNC)
// GPIO_OUTPUT_SET(gpio_no, bit_value)  -> GPIO_OUTPUT_SET(ESPBOT_D1_NUM, ESPBOT_LOW)
// GPIO_DIS_OUTPUT(gpio_no)             -> GPIO_DIS_OUTPUT(ESPBOT_D1_NUM)
// GPIO_INPUT_GET(gpio_no)              -> GPIO_INPUT_GET(ESPBOT_D1_NUM)
// ETS_GPIO_INTR_ATTACH(func, arg)      
// ETS_GPIO_INTR_DISABLE()               
// ETS_GPIO_INTR_ENABLE()                
// 
// examples:
// set D1 ouput high and enable pull-up -> PIN_FUNC_SELECT(ESPBOT_D1_MUX, ESPBOT_D1_FUNC)
//                                      -> GPIO_OUTPUT_SET(ESPBOT_D1_NUM, ESPBOT_HIGH)
//                                      -> PIN_PULLUP_EN(ESPBOT_D1_MUX)
// 
// enable pull-up and get D1 input level -> PIN_FUNC_SELECT(ESPBOT_D1_MUX, ESPBOT_D1_FUNC)
//                                       -> PIN_PULLUP_EN(ESPBOT_D1_MUX)
//                                       -> GPIO_DIS_OUTPUT(ESPBOT_D1_NUM)
//                                       -> level = GPIO_INPUT_GET(ESPBOT_D1_NUM)
// 

#endif