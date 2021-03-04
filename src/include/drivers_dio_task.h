/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __DIO_TASK_H__
#define __DIO_TASK_H__

#define DIO_TASK_QUEUE_LEN 4
#define SIG_DO_SEQ_COMPLETED 1
#define SIG_DI_SEQ_COMPLETED 2

typedef enum
{
  direct = 0,
  task
} CB_call_type;

void init_dio_task(void);

#endif