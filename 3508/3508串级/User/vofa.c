#include "vofa.h"
#include "My_usart.h"   
#include "dma.h"     

static uint8_t vofa_tx_buffer[VOFA_BUFFER_SIZE];

volatile uint8_t vofa_send_complete = 1;  

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)  
    {
        vofa_send_complete = 1;  
		}
}

void vofa_init(void)
{
    vofa_send_complete = 1;
    memset(vofa_tx_buffer, 0, VOFA_BUFFER_SIZE);
}

void vofa_send_motor_data(float A, float B, float C,float D)
{
    if (!vofa_send_complete) {
        return;  
    }
    
    float data[VOFA_CHANNEL_COUNT] = {A, B, C,D};
    uint8_t tail[VOFA_TAIL_SIZE] = {0x00, 0x00, 0x80, 0x7F};
    
    memcpy(vofa_tx_buffer, data, VOFA_DATA_SIZE);
    memcpy(vofa_tx_buffer + VOFA_DATA_SIZE, tail, VOFA_TAIL_SIZE);
    
    vofa_send_complete = 0;
    
    HAL_UART_Transmit_DMA(&huart2, vofa_tx_buffer, VOFA_BUFFER_SIZE);
}

void vofa_send_data(float * data, uint8_t channel_count)
{
    if (!vofa_send_complete) return;
    
    uint16_t data_size = channel_count * sizeof(float);
    uint8_t tail[VOFA_TAIL_SIZE] = {0x00, 0x00, 0x80, 0x7F};
    
    memcpy(vofa_tx_buffer, data, data_size);
    memcpy(vofa_tx_buffer + data_size, tail, VOFA_TAIL_SIZE);
    
    vofa_send_complete = 0;
    HAL_UART_Transmit_DMA(&huart2, vofa_tx_buffer, data_size + VOFA_TAIL_SIZE);
}

void vofa_wait_for_complete(void)
{
    while (!vofa_send_complete);
}
