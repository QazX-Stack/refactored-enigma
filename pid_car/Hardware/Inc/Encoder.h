#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

void Encoder_Init(void);			// 初始化编码器（TIM3/TIM4 编码器模式）
int16_t Encoder_Get(uint8_t n);		// 读取编码器增量并清零（n=1/2）

#endif
