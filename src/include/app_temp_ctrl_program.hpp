/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_TEMP_CTRL_PROGRAM_HPP__
#define __APP_TEMP_CTRL_PROGRAM_HPP__

enum week_days{
    sun = 0,
    mon,
    tue,
    wed,
    thu,
    fri,
    sat,
    everyday = 8
};

struct prgm_headings
{
    int id;
    char desc[33];    
};

#define MAX_PROGRAM_COUNT 10

extern List<struct prgm_headings> *program_lst;

void init_program_list(void);

struct prgm_period
{
    enum week_days day_of_week;
    int mm_start; // period starting minute on a day period [0..1440]
    int mm_end;   // period ending minute on a day period [0..1440]
    int setpoint;
};

struct prgm
{
    int id;
    int min_temp;
    int period_count;
    struct prgm_period *period; // struct prgm_period period[] 
};

extern struct prgm *current_program;

#define MAX_PRG_COUNT_REACHED (-1)
#define ERR_SAVING_PRG (-2)
#define ERR_MEM_EXHAUSTED (-3)

int add_program(char *, struct prgm *); // return id or <0 if fails
int del_program(int);                   // return id or <0 if fails

void delete_program(struct prgm *);
struct prgm *load_program(int idx);
char *get_cur_program_name(int);

#endif