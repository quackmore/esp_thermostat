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
  JSON_OBJECT
} Json_value_type;

typedef enum
{
  JSON_ERR = 0,
  JSON_NEW_PAIR_FOUND,
  JSON_NO_NEW_PAIR_FOUND
} Json_pair_type;

class Json_str
{
private:
  char *m_str;   // the string to be parsed
  int m_str_len; // the string len
  char *m_cursor;
  char *m_cur_pair_string;
  int m_cur_pair_string_len;
  Json_value_type m_cur_pair_value_type;
  char *m_cur_pair_value; //
  int m_cur_pair_value_len;

  char *find_object_end(char *);

public:
  Json_str(char *, int); // get string to be parsed and string length
  ~Json_str(){};
  int syntax_check(void);                        // return JSON_SINTAX_OK on fine syntax
                                                 // return (error position) in case of error
  Json_pair_type find_next_pair(void);           // find next json pair
  char *get_cur_pair_string(void);               // return current pair string (NULL if none)
  int get_cur_pair_string_len(void);             // return current pair string length (0 if none)
  Json_value_type get_cur_pair_value_type(void); // return current json pair value type
  char *get_cur_pair_value(void);                // return current pair value as string (NULL if none)
  int get_cur_pair_value_len(void);              // return current pair value length (0 if none)
  // char *get_cursor(void);                        // for debug purposes
  Json_pair_type find_pair(const char *t_string);      // find json pair with string == t_string
};

#endif