/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __ESPBOT_UTILS_HPP__
#define __ESPBOT_UTILS_HPP__

extern "C"
{
#include "osapi.h"
#include "ip_addr.h"

  int atoi(char *);
  float atof(char *);
  int atoh(char *);
}

// my own
extern "C"
{
  void decodeUrlStr(char *);
  void atoipaddr(struct ip_addr *ip, char *str);
  int get_rand_int(int max_value);
  char *f2str(char *str, float value, int decimals);
}

typedef enum
{
  free = 0,
  dont_free
} Free_opt;

class Heap_chunk
{
private:
  Free_opt m_to_be_free;

public:
  Heap_chunk(int len, Free_opt free_opt = free); // takes string len and
                                                 //     free      - will free memory on variable destruction
                                                 //     dont_free - won't free memory on variable destruction
  ~Heap_chunk();
  char *ref; // string reference
  int len(void);

  //        EXAMPLE:
  //
  //        Heap_chunk tmp_str(128);
  //        if (tmp_str.ref)
  //        {
  //            os_strncpy(tmp_port.ref, "source", source_length);
  //        }
  //        else
  //        {
  //            ... not enough heap memory ...
  //        }
};

#endif