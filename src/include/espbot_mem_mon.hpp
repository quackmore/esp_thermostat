/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __MEM_MON_H__
#define __MEM_MON_H__

extern "C"
{
#include "mem.h"
}

void *espbot_zalloc(size_t);
void espbot_free(void *);

void mem_mon_init(void);
void mem_mon_stack(void);
void mem_mon_heap(void);

char *mem_mon_json_stringify(char *dest = NULL, int len = 0);
char *mem_dump_json_stringify(char *address_str, int dump_len, char *dest = NULL, int len = 0);
char *mem_dump_hex_json_stringify(char *address_str, int dump_len, char *dest = NULL, int len = 0);
char *mem_last_reset_json_stringify(char *dest = NULL, int len = 0);

#endif