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

#define JSON_SINTAX_OK -1

// json errors
typedef enum
{
  JSON_TYPE_ERR = 0,
  JSON_STRING,
  JSON_INTEGER,
  JSON_OBJECT,
  JSON_ARRAY
} Json_value_type;

typedef enum
{
  JSON_ERR = 0,
  JSON_NEW_PAIR_FOUND,
  JSON_NO_NEW_PAIR_FOUND
} Json_pair_type;

class Json_str
{
public:
  Json_str(char *, int);                          // gets string to be parsed and string length
  ~Json_str(){};                                  //
  int syntax_check(void);                         // return JSON_SINTAX_OK on fine syntax
                                                  // return (error position) in case of error
  Json_pair_type find_next_pair(void);            // find next json pair
  char *get_cur_pair_string(void);                // return current pair string (NULL if none)
  int get_cur_pair_string_len(void);              // return current pair string length (0 if none)
  Json_value_type get_cur_pair_value_type(void);  // return current json pair value type (JSON_TYPE_ERR if none)
  char *get_cur_pair_value(void);                 // return current pair value as string (NULL if none)
  int get_cur_pair_value_len(void);               // return current pair value length (0 if none)
  char *get_cursor(void);                         // for debug purposes
  Json_pair_type find_pair(const char *t_string); // find json pair with string == t_string

private:
  char *_str;   // the string to be parsed
  int _str_len; // the string len
  char *_cursor;
  char *_pair_string;
  int _pair_string_len;
  Json_value_type _pair_value_type;
  char *_pair_value; //
  int _pair_value_len;
};

class Json_array_str
{
public:
  Json_array_str(char *, int);        // gets string to be parsed and string length
  ~Json_array_str(){};                //
  int syntax_check(void);             // return JSON_SINTAX_OK on fine syntax
                                      //
  int size(void);                     // return array size (count of elements)
  char *get_elem(int);                // return current element string (NULL if none)
  int get_elem_len(int);              // return current element string length (0 if none)
  Json_value_type get_elem_type(int); // return current element type (JSON_TYPE_ERR if none)
  char *get_cursor(void);             // for debug purposes

private:
  char *_str;   // the string to be parsed
  int _str_len; // the string len
  char *_cursor;
  int _el_count;
  int _cur_elem;
  char *_el;
  int _el_len;
  Json_value_type _el_type;
  int find_elem(int);
};

#endif