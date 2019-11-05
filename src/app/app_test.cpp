/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

// SDK includes
extern "C"
{
#include "mem.h"
#include "library_dio_task.h"
#include "esp8266_io.h"
}

#include "app.hpp"
#include "espbot_global.hpp"
#include "app_temp_control.hpp"


void run_test(int test_number)
{
    switch (test_number)
    {
    case 1:
        os_printf("TEST: -----> got CTRL OFF\n");
        ctrl_off();
        break;
    case 2:
        os_printf("TEST: -----> got CTRL MANUAL CONTINUOS\n");
        ctrl_manual(0, 0, 0);
        break;
    case 3:
        os_printf("TEST: -----> got CTRL MANUAL CONTINUOS for 3 minutes\n");
        ctrl_manual(0, 0, 3);
        break;
    case 4:
        os_printf("TEST: -----> got CTRL MANUAL 2 minutes on 2 minutes off - never stop\n");
        ctrl_manual(2, 2, 0);
        break;
    case 5:
        os_printf("TEST: -----> got CTRL MANUAL 2 minutes on 2 minutes off - stop after 5 minutes\n");
        ctrl_manual(2, 2, 5);
        break;
    
    default:
        break;
    }

}