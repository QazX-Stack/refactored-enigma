#include "My_usart.h"
#include "pid.h"
#include "vofa.h"
#include "My_can.h"
#include <stdlib.h>
#include "motor.h"

Struct_UART_Manage_Object UART2_Manage_Object = {0};
 
uint8_t UART2_Tx_Data[256];

void Uart_Init(UART_HandleTypeDef *huart, uint8_t *Rx_Buffer, uint16_t Rx_Buffer_Size, UART_Call_Back Callback_Function)
{
	if (huart->Instance == USART2)
    {
        UART2_Manage_Object.Rx_Buffer = Rx_Buffer;
        UART2_Manage_Object.Rx_Buffer_Size = Rx_Buffer_Size;
        UART2_Manage_Object.UART_Handler = huart;
        UART2_Manage_Object.Callback_Function = Callback_Function;
    }
    HAL_UARTEx_ReceiveToIdle_DMA(huart, Rx_Buffer, Rx_Buffer_Size);
}


void UART_Tuning_Callback(uint8_t *Buffer, uint16_t Length)
{
    char cmd[32];
    char field;
    float value;
    uint16_t i;

    if (Length == 0) return;

    for (i = 0; i < Length && i < sizeof(cmd) - 1; i++)
    {
        if (Buffer[i] == '\r' || Buffer[i] == '\n') break;
        cmd[i] = (Buffer[i] >= 'a' && Buffer[i] <= 'z') ? (Buffer[i] - 32) : Buffer[i];
    }
    cmd[i] = '\0';

    if (cmd[0] == 'X')
    {
        PID_Omega.Kp = 0.0f;
        PID_Omega.Ki = 0.0f;
        PID_Omega.Kd = 0.0f;
				PID_Omega.Kf =0.0f;
        return;
    }

		if (i < 2) return;
		
    field = cmd[0];
    value = (float)atof(&cmd[1]);
		
        switch (field)
        {
        case 'P': PID_Omega.Kp     = value; break;
        case 'I': PID_Omega.Ki     = value; break;
        case 'D': PID_Omega.Kd     = value; break;
				case 'F': PID_Omega.Kf     = value; break;
        case 'T': PID_Omega.Target = value; break;
        case 'O': PID_Omega.OutMax = value; break;
        case 'M': PID_Omega.OutMin = value; break;
				case 'B': PID_Omega.OutOffset = value; break;
        }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
		if(huart->Instance == USART2)
		{
        if (UART2_Manage_Object.Callback_Function != NULL)
        {
            UART2_Manage_Object.Callback_Function(UART2_Manage_Object.Rx_Buffer, Size);
        }
        HAL_UARTEx_ReceiveToIdle_DMA(huart, UART2_Manage_Object.Rx_Buffer, UART2_Manage_Object.Rx_Buffer_Size);
     
    }		
}
