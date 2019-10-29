/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <quackmore-ff@yahoo.com> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you 
 * think this stuff is worth it, you can buy me a beer in return. Quackmore
 * ----------------------------------------------------------------------------
 */
#ifndef __DI_SEQUENCE_H__
#define __DI_SEQUENCE_H__

#include "library_dio_task.h"

typedef enum
{
    TIMEOUT_MS = 0,
    TIMEOUT_US
} Di_timeout_unit;

struct di_seq
{
    // please initialize these using new_di_seq
    int di_pin;
    int pulse_max_count;
    int timeout_val;
    Di_timeout_unit timeout_unit;

    // please initialize these using set_di_seq_cb
    void (*end_sequence_callack)(void *);
    void *end_sequence_callack_param;

    // do not initialize these are private members
    // arrays will be allocated here for storing input readings
    char *pulse_level;
    uint32 *pulse_duration;

    int current_pulse;
    os_timer_t timeout_timer;
    bool ended_by_timeout;
    bool callback_direct;
};

//
// the input sequence reading is surveilled by a timeout timer
// the timeout timer can be set in ms or us
// differences from the stop timeout function are meaningful
// for the execution of the sequence end callback
//   90-110 us when timeout timer is expressed in ms
//   40     us when timeout timer is expressed in ms
//

struct di_seq *new_di_seq(int pin, int num_pulses, int timeout_val, Di_timeout_unit timeout_unit); // allocating heap memory
void free_di_seq(struct di_seq *seq);                                                              // freeing allocated memory
void set_di_seq_cb(struct di_seq *seq, void (*cb)(void *), void *cb_param, CB_call_type cb_call);

void seq_di_clear(struct di_seq *seq); // will clear the pulse sequence only

int get_di_seq_length(struct di_seq *seq);
char get_di_seq_pulse_level(struct di_seq *seq, int idx);
uint32 get_di_seq_pulse_duration(struct di_seq *seq, int idx);

void read_di_sequence(struct di_seq *seq);
void stop_di_sequence_timeout(struct di_seq *seq);

//
// ############################ EXAMPLE ##########################
//
// // the callback function
//
// static void ICACHE_FLASH_ATTR input_seq_completed(void *param)
// {
//     struct di_seq *seq = (struct di_seq *)param;
//     // how to check how the input sequence reading ended
//     if (seq->ended_by_timeout)
//         os_printf("Input sequence reading ended by timeout timer\n");
//     else
//         os_printf("Input sequence reading completed\n");
//     // how to analyze the acquired input sequence
//     {
//         int idx = 0;
//         char level;
//         uint32 duration;
//         os_printf("Sequence defined as:\n");
//         for (idx = 0; idx < get_di_seq_length(seq); idx++)
//         {
//             level = get_di_seq_pulse_level(seq, idx);
//             duration = get_di_seq_pulse_duration(seq, idx);
//             if (level == ESPBOT_LOW)
//                 os_printf("pulse %d: level  'LOW' - duration %d\n", idx, duration);
//             else
//                 os_printf("pulse %d: level 'HIGH' - duration %d\n", idx, duration);
//         }
//         os_printf("Sequence end.\n");
//     }
//     free_di_seq(seq);  // <== don't forget to free memory
// }
//
// // the input sequence definition and reading
//
// {
//     ...
//     PIN_FUNC_SELECT(ESPBOT_D5_MUX, ESPBOT_D5_FUNC);
//     PIN_PULLUP_EN(ESPBOT_D5_MUX);
//     GPIO_DIS_OUTPUT(ESPBOT_D5_NUM);
//
//     struct di_seq *input_seq = new_di_seq(ESPBOT_D5_NUM, 9, 20, TIMEOUT_MS);
//     set_di_seq_cb(input_seq, input_seq_completed, (void *)input_seq);
//     read_di_sequence(input_seq);
//     ...
// }

#endif