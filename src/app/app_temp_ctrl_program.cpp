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

    // prgm_count
    if (cfgfile.find_string(f_str("prgm_count")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE);
        ERROR("temp_control_restore_prgm_list cannot find \"prgm_count\"");
        return false;
    }
    int prg_count = atoi(cfgfile.get_value());

    // prgm_headings
    if (cfgfile.find_string(f_str("headings")))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE);
        ERROR("temp_control_restore_prgm_list cannot find \"headings\"");
        return false;
    }
    Json_array_str prg_array(cfgfile.get_value(), os_strlen(cfgfile.get_value()));
    if ((prg_array.syntax_check() != JSON_SINTAX_OK) || (prg_array.size() != prg_count))
    {
        esp_diag.error(TEMP_CTRL_RESTORE_PRGM_LIST_INCOMPLETE);
        ERROR("temp_control_restore_prgm_list incorrect array sintax");
        return false;
    }

    program_lst = new List<struct prgm_headings>(prg_count, delete_content);
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
        if (max_len > 32)
            max_len = 32;
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

    // prgm_count
    if (cfgfile.find_string(f_str("prgm_count")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_prgm_list_not_updated cannot find \"prgm_count\"");
        return true;
    }
    if (program_lst->size() != atoi(cfgfile.get_value()))
        return true;

    // prgm_headings
    if (cfgfile.find_string(f_str("headings")))
    {
        esp_diag.error(TEMP_CTRL_SAVED_PRGM_LIST_NOT_UPDATED_INCOMPLETE);
        ERROR("temp_control_saved_prgm_list_not_updated cannot find \"headings\"");
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
        if ((cur_heading->id != tmp_id) || (os_strncmp(cur_heading->desc, heading.get_cur_pair_value(), 32)))
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
        //  {"prgm_count": ,"headings": [
        char buffer[(29 + 1 + 2)];
        fs_sprintf(buffer,
                   "{\"prgm_count\": %d,\"headings\": [",
                   program_lst->size());
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
    int idx;
    bool first_time = true;
    struct prgm_headings *cur_heading = program_lst->front();
    for (idx = 0; idx < program_lst->size(); idx++)
    {
        //  ,{"id": ,"desc":""}
        char buffer[(19 + 1 + 2 + 32)];
        if (first_time)
            fs_sprintf(buffer,
                       "{\"id\": %d,\"desc\":\"%s\"",
                       cur_heading->id,
                       cur_heading->desc);
        else
            fs_sprintf(buffer,
                       ",{\"id\": %d,\"desc\":\"%s\"",
                       cur_heading->id,
                       cur_heading->desc);

        cfgfile.n_append(buffer, os_strlen(buffer));
        cur_heading = program_lst->next();
    }
    {
        //  ]}
        char buffer[(2 + 1)];
        fs_sprintf(buffer, "]}");
        cfgfile.n_append(buffer, os_strlen(buffer));
    }
}

// PROGRAM LIST
// {
//     "prgm_count": 3,
//     "headings": [
//         {"id": 1,"desc":"first"},
//         {"id": 2,"desc":"second"},
//         {"id": 3,"desc":"third"}
//     ]
// }

void init_program_list(void)
{
    ALL("init_program_list");

    program_lst = NULL;

    if (!restore_prgm_list())
    {
        // CTRL DEFAULT
        esp_diag.warn(TEMP_CTRL_INIT_DEFAULT_PRGM_LIST);
        WARN("init_program_list no ctrl cfg available");
    }
}

void delete_program(struct prgm *prog_ptr)
{
    if (prog_ptr == NULL)
        return;
    if (prog_ptr->period)
        delete[] prog_ptr->period;
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

struct prgm *load_program(int idx)
{
    if (program_lst == NULL)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_NO_PROGRAM_LIST);
        ERROR("load_program no program list");
        return NULL;
    }
    // search for program name
    bool program_not_found = true;
    struct prgm_headings *cur_heading = program_lst->front();
    while (cur_heading)
    {
        if (cur_heading->id == idx)
        {
            program_not_found = false;
            break;
        }
        cur_heading = program_lst->next();
    }
    // check if a program was found
    if (program_not_found)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_NO_PROGRAM_ID, idx);
        ERROR("load_program no program id %d", idx);
        return NULL;
    }
    char filename[33];
    os_memset(filename, 0, 33);
    os_strncpy(filename, cur_heading->desc, 32);
    // replace all chars different by letters or numbers with '_'
    int count;
    for (count = 0; count < 33; count++)
    {
        if (('0' <= filename[count]) && (filename[count] <= '9') ||
            ('A' <= filename[count]) && (filename[count] <= 'Z') ||
            ('a' <= filename[count]) && (filename[count] <= 'z') ||
            (filename[count] == 0))
            continue;
        else
            filename[count] = '_';
    }
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
    if (idx != tmp_id)
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

    struct prgm *new_prg = new struct prgm[tmp_period_count];
    if (new_prg == NULL)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_HEAP_EXHAUSTED, sizeof(struct prgm[tmp_period_count]));
        ERROR("load_program heap exhausted %d", sizeof(struct prgm[tmp_period_count]));
        return NULL;
    }
    new_prg->id = idx;
    new_prg->min_temp = tmp_min_temp;
    new_prg->period_count = tmp_period_count;
    new_prg->period = new struct prgm_period[tmp_period_count];
    if (new_prg->period == NULL)
    {
        esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_HEAP_EXHAUSTED, sizeof(struct prgm_period[tmp_period_count]));
        ERROR("load_program heap exhausted %d", sizeof(struct prgm_period[tmp_period_count]));
        return NULL;
    }

    for (count = 0; count < program_lst->size(); count++)
    {
        if ((tmp_periods.get_elem(count) == NULL) || (tmp_periods.get_elem_type(count) != JSON_OBJECT))
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        Json_str period(tmp_periods.get_elem(count), tmp_periods.get_elem_len(count));
        if (period.find_pair(f_str("wd")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->period[count].day_of_week = (week_days)atoi(period.get_cur_pair_value());
        if (period.find_pair(f_str("b")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->period[count].mm_start = (week_days)atoi(period.get_cur_pair_value());
        if (period.find_pair(f_str("e")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->period[count].mm_end = (week_days)atoi(period.get_cur_pair_value());
        if (period.find_pair(f_str("sp")) != JSON_NEW_PAIR_FOUND)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        if (period.get_cur_pair_value_type() != JSON_INTEGER)
        {
            esp_diag.error(TEMP_CTRL_LOAD_PROGRAM_PRGM_INCOMPLETE, count);
            ERROR("load_program array[%d] bad sintax", count);
            delete_program(new_prg);
            return NULL;
        }
        new_prg->period[count].setpoint = (week_days)atoi(period.get_cur_pair_value());
    }
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