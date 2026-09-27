#ifndef __My_usart_H
#define __My_usart_H

#include "stm32f4xx_hal.h"
#include "main.h"
#include "motor.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h> 

#define UART_BUFFER_SIZE 512

typedef void (*UART_Call_Back)(uint8_t *Buffer, uint16_t Length);

typedef struct {
    UART_HandleTypeDef *UART_Handler;
		uint8_t *Rx_Buffer;
    uint16_t Rx_Buffer_Size;
    UART_Call_Back Callback_Function;	
}Struct_UART_Manage_Object;


extern UART_HandleTypeDef huart2;

extern Struct_UART_Manage_Object UART2_Manage_Object;

extern uint8_t UART2_Tx_Data[];

void Uart_Init(UART_HandleTypeDef *huart, uint8_t *Rx_Buffer, uint16_t Rx_Buffer_Size, UART_Call_Back Callback_Function);
void UART_Tuning_Callback(uint8_t *Buffer, uint16_t Length);
uint8_t UART_Send_Data(UART_HandleTypeDef *huart, uint8_t *Data, uint16_t Length);

#endif
