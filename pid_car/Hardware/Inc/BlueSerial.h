#ifndef __BLUE_SERIAL_H
#define __BLUE_SERIAL_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

extern char BlueSerial_String[];			// 解析后的帧内容（去括号）
extern char BlueSerial_StringArray[][20];	// 按逗号分割的子串数组
extern uint16_t BlueSerial_RxCount;			// 接收字节计数（调试用）

/* ====================== 初始化 ====================== */
void BlueSerial_Init(void);

/* ====================== 数据发送 ====================== */
void BlueSerial_SendByte(uint8_t Byte);							// 发送单字节
void BlueSerial_SendArray(uint8_t *Array, uint16_t Length);		// 发送字节数组
void BlueSerial_SendString(char *String);						// 发送字符串
void BlueSerial_Printf(char *format, ...);						// 格式化打印

/* ====================== 环形缓冲区操作 ====================== */
uint8_t BlueSerial_Put(uint8_t Byte);		// 写入一字节（返回0=成功, 1=满）
uint8_t BlueSerial_Get(uint8_t *Byte);		// 读取一字节（返回0=成功, 1=空）
uint16_t BlueSerial_Length(void);			// 获取缓冲区数据长度
void BlueSerial_ClearBuffer(void);			// 清空缓冲区

/* ====================== 帧接收 ====================== */
uint8_t BlueSerial_ReceiveFlag(void);		// 检查是否收到完整帧（[...]格式）
void BlueSerial_Receive(void);				// 解析收到的帧

#endif
