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
}

#include "espbot_config.hpp"
#include "espbot_global.hpp"
#include "espbot_json.hpp"
#include "espbot_list.hpp"
#include "espbot_utils.hpp"
#include "app.hpp"
#include "app_event_codes.h"
#include "app_temp_ctrl_program.hpp"

static void save_prgm_list(void);

List<struct prgm_headings> *program_lst;

// CRTL settings management

#define PRG_LIST_FILENAME f_str("program_list.cfg")

static bool restore_prgm_list(void)
{
    ALL("temp_control_restore_prgm_list");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_FS_NOT_AVAILABLE);
        ERROR("temp_control_restore_prgm_list FS not available");
        return false;
    }
    File_to_json cfgfile(PRG_LIST_FILENAME);
    if (!cfgfile.exists())
        return false;

    // prg_count
    if (cfgfile.find_string(f_str("prg_count")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE);
        ERROR("temp_control_restore_prgm_list cannot find \"prg_count\"");
        return false;
    }
    int prg_count = atoi(cfgfile.get_value());

    // prg_headings
    if (cfgfile.find_string(f_str("prg_headings")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE);
        ERROR("temp_control_restore_prgm_list cannot find \"prg_headings\"");
        return false;
    }
    Json_array_str prg_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
    if ((prg_array.syntax_check() != JSON_SINTAX_OK) || (prg_array.size() != prg_count))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE);
        ERROR("temp_control_restore_prgm_list incorrect array sintax");
        return false;
    }

    int idx;
    for (idx = 0; idx < prg_count; idx++)
    {
        if ((prg_array.get_elem(idx) == NULL) || (prg_array.get_elem_type(idx) != JSON_OBJECT))
        {
            esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE, idx);
            ERROR("temp_control_restore_prgm_list array[%d] bad sintax", idx);
            return false;
        }
        Json_str heading(prg_array.get_elem(idx), prg_array.get_elem_len(idx));
        if (heading.find_pair(f_str("id")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE, idx);
            ERROR("temp_control_restore_prgm_list array[%d] bad sintax", idx);
            return false;
        }
        if (heading.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE, idx);
            ERROR("temp_control_restore_prgm_list array[%d] bad sintax", idx);
            return false;
        }
        int tmp_id = atoi(heading.get_cur_pair_value());
        if (heading.find_pair(f_str("desc")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE, idx);
            ERROR("temp_control_restore_prgm_list array[%d] bad sintax", idx);
            return false;
        }
        if (heading.get_cur_pair_value_type() != JSON_STRING)
        {
            esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE, idx);
            ERROR("temp_control_restore_prgm_list array[%d] bad sintax", idx);
            return false;
        }
        struct prgm_headings *heading_el = new struct prgm_headings;
        if (heading_el == NULL)
        {
            esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_HEAP_EXHAUSTED, sizeof(struct prgm_headings));
            ERROR("temp_control_restore_prgm_list array[%d] bad sintax", sizeof(struct prgm_headings));
            return false;
        }
        heading_el->id = tmp_id;
        int max_len = heading.get_cur_pair_value_len();
        if (max_len > 31)
            max_len = 31;
        os_strncpy(heading_el->desc, heading.get_cur_pair_value(), max_len);
        program_lst->push_back(heading_el);
    }
    espmem.stack_mon();
    return true;
}

static bool saved_prgm_list_not_updated(void)
{
    ALL("temp_control_saved_prgm_list_not_updated");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_FS_NOT_AVAILABLE);
        ERROR("temp_control_saved_prgm_list_not_updated FS not available");
        return true;
    }
    File_to_json cfgfile(PRG_LIST_FILENAME);
    if (!cfgfile.exists())
        return true;

    // prg_count
    if (cfgfile.find_string(f_str("prg_count")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_prgm_list_not_updated cannot find \"prg_count\"");
        return true;
    }
    if (program_lst->size() != atoi(cfgfile.get_value()))
        return true;

    // prg_headings
    if (cfgfile.find_string(f_str("prg_headings")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_prgm_list_not_updated cannot find \"prg_headings\"");
        return true;
    }
    Json_array_str prg_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
    if ((prg_array.syntax_check() != JSON_SINTAX_OK) || (prg_array.size() != program_lst->size()))
    {
        esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_prgm_list_not_updated incorrect array sintax");
        return true;
    }

    struct prgm_headings *cur_heading = program_lst->front();
    int idx;
    for (idx = 0; idx < program_lst->size(); idx++)
    {
        if ((prg_array.get_elem(idx) == NULL) || (prg_array.get_elem_type(idx) != JSON_OBJECT))
        {
            // don't rise any error, just say cfg not updated...
            // esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE, idx);
            // ERROR("temp_control_saved_prgm_list_not_updated array[%d] bad sintax", idx);
            return true;
        }
        Json_str heading(prg_array.get_elem(idx), prg_array.get_elem_len(idx));
        if (heading.find_pair(f_str("id")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE, idx);
            ERROR("temp_control_saved_prgm_list_not_updated array[%d] bad sintax", idx);
            return true;
        }
        if (heading.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE, idx);
            ERROR("temp_control_saved_prgm_list_not_updated array[%d] bad sintax", idx);
            return true;
        }
        int tmp_id = atoi(heading.get_cur_pair_value());
        if (heading.find_pair(f_str("desc")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE, idx);
            ERROR("temp_control_saved_prgm_list_not_updated array[%d] bad sintax", idx);
            return false;
        }
        if (heading.get_cur_pair_value_type() != JSON_STRING)
        {
            esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE, idx);
            ERROR("temp_control_saved_prgm_list_not_updated array[%d] bad sintax", idx);
            return false;
        }
        if (cur_heading == NULL)
            return true;
        if ((cur_heading->id != tmp_id) || (os_strncmp(cur_heading->desc, heading.get_cur_pair_value(), 31)))
            return true;
        cur_heading = program_lst->next();
    }
    espmem.stack_mon();
    return false;
}

static void remove_prgm_list(void)
{
    ALL("remove_prgm_list");
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_REMOVE_PRGM_LIST_FS_NOT_AVAILABLE);
        ERROR("remove_prgm_list FS not available");
        return;
    }
    if (Ffile::exists(&espfs, (char *)PRG_LIST_FILENAME))
    {
        Ffile cfgfile(&espfs, (char *)PRG_LIST_FILENAME);
        cfgfile.remove();
    }
}

static void save_prgm_list(void)
{
    ALL("save_prgm_list");
    if (program_lst->size() == 0)
    {
        remove_prgm_list();
        return;
    }
    if (saved_prgm_list_not_updated())
        remove_prgm_list();
    else
        return;
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_PRGM_LIST_FS_NOT_AVAILABLE);
        ERROR("save_prgm_list FS not available");
        return;
    }
    Ffile cfgfile(&espfs, (char *)PRG_LIST_FILENAME);
    if (!cfgfile.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_PRGM_LIST_CANNOT_OPEN_FILE);
        ERROR("save_prgm_list cannot open %s", PRG_LIST_FILENAME);
        return;
    }

    {
        //  {"prg_count":,"prg_headings":[
        char buffer[(33 + 1 + 2)];
        fs_sprintf(buffer,
                   "{\"prg_count\":%d,\"prg_headings\":[",
                   program_lst->size());
        cfgfile.n_append(buffer, os_strlen(buffer));
        espmem.stack_mon();
    }
    int idx;
    bool first_time = true;
    struct prgm_headings *cur_heading = program_lst->front();
    for (idx = 0; idx < program_lst->size(); idx++)
    {
        //  ,{"id":,"desc":""}
        char buffer[(18 + 1 + 2 + 31)];
        if (first_time)
        {
            first_time = false;
            fs_sprintf(buffer,
                       "{\"id\": %d,\"desc\":\"%s\"}",
                       cur_heading->id,
                       cur_heading->desc);
        }
        else
            fs_sprintf(buffer,
                       ",{\"id\": %d,\"desc\":\"%s\"}",
                       cur_heading->id,
                       cur_heading->desc);

        cfgfile.n_append(buffer, os_strlen(buffer));
        cur_heading = program_lst->next();
        espmem.stack_mon();
    }
    {
        //  ]}
        char buffer[(2 + 1)];
        fs_sprintf(buffer, "]}");
        cfgfile.n_append(buffer, os_strlen(buffer));
        espmem.stack_mon();
    }
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

    if (!restore_prgm_list())
    {
        // CTRL DEFAULT
        esp_diag.warn(TEMP_CTRL_INIT_DEFAULT_PRGM_LIST);
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
//     "period_count": 3,
//     "periods": [
//         {"wd": 8,"b":1880,"e":1880,"sp":200},
//         {"wd": 8,"b":1880,"e":1880,"sp":200},
//         {"wd": 8,"b":1880,"e":1880,"sp":200}
//     ]
// }

struct prgm *load_program(int prg_id)
{
    if (program_lst->size() == 0)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_NO_PROGRAM_LIST);
        ERROR("load_program no program list");
        return NULL;
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
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_NO_PROGRAM_ID, prg_id);
        ERROR("load_program no program id %d", prg_id);
        return NULL;
    }
    char filename[32];
    os_memset(filename, 0, 32);
    fs_snprintf(filename, 31, "program_%d.prg", prg_id);

    // os_strncpy(filename, cur_heading->desc, 31);
    // // replace all chars different by letters or numbers with '_'
    // int count;
    // for (count = 0; count < 32; count++)
    // {
    //     if (('0' <= filename[count]) && (filename[count] <= '9') ||
    //         ('A' <= filename[count]) && (filename[count] <= 'Z') ||
    //         ('a' <= filename[count]) && (filename[count] <= 'z') ||
    //         (filename[count] == 0))
    //         continue;
    //     else
    //         filename[count] = '_';
    // }

    TRACE("load_program filename %s", filename);
    // now look for file
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_FS_NOT_AVAILABLE);
        ERROR("load_program FS not available");
        return NULL;
    }
    File_to_json cfgfile(filename);
    if (!cfgfile.exists())
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_CANNOT_OPEN_FILE);
        ERROR("load_program cannot open %s", filename);
        return NULL;
    }
    // id
    if (cfgfile.find_string(f_str("id")))
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE);
        ERROR("load_program cannot find \"id\"");
        return NULL;
    }
    int tmp_id = atoi(cfgfile.get_value());
    if (prg_id != tmp_id)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE);
        ERROR("load_program wrong id \"id\"", atoi(cfgfile.get_value()));
        return NULL;
    }
    // min_temp
    if (cfgfile.find_string(f_str("min_temp")))
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE);
        ERROR("load_program cannot find \"min_temp\"");
        return NULL;
    }
    int tmp_min_temp = atoi(cfgfile.get_value());

    // period_count
    if (cfgfile.find_string(f_str("period_count")))
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE);
        ERROR("load_program cannot find \"period_count\"");
        return NULL;
    }
    int tmp_period_count = atoi(cfgfile.get_value());

    // periods
    if (cfgfile.find_string(f_str("periods")))
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE);
        ERROR("load_program cannot find \"periods\"");
        return NULL;
    }
    Json_array_str tmp_periods(cfgfile.get_value(), os_strlen(cfgfile.get_value()));

    if ((tmp_periods.syntax_check() != JSON_SINTAX_OK) || (tmp_periods.size() != tmp_period_count))
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE);
        ERROR("load_program incorrect array sintax");
        return NULL;
    }

    struct prgm *new_prg = new struct prgm;
    if (new_prg == NULL)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_HEAP_EXHAUSTED, sizeof(struct prgm));
        ERROR("load_program heap exhausted %d", sizeof(struct prgm));
        return NULL;
    }
    new_prg->id = prg_id;
    new_prg->min_temp = tmp_min_temp;
    new_prg->period_count = tmp_period_count;
    // there are no period in the program
    if (new_prg->period_count == 0)
        return new_prg;
    // there are periods in the program
    new_prg->periods = new struct prgm_period[tmp_period_count];
    if (new_prg->periods == NULL)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_HEAP_EXHAUSTED, sizeof(struct prgm_period[tmp_period_count]));
        ERROR("load_program heap exhausted %d", sizeof(struct prgm_period[tmp_period_count]));
        return NULL;
    }
    int count;
    for (count = 0; count < tmp_period_count; count++)
    {
        if ((tmp_periods.get_elem(count) == NULL) || (tmp_periods.get_elem_type(count) != JSON_OBJECT))
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        Json_str periods(tmp_periods.get_elem(count), tmp_periods.get_elem_len(count));
        if (periods.find_pair(f_str("wd")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (periods.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->periods[count].day_of_week = (week_days)atoi(periods.get_cur_pair_value());
        if (periods.find_pair(f_str("b")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (periods.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->periods[count].mm_start = (week_days)atoi(periods.get_cur_pair_value());
        if (periods.find_pair(f_str("e")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (periods.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->periods[count].mm_end = (week_days)atoi(periods.get_cur_pair_value());
        if (periods.find_pair(f_str("sp")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (periods.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->periods[count].setpoint = (week_days)atoi(periods.get_cur_pair_value());
    }
    espmem.stack_mon();
    return new_prg;
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

static bool save_program(struct prgm *prg)
{
    char filename[32];
    os_memset(filename, 0, 32);
    fs_snprintf(filename, 31, "program_%d.prg", prg->id);

    TRACE("save_program filename %s", filename);
    // now look for file
    if (!espfs.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_PRGM_FS_NOT_AVAILABLE);
        ERROR("save_program FS not available");
        return false;
    }
    if (Ffile::exists(&espfs, filename))
    {
        Ffile cfgfile(&espfs, filename);
        cfgfile.remove();
    }
    Ffile cfgfile(&espfs, filename);
    if (!cfgfile.is_available())
    {
        esp_diag.error(TEMP_CTRL_SAVE_PRGM_CANNOT_OPEN_FILE);
        ERROR("save_program cannot open %s", filename);
        return false;
    }

    // PROGRAM
    // {
    //     "id": 1,
    //     "min_temp": 100,
    //     "period_count": 3,
    //     "periods": [
    //         {"wd": 8,"b":1880,"e":1880,"sp":200},
    //         {"wd": 8,"b":1880,"e":1880,"sp":200},
    //         {"wd": 8,"b":1880,"e":1880,"sp":200}
    //     ]
    // }
    {
        //  {"id": ,"min_temp": ,"period_count": ,"periods": [
        char buffer[(50 + 1 + 2 + 5 + 5)];
        fs_sprintf(buffer,
                   "{\"id\": %d,\"min_temp\": %d,\"period_count\": %d,\"periods\": [",
                   prg->id,
                   prg->min_temp,
                   prg->period_count);
        cfgfile.n_append(buffer, os_strlen(buffer));
        espmem.stack_mon();
    }
    int idx;
    bool first_time = true;
    for (idx = 0; idx < prg->period_count; idx++)
    {
        //  ,{"wd":,"b":,"e":,"sp":}
        char buffer[(24 + 1 + 1 + 4 + 4 + 5)];
        if (first_time)
        {
            first_time = false;
            fs_sprintf(buffer,
                       "{\"wd\":%d,\"b\":%d,\"e\":%d,\"sp\":%d}",
                       prg->periods[idx].day_of_week,
                       prg->periods[idx].mm_start,
                       prg->periods[idx].mm_end,
                       prg->periods[idx].setpoint);
        }
        else
            fs_sprintf(buffer,
                       ",{\"wd\":%d,\"b\":%d,\"e\":%d,\"sp\":%d}",
                       prg->periods[idx].day_of_week,
                       prg->periods[idx].mm_start,
                       prg->periods[idx].mm_end,
                       prg->periods[idx].setpoint);

        cfgfile.n_append(buffer, os_strlen(buffer));
        espmem.stack_mon();
    }
    {
        //  ]}
        char buffer[(2 + 1)];
        fs_sprintf(buffer, "]}");
        cfgfile.n_append(buffer, os_strlen(buffer));
        espmem.stack_mon();
    }
    return true;
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
    if (!save_program(prg))
        return ERR_SAVING_PRG;
    // add program heading
    struct prgm_headings *heading_ptr = new struct prgm_headings;
    if (heading_ptr == NULL)
    {
        esp_diag.error(TEMP_CTRL_ADD_PRGM_HEAP_EXHAUSTED, sizeof(struct prgm_headings));
        ERROR("add_program heap memory exhausted %d", sizeof(struct prgm_headings));
        return ERR_MEM_EXHAUSTED;
    }
    heading_ptr->id = prg_id;
    os_strncpy(heading_ptr->desc, name, 31);
    program_lst->push_back(heading_ptr);
    if (saved_prgm_list_not_updated())
        save_prgm_list();
    espmem.stack_mon();
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
    if (saved_prgm_list_not_updated())
        save_prgm_list();
    // override program id
    prg->id = prg_id;
    // write program file
    if (!save_program(prg))
        return ERR_SAVING_PRG;
    espmem.stack_mon();
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
    if (espfs.is_available())
    {
        if (Ffile::exists(&espfs, filename))
        {
            Ffile cfgfile(&espfs, filename);
            cfgfile.remove();
        }
    }
    else
    {
        esp_diag.error(TEMP_CTRL_DEL_PRGM_FS_NOT_AVAILABLE);
        ERROR("del_program FS not available");
    }
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
    espmem.stack_mon();
    if (heading_found)
    {
        // return the program_id
        program_lst->remove();
        if (saved_prgm_list_not_updated())
            save_prgm_list();
        return prg_id;
    }
    else
    {
        return ERR_PRG_NOT_FOUND;
    }
}