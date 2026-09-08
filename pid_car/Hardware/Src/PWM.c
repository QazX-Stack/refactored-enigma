#include "stm32f1xx_hal.h"
#include "tim.h"

/**
  * @brief  设置定时器2通道1的PWM比较值
  * @param  Compare 比较值，范围：0~999（对应0%~100%占空比）
  * @retval 无
  */
void PWM_SetCompare1(uint16_t Compare)
{
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, Compare);
}

/**
  * @brief  设置定时器2通道2的PWM比较值
  * @param  Compare 比较值，范围：0~999（对应0%~100%占空比）
  * @retval 无
  */
void PWM_SetCompare2(uint16_t Compare)
{
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, Compare);
}
