/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

class File_to_json
{
private:
  char *m_filename;
  char *m_cache;
  char *m_value_str;
  int m_value_len;
  int get_value_len(void);

public:
  File_to_json(char *); // require filename
  ~File_to_json();
  bool exists(void);
  int find_string(char *); // require string name of a json pair
                           // return 0 on success !=0 on fail
  char *get_value(void);
};

#endif