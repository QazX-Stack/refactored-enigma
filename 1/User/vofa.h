#ifndef __VOFA_H
#define __VOFA_H

#include "main.h"
#include <string.h>

#include <stdint.h>

#define VOFA_CHANNEL_COUNT  4          
#define VOFA_DATA_SIZE      (VOFA_CHANNEL_COUNT * sizeof(float))
#define VOFA_TAIL_SIZE      4
#define VOFA_BUFFER_SIZE    (VOFA_DATA_SIZE + VOFA_TAIL_SIZE)

extern volatile uint8_t vofa_send_complete;

void vofa_init(void);
void vofa_send_motor_data(float A, float B, float C,float D);
void vofa_send_data(float* data, uint8_t channel_count);
void vofa_wait_for_complete(void);  

#endif

