#include "sbus.h"
#include "My_usart.h"
#include "usart.h"
#include "vofa.h"

volatile uint16_t Sbus_Channel[SBUS_CHANNEL_NUM] = {0};
volatile uint8_t  Sbus_Frame_Lost = 0;
volatile uint8_t  Sbus_Failsafe = 0;
volatile uint32_t Sbus_Stage = 0;
volatile uint32_t Sbus_Frame_Count = 0;
volatile uint32_t Sbus_Error_Count = 0;
volatile uint16_t Sbus_Last_Length = 0;
volatile uint32_t Sbus_Last_Rx_Tick = 0;
volatile uint8_t  Sbus_Raw[8] = {0};
volatile uint8_t  Sbus_Last_Byte = 0;

uint8_t Sbus_Vofa_Enable = 0;

static uint8_t Sbus_Byte = 0;
static uint8_t Sbus_Raw_Index = 0;
static uint8_t Sbus_Since_Header = 0;
static uint8_t Sbus_Index = 0;
static uint8_t Sbus_Frame[SBUS_FRAME_SIZE];

static void Sbus_Decode(const uint8_t *Frame)
{
    uint8_t  ch;
    uint8_t  byte;
    uint8_t  shift;
    uint32_t v;

    Sbus_Frame_Lost = (Frame[23] & 0x04) ? 1 : 0;
    Sbus_Failsafe   = (Frame[23] & 0x08) ? 1 : 0;

    for (ch = 0; ch < SBUS_CHANNEL_NUM; ch++)
    {
        byte  = (uint8_t)(((uint16_t)ch * 11) >> 3);
        shift = (uint8_t)(((uint16_t)ch * 11) & 7);

        v = (uint32_t)Frame[1 + byte]
          | ((uint32_t)Frame[2 + byte] << 8)
          | ((uint32_t)Frame[3 + byte] << 16);

        Sbus_Channel[ch] = (uint16_t)((v >> shift) & 0x7FF);
    }
}

void Sbus_Init(void)
{
    (void)huart1.Instance->SR;
    (void)huart1.Instance->DR;

    UART1_Manage_Object.UART_Handler = &huart1;
    UART1_Manage_Object.Rx_Buffer = &Sbus_Byte;
    UART1_Manage_Object.Rx_Buffer_Size = 1;

    Sbus_Index = 0;

    if (HAL_UART_Receive_IT(&huart1, &Sbus_Byte, 1) == HAL_OK)
    {
        Sbus_Stage = 1;
    }
    else
    {
        Sbus_Stage = 101;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t b;

    if (huart->Instance != USART1)
    {
        return;
    }

    b = Sbus_Byte;

    Sbus_Last_Byte = b;

    Sbus_Raw[Sbus_Raw_Index] = b;
    Sbus_Raw_Index++;
    if (Sbus_Raw_Index >= 8)
    {
        Sbus_Raw_Index = 0;
    }

    Sbus_Since_Header++;

    if (Sbus_Stage < 2)
    {
        Sbus_Stage = 2;
    }

    if (b == SBUS_HEADER)
    {
        Sbus_Last_Length = Sbus_Since_Header;
        Sbus_Since_Header = 0;

        Sbus_Frame[0] = b;
        Sbus_Index = 1;

        if (Sbus_Stage < 4)
        {
            Sbus_Stage = 4;
        }
    }
    else if (Sbus_Index > 0)
    {
        Sbus_Frame[Sbus_Index] = b;
        Sbus_Index++;

        if (Sbus_Index >= SBUS_FRAME_SIZE)
        {
            Sbus_Index = 0;

            if (Sbus_Frame[SBUS_FRAME_SIZE - 1] == 0x00)
            {
                Sbus_Decode(Sbus_Frame);
                Sbus_Last_Rx_Tick = HAL_GetTick();
                Sbus_Frame_Count++;

                if (Sbus_Stage < 5)
                {
                    Sbus_Stage = 5;
                }
            }
            else
            {
                Sbus_Error_Count++;
            }
        }
    }
    else
    {
        Sbus_Error_Count++;
    }

    HAL_UART_Receive_IT(&huart1, &Sbus_Byte, 1);
}

uint8_t Sbus_Is_Connected(void)
{
    if (Sbus_Frame_Count == 0)
    {
        return 0;
    }

    if (Sbus_Failsafe)
    {
        return 0;
    }

    if ((HAL_GetTick() - Sbus_Last_Rx_Tick) > SBUS_TIMEOUT_MS)
    {
        return 0;
    }

    return 1;
}

void Sbus_Vofa_Send(void)
{
    static uint8_t Divider = 0;

    uint8_t i;
    float   data[SBUS_CHANNEL_NUM];

    Divider++;
    if (Divider < SBUS_VOFA_DIVIDER)
    {
        return;
    }
    Divider = 0;

    for (i = 0; i < SBUS_CHANNEL_NUM; i++)
    {
        data[i] = (float)Sbus_Channel[i];
    }

    vofa_send_data(data, SBUS_CHANNEL_NUM);
}
