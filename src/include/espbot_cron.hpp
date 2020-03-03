/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __APP_CRON_HPP__
#define __APP_CRON_HPP__

#define CRON_MAX_JOBS 10

struct date
{
    int year;
    char month;
    char day_of_month;
    char hours;
    char minutes;
    char seconds;
    char day_of_week;
    uint32 timestamp;
};

void cron_init(void); 

/*
 * will sync to time provided by NTP
 * once synced job execution will start
 */
void cron_sync(void); 

#define CRON_STAR 0xFF

/*
 * # job definition:
 * # .---------------- minute (0 - 59)
 * # |  .------------- hour (0 - 23)
 * # |  |  .---------- day of month (1 - 31)
 * # |  |  |  .------- month (1 - 12) OR jan,feb,mar,apr ...
 * # |  |  |  |  .---- day of week (0 - 6) (Sunday=0 or 7) OR sun,mon,tue,wed,thu,fri,sat
 * # |  |  |  |  |
 * # *  *  *  *  * funcntion
 * 
 * result: > 0  -> job id
 *         < 0  -> error
 */

int cron_add_job(char min, char hour, char day_of_month, char month, char day_of_week, void (*command)(void));

/*
 * delete job_id
 */
void cron_del_job(int job_id);

/*
 * get current time as struct date
 */
struct date *get_current_time(void);

/*
 * force initialization of current time before than cron execution
 */
void init_current_time(void);


/*
 * CONFIGURATION & PERSISTENCY
 */
void enable_cron(void);
void start_cron(void);
void disable_cron(void);
void stop_cron(void);
bool cron_enabled(void);
int save_cron_cfg(void); // return CFG_OK on success, otherwise CFG_ERROR

/*
 * DEBUG
 */
void cron_print_jobs(void);

#endif