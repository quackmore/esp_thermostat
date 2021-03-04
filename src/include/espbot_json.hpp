/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __JSON_HPP__
#define __JSON_HPP__

/**
 * @brief 
 * 
 */

typedef enum
{
  JSON_outOfBoundary = -8,
  JSON_empty = -7,
  JSON_pairFound = -6,
  JSON_pairNotFound = -5,
  JSON_found = -4,
  JSON_notFound = -3,
  JSON_typeMismatch = -2,
  JSON_noerr = -1,
  JSON_sintaxErr
} JSONerr;

typedef enum
{
  JSON_unknown = 0,
  JSON_num,
  JSON_str,
  JSON_obj,
  JSONP_array
} Json_value_type;

class JSONP_ARRAY;

/**
 * @brief a class for JSONP parsin
 * 
 */

class JSONP
{
public:
  JSONP();
  JSONP(char *);
  JSONP(char *, int);
  ~JSONP(){};

  int getInt(const char *name);
  float getFloat(const char *name);
  void getStr(const char *name, char *dest, int len);
  int getStrlen(const char *name);
  JSONP getObj(const char *name);
  JSONP_ARRAY getArray(const char *name);
  void clearErr(void);
  int getErr(void);
#ifdef JSON_TEST
  void getStr(char *str, int str_len);
#endif

private:
  char *_jstr;
  int _len;
  char *_cursor;
  char *_cur_name;
  int _cur_name_len;
  Json_value_type _cur_type;
  char *_cur_value; //
  int _cur_value_len;
  int _err;

  int syntax_check(void);
  int find_key(const char *t_string);
  int find_pair(void);
};

class JSONP_ARRAY
{
public:
  JSONP_ARRAY();
  JSONP_ARRAY(char *);
  JSONP_ARRAY(char *, int);
  ~JSONP_ARRAY(){};
  int len(void);
  int getInt(int id);
  float getFloat(int id);
  void getStr(int id, char *dest, int len);
  int getStrlen(int id);
  JSONP getObj(int id);
  JSONP_ARRAY getArray(int id);
  void clearErr(void);
  int getErr(void);
#ifdef JSON_TEST
  void getStr(char *str, int str_len);
#endif

private:
  char *_jstr;
  int _len;
  char *_cursor;
  int _el_count;
  int _cur_id;
  char *_cur_el;
  int _cur_len;
  Json_value_type _cur_type;
  int _err;

  int syntax_check(void);
  int find_elem(int idx);
};

#endif