#include "My_usart.h"
#include "pid.h"
#include "vofa.h"
#include "My_can.h"
#include <stdlib.h>
#include "motor.h"
#include "chassis.h"
#include "sbus.h"

Struct_UART_Manage_Object UART1_Manage_Object = {0};
Struct_UART_Manage_Object UART2_Manage_Object = {0};

uint8_t UART2_Tx_Data[256];

volatile uint32_t UART1_Error_Count = 0;
volatile uint32_t UART2_Error_Count = 0;

static float Chassis_Vx = 0.0f;
static float Chassis_Vy = 0.0f;
static float Chassis_Wz = 0.0f;

HAL_StatusTypeDef Uart_Init(UART_HandleTypeDef *huart, uint8_t *Rx_Buffer, uint16_t Rx_Buffer_Size, UART_Call_Back Callback_Function)
{
	if (huart->Instance == USART2)
    {
        UART2_Manage_Object.Rx_Buffer = Rx_Buffer;
        UART2_Manage_Object.Rx_Buffer_Size = Rx_Buffer_Size;
        UART2_Manage_Object.UART_Handler = huart;
        UART2_Manage_Object.Callback_Function = Callback_Function;
    }
    else if (huart->Instance == USART1)
    {
        UART1_Manage_Object.Rx_Buffer = Rx_Buffer;
        UART1_Manage_Object.Rx_Buffer_Size = Rx_Buffer_Size;
        UART1_Manage_Object.UART_Handler = huart;
        UART1_Manage_Object.Callback_Function = Callback_Function;
    }

    (void)huart->Instance->SR;
    (void)huart->Instance->DR;
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    return HAL_UARTEx_ReceiveToIdle_DMA(huart, Rx_Buffer, Rx_Buffer_Size);
}


void UART_Tuning_Callback(uint8_t *Buffer, uint16_t Length)
{
    char cmd[32];
    char field;
    float value;
    uint16_t i;
    uint8_t n;

    if (Length == 0) return;

    for (i = 0; i < Length && i < sizeof(cmd) - 1; i++)
    {
        if (Buffer[i] == '\r' || Buffer[i] == '\n') break;
        cmd[i] = (Buffer[i] >= 'a' && Buffer[i] <= 'z') ? (Buffer[i] - 32) : Buffer[i];
    }
    cmd[i] = '\0';

    if (cmd[0] == 'X')
    {
        for (n = 0; n < MOTOR_NUM; n++)
        {
            Motor[n].PID_Omega.Kp = 0.0f;
            Motor[n].PID_Omega.Ki = 0.0f;
            Motor[n].PID_Omega.Kd = 0.0f;
            Motor[n].PID_Omega.Kf = 0.0f;
        }
        return;
    }

    if (cmd[0] == 'S')
    {
        Motor_Vofa_Select = (uint8_t)atoi(&cmd[1]);
        if (Motor_Vofa_Select > MOTOR_NUM) Motor_Vofa_Select = 0;
        return;
    }

    if (cmd[0] == 'E')
    {
        Sbus_Vofa_Enable = (uint8_t)atoi(&cmd[1]);
        return;
    }

    if (cmd[0] == 'C')
    {
        Chassis_Input_Calibrate();
        return;
    }

    if (cmd[0] == 'Z')
    {
        Chassis_Vx = 0.0f;
        Chassis_Vy = 0.0f;
        Chassis_Wz = 0.0f;
        Chassis_Set_Velocity(0.0f, 0.0f, 0.0f);
        return;
    }

    if (i < 2) return;

    field = cmd[0];
    value = (float)atof(&cmd[1]);

    if (field == 'V' || field == 'Y' || field == 'W')
    {
        switch (field)
        {
            case 'V': Chassis_Vx = value; break;
            case 'Y': Chassis_Vy = value; break;
            case 'W': Chassis_Wz = value; break;
        }

        Chassis_Set_Velocity(Chassis_Vx, Chassis_Vy, Chassis_Wz);
        return;
    }

    for (n = 0; n < MOTOR_NUM; n++)
    {
        switch (field)
        {
            case 'P': Motor[n].PID_Omega.Kp        = value; break;
            case 'I': Motor[n].PID_Omega.Ki        = value; break;
            case 'D': Motor[n].PID_Omega.Kd        = value; break;
            case 'F': Motor[n].PID_Omega.Kf        = value; break;
            case 'B': Motor[n].PID_Omega.OutOffset = value; break;
            case 'O': Motor[n].PID_Omega.OutMax    = value; break;
            case 'M': Motor[n].PID_Omega.OutMin    = value; break;
        }
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
    else if (huart->Instance == USART1)
    {
        if (UART1_Manage_Object.Callback_Function != NULL)
        {
            UART1_Manage_Object.Callback_Function(UART1_Manage_Object.Rx_Buffer, Size);
        }
        HAL_UARTEx_ReceiveToIdle_DMA(huart, UART1_Manage_Object.Rx_Buffer, UART1_Manage_Object.Rx_Buffer_Size);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    (void)huart->Instance->SR;
    (void)huart->Instance->DR;
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    if (huart->Instance == USART2)
    {
        UART2_Error_Count++;
        HAL_UARTEx_ReceiveToIdle_DMA(huart, UART2_Manage_Object.Rx_Buffer, UART2_Manage_Object.Rx_Buffer_Size);
    }
    else if (huart->Instance == USART1)
    {
        UART1_Error_Count++;
        HAL_UART_Receive_IT(huart, UART1_Manage_Object.Rx_Buffer, 1);
    }
}
