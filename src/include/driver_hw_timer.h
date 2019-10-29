/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> modified this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

#ifndef __HW_TIMER_H__
#define __HW_TIMER_H__

#include "ets_sys.h"

#define US_TO_RTC_TIMER_TICKS(t) \
    ((t) ? (((t) > 0x35A) ? (((t) >> 2) * ((APB_CLK_FREQ >> 4) / 250000) + ((t)&0x3) * ((APB_CLK_FREQ >> 4) / 1000000)) : (((t) * (APB_CLK_FREQ >> 4)) / 1000000)) : 0)

#define FRC1_ENABLE_TIMER BIT7
#define FRC1_DISABLE_TIMER 0x0
#define FRC1_AUTO_LOAD BIT6

typedef enum
{
    DIVDED_BY_1 = 0,   // timer clock
    DIVDED_BY_16 = 4,  // divided by 16
    DIVDED_BY_256 = 8, // divided by 256
} TIMER_PREDIVED_MODE;

typedef enum
{                     // timer interrupt mode
    TM_LEVEL_INT = 1, // level interrupt
    TM_EDGE_INT = 0,  // edge interrupt
} TIMER_INT_MODE;

typedef enum
{
    FRC1_SOURCE = 0,
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

void hw_timer_set_func(void (*user_hw_timer_cb_set)(void));
void hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req);
void hw_timer_arm(uint32 val);
void hw_timer_disarm(void);

#endif