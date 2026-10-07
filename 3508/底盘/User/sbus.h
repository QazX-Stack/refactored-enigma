#ifndef __SBUS_H
#define __SBUS_H

#include "stm32f4xx_hal.h"

#define SBUS_HEADER         0x0F
#define SBUS_FRAME_SIZE     25
#define SBUS_CHANNEL_NUM    16
#define SBUS_RX_BUFFER_SIZE 64
#define SBUS_VOFA_DIVIDER   10
#define SBUS_TIMEOUT_MS     100

#define SBUS_VALUE_MIN      172
#define SBUS_VALUE_MID      992
#define SBUS_VALUE_MAX      1811

extern volatile uint16_t Sbus_Channel[SBUS_CHANNEL_NUM];
extern volatile uint8_t  Sbus_Frame_Lost;
extern volatile uint8_t  Sbus_Failsafe;
extern volatile uint32_t Sbus_Stage;
extern volatile uint32_t Sbus_Frame_Count;
extern volatile uint32_t Sbus_Error_Count;
extern volatile uint16_t Sbus_Last_Length;
extern volatile uint32_t Sbus_Last_Rx_Tick;
extern volatile uint8_t  Sbus_Raw[8];
extern volatile uint8_t  Sbus_Last_Byte;

extern uint8_t Sbus_Vofa_Enable;

void    Sbus_Init(void);
void    Sbus_Vofa_Send(void);
uint8_t Sbus_Is_Connected(void);

#endif
