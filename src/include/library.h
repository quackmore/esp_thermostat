/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include "c_types.h"


// these are espbot_2.0 memory management methods
// https://github.com/quackmore/espbot_2.0
void *call_espbot_zalloc(size_t size);
void call_espbot_free(void *addr);

#endif