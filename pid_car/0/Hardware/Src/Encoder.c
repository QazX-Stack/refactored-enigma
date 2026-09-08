#include "Encoder.h"
#include "stm32f1xx_hal.h"
#include "tim.h"

/**
  * @brief  读取编码器计数值并清零（获取自上次读取以来的增量）
  * @param  n 编码器编号：1=编码器1(TIM3)，2=编码器2(TIM4)
  * @retval 编码器脉冲增量值（有符号），正数=正转，负数=反转
  */
int16_t Encoder_Get(uint8_t n)
{
	int16_t Temp;
	if (n == 1)
	{
		Temp = __HAL_TIM_GET_COUNTER(&htim3);
		__HAL_TIM_SET_COUNTER(&htim3, 0);
		return Temp;
	}
	else if (n == 2)
	{
		Temp = __HAL_TIM_GET_COUNTER(&htim4);
		__HAL_TIM_SET_COUNTER(&htim4, 0);
		return Temp;
	}
	return 0;
}
