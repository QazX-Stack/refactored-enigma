#include "Serial.h"
#include "usart.h"      // 引用 CubeMX 生成的 usart.h，里面声明了 huart1
#include <stdarg.h>
#include <string.h>

extern UART_HandleTypeDef huart1;   // 声明外部变量，实际定义在 usart.c 中

// 发送单字节
void Serial_SendByte(uint8_t Byte)
{
    HAL_UART_Transmit(&huart1, &Byte, 1, HAL_MAX_DELAY);
}

// 发送数组
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    HAL_UART_Transmit(&huart1, Array, Length, HAL_MAX_DELAY);
}

// 发送字符串
void Serial_SendString(char *String)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)String, strlen(String), HAL_MAX_DELAY);
}

// 次方计算（内部使用）
static uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

// 发送数字（十进制），可指定长度
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
    {
        uint8_t ch = (Number / Serial_Pow(10, Length - i - 1)) % 10 + '0';
        Serial_SendByte(ch);
    }
}

// printf 重定向，需要勾选 MicroLIB
int fputc(int ch, FILE *f)
{
    Serial_SendByte((uint8_t)ch);
    return ch;
}

// 格式化打印
void Serial_Printf(char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    Serial_SendString(buffer);
}

