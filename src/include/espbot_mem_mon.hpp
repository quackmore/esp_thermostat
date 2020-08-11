/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__

extern "C"
{
#include "mem.h"
}

#ifdef ESPBOT

#define esp_zalloc(a) espmem.espbot_zalloc(a)
#define esp_free(a) espmem.espbot_free(a)
#define esp_stack_mon(a) espmem.stack_mon()

#else

#define esp_zalloc(a) os_zalloc(a)
#define esp_free(a) os_free(a)
#define esp_stack_mon(a)

#endif

typedef enum
{
  first = 0,
  next
} List_item;

struct heap_item
{
  int size;
  void *addr;
};

#define HEAP_ARRAY_SIZE 200

class Esp_mem
{
private:
  // stack infos
  uint32 _stack_min_addr;
  uint32 _stack_max_addr;
  // heap infos
  uint32 _heap_start_addr;
  uint32 _max_heap_size;
  uint32 _min_heap_size;
  uint32 _heap_objs;
  uint32 _max_heap_objs;

  // DEBUG
  // struct heap_item _heap_array[HEAP_ARRAY_SIZE];

  void heap_mon(void);

public:
  Esp_mem(){};
  ~Esp_mem(){};

  void init(void);

  void stack_mon(void);

  static void *espbot_zalloc(size_t);
  static void espbot_free(void *);

  uint32 get_min_stack_addr(void);
  uint32 get_max_stack_addr(void);

  uint32 get_start_heap_addr(void);
  uint32 get_max_heap_size(void);
  uint32 get_mim_heap_size(void);
  // uint32 get_used_heap_size(void);
  uint32 get_heap_objs(void);
  uint32 get_max_heap_objs(void);
  // struct heap_item *get_heap_item(List_item item = first); // return NULL if no item is found
  // DEBUG
  // void print_heap_objects(void);
};

#endif