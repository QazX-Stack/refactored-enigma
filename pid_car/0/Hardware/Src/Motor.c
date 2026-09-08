#include "stm32f1xx_hal.h"
#include "PWM.h"

/**
  * @brief  设置电机PWM驱动（带方向控制）
  * @param  n 电机编号：1=电机1（PB12/PB13），2=电机2（PB14/PB15）
  * @param  Duty 占空比，范围：-99~99
  *              正数：正转，数值越大速度越快
  *              负数：反转，绝对值越大速度越快
  *              0：停止（刹车）
  * @retval 无
  */
void Motor_SetPWM(uint8_t n, int16_t Duty)
{
	if (n == 1)
	{
		if (Duty >= 0)
		{
			/* 电机1正转：PB13=0（低电平）, PB12=1（PWM输出） */
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
				if (Duty > 99) Duty = 99;
				PWM_SetCompare1((uint16_t)Duty);
		}
		else
		{
			/* 电机1反转：PB12=0（低电平）, PB13=1（PWM输出） */
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
				if (-Duty > 99) Duty = -99;
				PWM_SetCompare1((uint16_t)(-Duty));
		}
	}
	else if (n == 2)
	{
		if (Duty >= 0)
		{
			/* 电机2正转：PB14=0（低电平）, PB15=1（PWM输出） */
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
				if (Duty > 99) Duty = 99;
				PWM_SetCompare2((uint16_t)Duty);
		}
		else
		{
			/* 电机2反转：PB15=0（低电平）, PB14=1（PWM输出） */
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
				if (-Duty > 99) Duty = -99;
				PWM_SetCompare2((uint16_t)(-Duty));
		}
	}

}
