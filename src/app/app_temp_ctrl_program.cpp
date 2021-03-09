/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */

// SDK includes
extern "C"
{
#include "mem.h"
#include "espbot_mem_macros.h"
}

#include "espbot_cfgfile.hpp"
#include "espbot_diagnostic.hpp"
#include "espbot_json.hpp"
#include "espbot_list.hpp"
#include "espbot_mem_mon.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_temp_ctrl_program.hpp"

List<struct prgm_headings> *program_lst;

// CRTL settings management

#define PRG_LIST_FILENAME ((char *)f_str("program_list.cfg"))

static int prg_list_restore(void)
{
    ALL("prg_list_restore");
    if (!Espfile::exists(PRG_LIST_FILENAME))
        return CFG_cantRestore;
    Cfgfile cfgfile(PRG_LIST_FILENAME);
    JSONP_ARRAY prg_headings = cfgfile.getArray(f_str("prg_headings"));
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(CTRL_PRG_LIST_RESTORE_ERROR);
        ERROR("prg_list_restore error");
        return CFG_error;
    }
    int idx;
    for (idx = 0; idx < prg_headings.len(); idx++)
    {
        if (idx >= MAX_PROGRAM_COUNT)
            break;
        JSONP heading = prg_headings.getObj(idx);
        if (heading.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_PRG_LIST_RESTORE_ERROR);
            ERROR("prg_list_restore error");
            return CFG_error;
        }
        int id = heading.getInt(f_str("id"));
        char desc[32];
        os_memset(desc, 0, 32);
        heading.getStr(f_str("desc"), desc, 32);
        if (heading.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_PRG_LIST_RESTORE_ERROR);
            ERROR("prg_list_restore error");
            return CFG_error;
        }
        struct prgm_headings *heading_el = new struct prgm_headings;
        if (heading_el == NULL)
        {
            dia_error_evnt(CTRL_PRG_LIST_RESTORE_HEAP_EXHAUSTED, sizeof(struct prgm_headings));
            ERROR("temp_control_restore_prgm_list array[%d] bad syntax", sizeof(struct prgm_headings));
            return CFG_error;
        }
        heading_el->id = id;
        os_strncpy(heading_el->desc, desc, 31);
        program_lst->push_back(heading_el);
    }
    mem_mon_stack();
    return CFG_ok;
}

static int prg_list_saved_updated(void)
{
    ALL("prg_list_saved_updated");
    Cfgfile cfgfile(PRG_LIST_FILENAME);
    JSONP_ARRAY prg_headings = cfgfile.getArray(f_str("prg_headings"));
    if (cfgfile.getErr() != JSON_noerr)
        return CFG_error;
    if (program_lst->size() != prg_headings.len())
        return CFG_notUpdated;
    struct prgm_headings *cur_heading = program_lst->front();
    int idx;
    for (idx = 0; idx < prg_headings.len(); idx++)
    {
        if (idx >= MAX_PROGRAM_COUNT)
            break;
        JSONP heading = prg_headings.getObj(idx);
        if (heading.getErr() != JSON_noerr)
            return CFG_error;
        int id = heading.getInt(f_str("id"));
        char desc[32];
        os_memset(desc, 0, 32);
        heading.getStr(f_str("desc"), desc, 32);
        if (heading.getErr() != JSON_noerr)
            return CFG_error;
        if (cur_heading == NULL)
            return CFG_notUpdated;
        if ((cur_heading->id != id) || (os_strncmp(cur_heading->desc, desc, 31)))
            return CFG_notUpdated;
        cur_heading = program_lst->next();
    }
    mem_mon_stack();
    return CFG_ok;
}

char *prg_list_json_stringify(char *dest, int len)
{
    // {
    //     "prg_headings": [
    //         {"id": 1,"desc":"first"},
    //         {"id": 2,"desc":"second"},
    //         {"id": 3,"desc":"third"}
    //     ]
    // }
    // {"prg_headings":[]}
    // {"id":,"desc":""},
    int msg_len = 19 + (program_lst->size() * (18 + 2 + 31)) + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_PRG_LIST_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("prg_list_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg, "{\"prg_headings\":[");
    int idx;
    bool first_time = true;
    struct prgm_headings *cur_heading = program_lst->front();
    for (idx = 0; idx < program_lst->size(); idx++)
    {
        //  ,{"id":,"desc":""}
        if (first_time)
        {
            first_time = false;
            fs_sprintf(msg + os_strlen(msg),
                       "{\"id\":%d,\"desc\":\"%s\"}",
                       cur_heading->id,
                       cur_heading->desc);
        }
        else
            fs_sprintf(msg + os_strlen(msg),
                       ",{\"id\":%d,\"desc\":\"%s\"}",
                       cur_heading->id,
                       cur_heading->desc);
        cur_heading = program_lst->next();
    }
    fs_sprintf(msg + os_strlen(msg), "]}");
    mem_mon_stack();
    return msg;
}

int prg_list_save(void)
{
    ALL("prg_list_save");
    if (prg_list_saved_updated() == CFG_ok)
        return CFG_ok;
    Cfgfile cfgfile(PRG_LIST_FILENAME);
    if (cfgfile.clear() != SPIFFS_OK)
        return CFG_error;
    char *str = prg_list_json_stringify();
    int res = cfgfile.n_append(str, os_strlen(str));
    delete str;
    if (res < SPIFFS_OK)
        return CFG_error;
    mem_mon_stack();
    return CFG_ok;
}

// PROGRAM LIST
// {
//     "prg_count": 3,
//     "prg_headings": [
//         {"id": 1,"desc":"first"},
//         {"id": 2,"desc":"second"},
//         {"id": 3,"desc":"third"}
//     ]
// }

void init_program_list(void)
{
    ALL("init_program_list");

    program_lst = new List<struct prgm_headings>(MAX_PROGRAM_COUNT, delete_content);

    if (prg_list_restore() != CFG_ok)
    {
        // CTRL DEFAULT
        dia_warn_evnt(CTRL_PRG_INIT_DEFAULT_PRGM_LIST);
        WARN("init_program_list no program available");
    }
}

void delete_program(struct prgm *prog_ptr)
{
    if (prog_ptr == NULL)
        return;
    if (prog_ptr->periods)
        delete[] prog_ptr->periods;
    delete prog_ptr;
}

// PROGRAM
// {
//     "id": 1,
//     "min_temp": 100,
//     "periods": [
//         {"wd": 8,"b":1880,"e":1880,"sp":200},
//         {"wd": 8,"b":1880,"e":1880,"sp":200},
//         {"wd": 8,"b":1880,"e":1880,"sp":200}
//     ]
// }

int load_program(int prg_id, struct prgm *&prg)
{
    if (program_lst->size() == 0)
    {
        dia_error_evnt(CTRL_PRG_LOAD_NO_PROGRAM_LIST);
        ERROR("load_program no program list");
        return ERR_PRG_NOT_FOUND;
    }
    // search for program id
    bool program_not_found = true;
    struct prgm_headings *cur_heading = program_lst->front();
    while (cur_heading)
    {
        if (cur_heading->id == prg_id)
        {
            program_not_found = false;
            break;
        }
        cur_heading = program_lst->next();
    }
    // check if a program was found
    if (program_not_found)
    {
        dia_error_evnt(CTRL_PRG_LOAD_NO_PROGRAM_ID, prg_id);
        ERROR("load_program no program id %d", prg_id);
        return ERR_PRG_NOT_FOUND;
    }
    char filename[32];
    os_memset(filename, 0, 32);
    fs_snprintf(filename, 31, "program_%d.prg", prg_id);
    TRACE("load_program filename %s", filename);
    Cfgfile cfgfile(filename);
    int id = cfgfile.getInt(f_str("id"));
    int min_temp = cfgfile.getInt(f_str("min_temp"));
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(CTRL_PRG_LOAD_BAD_SYNTAX);
        ERROR("load_program error");
        return ERR_PRG_BAD_SYNTAX;
    }
    if (prg_id != id)
    {
        dia_error_evnt(CTRL_PRG_LOAD_BAD_SYNTAX);
        ERROR("load_program wrong id \"id\"", id);
        return ERR_PRG_BAD_SYNTAX;
    }
    JSONP_ARRAY periods = cfgfile.getArray(f_str("periods"));
    if (cfgfile.getErr() != JSON_noerr)
    {
        dia_error_evnt(CTRL_PRG_LOAD_BAD_SYNTAX);
        ERROR("load_program cannot find \"periods\"");
        return ERR_PRG_BAD_SYNTAX;
    }
    prg = new struct prgm;
    if (prg == NULL)
    {
        dia_error_evnt(CTRL_PRG_LOAD_HEAP_EXHAUSTED, sizeof(struct prgm));
        ERROR("load_program heap exhausted %d", sizeof(struct prgm));
        return ERR_MEM_EXHAUSTED;
    }
    prg->id = id;
    prg->min_temp = min_temp;
    prg->period_count = periods.len();
    // there are no period in the program
    if (prg->period_count == 0)
    {
        prg->periods = NULL;
        return id;
    }
    // there are periods in the program
    if (prg->period_count > MAX_PROGRAM_PERIODS)
    {
        dia_error_evnt(CTRL_PRG_LOAD_BAD_SYNTAX);
        ERROR("load_program too many periods");
        return ERR_PRG_BAD_SYNTAX;
    }
    prg->periods = new struct prgm_period[prg->period_count];
    if (prg->periods == NULL)
    {
        dia_error_evnt(CTRL_PRG_LOAD_HEAP_EXHAUSTED, sizeof(struct prgm_period[prg->period_count]));
        ERROR("load_program heap exhausted %d", sizeof(struct prgm_period[prg->period_count]));
        return ERR_MEM_EXHAUSTED;
    }
    int count;
    for (count = 0; count < prg->period_count; count++)
    {
        JSONP period = periods.getObj(count);
        if (periods.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_PRG_LOAD_BAD_SYNTAX);
            ERROR("load_program cannot find period %d", count);
            return ERR_PRG_BAD_SYNTAX;
        }
        enum week_days wd = (enum week_days)period.getInt(f_str("wd"));
        int b = period.getInt(f_str("b"));
        int e = period.getInt(f_str("e"));
        int sp = period.getInt(f_str("sp"));
        if (period.getErr() != JSON_noerr)
        {
            dia_error_evnt(CTRL_PRG_LOAD_BAD_SYNTAX);
            ERROR("load_program bad json syntax on period %d", count);
            return ERR_PRG_BAD_SYNTAX;
        }
        prg->periods[count].day_of_week = wd;
        prg->periods[count].mm_start = b;
        prg->periods[count].mm_end = e;
        prg->periods[count].setpoint = sp;
    }
    mem_mon_stack();
    return id;
}

char *get_cur_program_name(int id)
{
    struct prgm_headings *tmp_ptr = program_lst->front();
    while (tmp_ptr)
    {
        if (tmp_ptr->id == id)
            break;
        tmp_ptr = program_lst->next();
    }
    if (tmp_ptr)
        return tmp_ptr->desc;
    else
        return (char *)f_str("none");
}

char *program_json_stringify(struct prgm *prg, char *dest, int len)
{
    // PROGRAM
    // {
    //     "id": 1,
    //     "min_temp": 100,
    //     "periods": [
    //         {"wd": 8,"b":1880,"e":1880,"sp":200},
    //         {"wd": 8,"b":1880,"e":1880,"sp":200},
    //         {"wd": 8,"b":1880,"e":1880,"sp":200}
    //     ]
    // }
    // {"id":,"min_temp":,"periods":[]}
    // {"wd":,"b":,"e":,"sp":},
    int msg_len = 32 + 2 + 6 + (prg->period_count * (24 + 1 + 4 + 4 + 6)) + 1;
    char *msg;
    if (dest == NULL)
    {
        msg = new char[msg_len];
        if (msg == NULL)
        {
            dia_error_evnt(CTRL_PRG_STRINGIFY_HEAP_EXHAUSTED, msg_len);
            ERROR("program_json_stringify heap exhausted [%d]", msg_len);
            return NULL;
        }
    }
    else
    {
        msg = dest;
        if (len < msg_len)
        {
            *msg = 0;
            return msg;
        }
    }
    fs_sprintf(msg,
               "{\"id\":%d,\"min_temp\":%d,\"periods\":[",
               prg->id,
               prg->min_temp);
    int idx;
    bool first_time = true;
    for (idx = 0; idx < prg->period_count; idx++)
    {
        if (first_time)
        {
            first_time = false;
            fs_sprintf(msg + os_strlen(msg),
                       "{\"wd\":%d,\"b\":%d,\"e\":%d,\"sp\":%d}",
                       prg->periods[idx].day_of_week,
                       prg->periods[idx].mm_start,
                       prg->periods[idx].mm_end,
                       prg->periods[idx].setpoint);
        }
        else
            fs_sprintf(msg + os_strlen(msg),
                       ",{\"wd\":%d,\"b\":%d,\"e\":%d,\"sp\":%d}",
                       prg->periods[idx].day_of_week,
                       prg->periods[idx].mm_start,
                       prg->periods[idx].mm_end,
                       prg->periods[idx].setpoint);
    }
    fs_sprintf(msg + os_strlen(msg), "]}");
    mem_mon_stack();
    return msg;
}

static int program_save(struct prgm *prg)
{
    char filename[32];
    os_memset(filename, 0, 32);
    fs_snprintf(filename, 31, "program_%d.prg", prg->id);

    TRACE("program_save filename %s", filename);
    Cfgfile cfgfile(filename);
    if (cfgfile.clear() != SPIFFS_OK)
        return CFG_error;
    char *str = program_json_stringify(prg);
    int res = cfgfile.n_append(str, os_strlen(str));
    delete str;
    if (res < SPIFFS_OK)
        return CFG_error;
    mem_mon_stack();
    return CFG_ok;
}

int add_program(char *name, struct prgm *prg)
{
    // check if the max number of programs was reached
    if (program_lst->size() >= MAX_PROGRAM_COUNT)
        return MAX_PRG_COUNT_REACHED;
    // find a free id [0..(MAX_PROGRAM_COUNT-1)]
    int idx;
    for (idx = 0; idx < MAX_PROGRAM_COUNT; idx++)
    {
        struct prgm_headings *prgm_ptr = program_lst->front();
        bool free_id_found = true;
        while (prgm_ptr)
        {
            if (prgm_ptr->id == idx)
            {
                free_id_found = false;
                break;
            }
            prgm_ptr = program_lst->next();
        }
        if (free_id_found)
            break;
    }
    int prg_id = idx;
    // override program id
    prg->id = prg_id;
    // write program file
    if (program_save(prg) != CFG_ok)
        return ERR_SAVING_PRG;
    // add program heading
    struct prgm_headings *heading_ptr = new struct prgm_headings;
    if (heading_ptr == NULL)
    {
        dia_error_evnt(CTRL_PRG_ADD_HEAP_EXHAUSTED, sizeof(struct prgm_headings));
        ERROR("add_program heap memory exhausted %d", sizeof(struct prgm_headings));
        return ERR_MEM_EXHAUSTED;
    }
    heading_ptr->id = prg_id;
    os_strncpy(heading_ptr->desc, name, 31);
    program_lst->push_back(heading_ptr);
    if (prg_list_save() != CFG_ok)
        return ERR_SAVING_PRG;
    mem_mon_stack();
    // return id
    return prg_id;
}

int mod_program(int prg_id, char *name, struct prgm *prg)
{
    // add program heading
    struct prgm_headings *heading_ptr = program_lst->front();
    while (heading_ptr)
    {
        if (heading_ptr->id == prg_id)
            break;
        heading_ptr = program_lst->next();
    }
    if (heading_ptr == NULL)
        return ERR_PRG_NOT_FOUND;
    os_strncpy(heading_ptr->desc, name, 31);

    if (prg_list_save() != CFG_ok)
        return ERR_SAVING_PRG;
    // override program id
    prg->id = prg_id;
    // write program file
    if (program_save(prg) != CFG_ok)
        return ERR_SAVING_PRG;
    mem_mon_stack();
    // return id
    return prg_id;
}

int del_program(int prg_id)
{
    // delete the file
    char filename[32];
    os_memset(filename, 0, 32);
    fs_snprintf(filename, 31, "program_%d.prg", prg_id);
    TRACE("del_program filename %s", filename);
    Cfgfile cfgfile(filename);
    if (cfgfile.remove() != SPIFFS_OK)
        return CFG_error;
    // find the program id into the headings list
    struct prgm_headings *heading_ptr = program_lst->front();
    bool heading_found = false;
    while (heading_ptr)
    {
        if (heading_ptr->id == prg_id)
        {
            heading_found = true;
            break;
        }
        heading_ptr = program_lst->next();
    }
    mem_mon_stack();
    if (heading_found)
    {
        // return the program_id
        program_lst->remove();
        if (prg_list_save() != CFG_ok)
            return ERR_SAVING_PRG;
        return prg_id;
    }
    else
    {
        return ERR_PRG_NOT_FOUND;
    }
}