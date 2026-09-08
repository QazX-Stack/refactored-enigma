#include "stm32f1xx_hal.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>
#include "usart.h"

#define BLUE_SERIAL_BUFFER_SIZE			100		// 环形缓冲区大小

uint8_t BlueSerial_Buffer[BLUE_SERIAL_BUFFER_SIZE];
uint16_t BlueSerial_PutIndex = 0;				// 环形缓冲区写指针
uint16_t BlueSerial_GetIndex = 0;				// 环形缓冲区读指针
uint8_t BlueSerial_RxData;						// 中断接收暂存字节

extern UART_HandleTypeDef huart2;
char BlueSerial_String[100];					// 解析后的完整帧（不含首尾括号）
char BlueSerial_StringArray[6][20];				// 按逗号分割后的子串数组
uint16_t BlueSerial_RxCount = 0;				/* 接收字节计数器, 用于调试 */

void BlueSerial_IRQHandler(uint8_t RxData);

/* 启动 UART2 中断接收 (需在 MX_USART2_UART_Init 之后调用) */
void BlueSerial_Init(void)
{
    HAL_UART_Receive_IT(&huart2, &BlueSerial_RxData, 1);
}

/**
  * @brief  通过蓝牙串口发送一个字节
  * @param  Byte 要发送的字节
  * @retval 无
  */
void BlueSerial_SendByte(uint8_t Byte)
{
    HAL_UART_Transmit(&huart2, &Byte, 1, HAL_MAX_DELAY);
}

/* UART2 接收完成回调: 处理收到的字节, 重新使能下一次接收 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		BlueSerial_IRQHandler(BlueSerial_RxData);
		HAL_UART_Receive_IT(&huart2, &BlueSerial_RxData, 1);
	}
}

/* UART2 错误回调: 发生错误(噪声/帧错误/溢出)时重新使能接收, 防止接收永久停止 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		/* 清除可能残留的错误标志后重新启动中断接收 */
		__HAL_UART_CLEAR_FLAG(huart, UART_FLAG_ORE | UART_FLAG_NE | UART_FLAG_FE);
		HAL_UART_Receive_IT(&huart2, &BlueSerial_RxData, 1);
	}
}

/* ====================== 环形缓冲区操作 ====================== */

/**
  * @brief  向环形缓冲区写入一个字节
  * @param  Byte 要写入的字节
  * @retval 0=成功，1=缓冲区满
  */
uint8_t BlueSerial_Put(uint8_t Byte)
{
    if ((BlueSerial_PutIndex + 1) % BLUE_SERIAL_BUFFER_SIZE == BlueSerial_GetIndex)
    {
        return 1;	// 缓冲区满
    }
	BlueSerial_Buffer[BlueSerial_PutIndex] = Byte;
	if (BlueSerial_PutIndex >= BLUE_SERIAL_BUFFER_SIZE - 1)
	{
		BlueSerial_PutIndex = 0;
	}
	else
	{
		BlueSerial_PutIndex ++;
	}
	return 0;
}

/**
  * @brief  从环形缓冲区读取一个字节（读取后移除）
  * @param  Byte 输出参数，存放读取到的字节
  * @retval 0=成功，1=缓冲区空
  */
uint8_t BlueSerial_Get(uint8_t *Byte)
{
	if (BlueSerial_GetIndex == BlueSerial_PutIndex)
	{
		*Byte = 0;
	    return 1;	// 缓冲区空
	}
	*Byte = BlueSerial_Buffer[BlueSerial_GetIndex];
//	BlueSerial_Buffer[BlueSerial_GetIndex] = 0x00;
	if (BlueSerial_GetIndex >= BLUE_SERIAL_BUFFER_SIZE - 1)
	{
		BlueSerial_GetIndex = 0;
	}
	else
	{
		BlueSerial_GetIndex ++;
	}
	return 0;
}

/**
  * @brief  获取环形缓冲区中的数据长度
  * @param  无
  * @retval 缓冲区中未读取的字节数
  */
uint16_t BlueSerial_Length(void)
{
	return (BlueSerial_PutIndex + BLUE_SERIAL_BUFFER_SIZE - BlueSerial_GetIndex) % BLUE_SERIAL_BUFFER_SIZE;;
}

/**
  * @brief  查看环形缓冲区中指定偏移处的字节（不移除）
  * @param  Index 偏移量（相对于读指针）
  * @retval 指定位置的字节值
  */
uint8_t BlueSerial_Read(uint16_t Index)
{
	return BlueSerial_Buffer[(BlueSerial_GetIndex + Index) % BLUE_SERIAL_BUFFER_SIZE];
}

/**
  * @brief  清空环形缓冲区
  * @param  无
  * @retval 无
  */
void BlueSerial_ClearBuffer(void)
{
	uint8_t i;
	for (i = 0; i < BLUE_SERIAL_BUFFER_SIZE; i ++)
	{
		BlueSerial_Buffer[i] = 0;
	}
}

/* ====================== 数据发送 ====================== */

/**
  * @brief  通过蓝牙串口发送字节数组
  * @param  Array 要发送的数据数组
  * @param  Length 发送长度
  * @retval 无
  */
void BlueSerial_SendArray(uint8_t *Array, uint16_t Length)
{
	HAL_UART_Transmit(&huart2, Array, Length, HAL_MAX_DELAY);
}

/**
  * @brief  通过蓝牙串口发送字符串
  * @param  String 要发送的字符串（以'\0'结尾）
  * @retval 无
  */
void BlueSerial_SendString(char *String)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)String, strlen(String), HAL_MAX_DELAY);
}

/**
  * @brief  蓝牙串口格式化打印（类似printf）
  * @param  format 格式化字符串
  * @param  ... 可变参数列表
  * @retval 无
  */
void BlueSerial_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	BlueSerial_SendString(String);
}

/* ====================== 数据接收与帧解析 ====================== */

/**
  * @brief  检查是否接收到完整的数据帧（以'['开头、']'结尾）
  * @param  无
  * @retval 1=收到完整帧，0=未收到
  */
uint8_t BlueSerial_ReceiveFlag(void)
{
	uint8_t Flag = 0;
	uint16_t Length = BlueSerial_Length();
	for (uint16_t i = 0; i < Length; i ++)
	{
		if (BlueSerial_Read(i) == '[')
		{
			Flag = 1;
		}
		else if (BlueSerial_Read(i) == ']' && Flag == 1)
		{
			return 1;	// 找到完整帧
		}
	}
	return 0;
}

/**
  * @brief  接收并解析蓝牙数据帧
  * @param  无
  * @retval 无
  * @note   帧格式：[xxx,yyy,zzz,...]（括号内逗号分隔）
  *         解析后：BlueSerial_String 存放去括号的完整内容
  *               BlueSerial_StringArray[][] 存放按逗号分割的各子串
  */
void BlueSerial_Receive(void)
{
	uint16_t p = 0, k = 0;
	uint8_t Flag = 0;
	uint16_t Length = BlueSerial_Length();
	/* 从缓冲区逐字节读取直到完整帧结束 */
	for (uint16_t i = 0; i < Length; i ++)
	{
		uint8_t Byte;
		BlueSerial_Get(&Byte);
		if (Byte == '[')				// 帧头
		{
			Flag = 1;
			p = 0;
		}
		else if (Byte == ']' && Flag == 1)	// 帧尾
		{
			BlueSerial_String[p] = '\0';
			break;
		}
		else if (Flag == 1)				// 帧内数据
		{
			BlueSerial_String[p] = Byte;
			p ++;
		}
	}

	/* 按逗号分割为子串数组 */
	p = 0;
	for (uint16_t i = 0; BlueSerial_String[i] != '\0'; i ++)
	{
		if (BlueSerial_String[i] == ',')	// 遇到逗号，结束当前子串
		{
			BlueSerial_StringArray[p][k] = '\0';
			p ++;
			k = 0;
		}
		else								// 普通字符，追加到当前子串
		{
			BlueSerial_StringArray[p][k] = BlueSerial_String[i];
			k ++;
		}
	}
	BlueSerial_StringArray[p][k] = '\0';	// 最后一个子串结束符
}

/**
  * @brief  串口中断服务函数（由HAL_UART_RxCpltCallback调用）
  * @param  RxData 收到的字节
  * @retval 无
  */
void BlueSerial_IRQHandler(uint8_t RxData)
{
	BlueSerial_RxCount ++;		/* 调试: 每收到一个字节计数+1 */
	BlueSerial_Put(RxData);		// 写入环形缓冲区
}
