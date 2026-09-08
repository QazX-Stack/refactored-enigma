#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>
#include <stdint.h>

void Serial_Init(void);												// 初始化串口
void Serial_SendByte(uint8_t Byte);									// 发送单字节
void Serial_SendArray(uint8_t *Array, uint16_t Length);				// 发送字节数组
void Serial_SendString(char *String);								// 发送字符串
void Serial_SendNumber(uint32_t Number, uint8_t Length);			// 发送数字（十进制，可指定位数）
void Serial_Printf(char *format, ...);								// 格式化打印（类似printf）

#endif
