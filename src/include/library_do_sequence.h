/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __DO_SEQUENCE_H__
#define __DO_SEQUENCE_H__

#include "library_dio_task.h"

struct do_seq
{
    // please initialize these using new_do_seq
    int do_pin;
    int pulse_max_count;
    // please initialize these using set_do_seq_cb
    void (*end_sequence_callack)(void *);
    void *end_sequence_callack_param;

    // initialize the sequence pulses using sequence_add method
    char *pulse_level;
    uint32 *pulse_duration;

    // do not initialize these are private members
    int pulse_count;
    os_timer_t pulse_timer;
    int current_pulse;
    int dig_output_initial_value;
    bool callback_direct;
};

struct do_seq *new_do_seq(int pin, int num_pulses); // allocating heap memory
void free_do_seq(struct do_seq *seq);               // freeing allocated memory
void set_do_seq_cb(struct do_seq *seq, void (*cb)(void *), void *cb_param, CB_call_type cb_call);

void out_seq_clear(struct do_seq *seq); // will clear the pulse sequence only

void out_seq_add(struct do_seq *seq, char level, uint32 duration);
int get_do_seq_length(struct do_seq *seq);
char get_do_seq_pulse_level(struct do_seq *seq, int idx);
uint32 get_do_seq_pulse_duration(struct do_seq *seq, int idx);

//
// when using sequence with pulse duration in milliseconds please consider that
//  - you are using an SW timers so you can run more than one sequence at a time
//  - pulse duration range is from 5 ms to 6.870.947 ms
//
// when using sequence with pulse duration in microseconds please consider that
//  - you are using an HW timer so you can run one sequence at a time
//  - pulse duration range is from 10 us to 199.999 us
//

void exe_do_seq_ms(struct do_seq *seq);
void exe_do_seq_us(struct do_seq *seq);

//
// ############################ EXAMPLE ##########################
//
// // end_sequence callback definition
// static void ICACHE_FLASH_ATTR sequence_completed(void *param)
// {
//     struct do_seq *seq = (struct do_seq *)param;
//
//     free_do_seq(seq);                 <== don't forget to free memory
//
//     os_printf("Test completed\n");
// }
//
// {
//     ...
//
//     // defining and running a sequence
//
//     PIN_FUNC_SELECT(ESPBOT_D4_MUX, ESPBOT_D4_FUNC);
//     struct do_seq *seq = new_do_seq(ESPBOT_D4_NUM, 4);
//     set_do_seq_cb(seq, sequence_completed, (void *)seq);
//
//     out_seq_clear(seq);
//     out_seq_add(seq, ESPBOT_LOW, 1000);
//     out_seq_add(seq, ESPBOT_HIGH, 1500);
//     out_seq_add(seq, ESPBOT_LOW, 2000);
//     out_seq_add(seq, ESPBOT_HIGH, 2500);
//
//     exe_do_seq_ms(seq);
//
//     // printing ax existing pulse sequence
//
//     int idx = 0;
//     char level;
//     uint32 duration;
//     os_printf("Sequence defined as:\n");
//     for (idx = 0; idx < get_do_seq_length(seq); idx++)
//     {
//         level = get_do_seq_pulse_level(seq, idx);
//         duration = get_do_seq_pulse_duration(seq, idx);
//         if (level == ESPBOT_LOW)
//             os_printf("pulse %d: level  'LOW' - duration %d\n", idx, duration);
//         else
//             os_printf("pulse %d: level 'HIGH' - duration %d\n", idx, duration);
//     }
//     os_printf("Sequence end.\n");
//
//     ...
// }

#endif