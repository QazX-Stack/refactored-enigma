#include "stm32f1xx_hal.h"
#include "LED.h"

/**
  * @brief  点亮LED（PC13，低电平点亮）
  * @param  无
  * @retval 无
  */
void LED_ON(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

/**
  * @brief  熄灭LED（PC13，高电平熄灭）
  * @param  无
  * @retval 无
  */
void LED_OFF(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

/**
  * @brief  翻转LED状态（亮→灭，灭→亮）
  * @param  无
  * @retval 无
  */
void LED_Turn(void)
{
	if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 0)
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	}
}
