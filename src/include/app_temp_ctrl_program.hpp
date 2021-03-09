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

enum week_days
{
    mon = 1,
    tue,
    wed,
    thu,
    fri,
    sat,
    sun,
    everyday = 8
};

struct prgm_headings
{
    int id;
    char desc[32];
};

#define MAX_PROGRAM_COUNT 20
#define MAX_PROGRAM_PERIODS 200

#include "espbot_list.hpp"

extern List<struct prgm_headings> *program_lst;

void init_program_list(void);

struct prgm_period
{
    enum week_days day_of_week;
    int mm_start; // period starting minute on a day range [0..1339]
    int mm_end;   // period ending minute on a day range [0..1339]
    int setpoint;
};

struct prgm
{
    int id;
    int min_temp;
    int period_count;
    struct prgm_period *periods; // struct prgm_period periods[]
};

#define MAX_PRG_COUNT_REACHED (-1)                         // errors codes for add_program and del_program
#define ERR_SAVING_PRG (-2)                                //
#define ERR_MEM_EXHAUSTED (-3)                             //
#define ERR_PRG_NOT_FOUND (-4)                             //
#define ERR_PRG_BAD_SYNTAX (-5)                            //
                                                           //
int add_program(char *, struct prgm *);                    // return id or <0 if fails
int del_program(int);                                      // return id or <0 if fails
int mod_program(int prg_id, char *name, struct prgm *prg); // return id or <0 if fails
                                                           //
extern struct prgm *current_program;                       //
                                                           //
char *get_cur_program_name(int);                           //
int load_program(int idx, struct prgm *&prg);               // allocates memory on heap (prg)
void delete_program(struct prgm *);                        // delete a struct prgm allocated on heap

char *program_json_stringify(struct prgm *, char *dest = NULL, int len = 0);
char *prg_list_json_stringify(char *dest = NULL, int len = 0);

#endif