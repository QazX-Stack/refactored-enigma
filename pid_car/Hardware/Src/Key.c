#include "stm32f1xx_hal.h"                 // Device header
#include "Key.h"

#define KEY_PRESSED				1
#define KEY_UNPRESSED			0

/* 按键时间阈值（单位：Key_Tick调用周期≈1ms） */
#define KEY_TIME_DOUBLE			0		// 双击间隔时间
#define KEY_TIME_LONG			1000	// 长按判定时间（约1秒）
#define KEY_TIME_REPEAT			100		// 长按连发间隔（约100ms）

uint8_t Key_Flag[KEY_COUNT];			// 按键标志位数组，支持4个按键

/**
  * @brief  读取按键当前物理状态
  * @param  n 按键编号：KEY_1(PB1), KEY_2(PB0), KEY_3(PA5), KEY_4(PA4)
  * @retval KEY_PRESSED(1) 按下，KEY_UNPRESSED(0) 未按下
  */
uint8_t Key_GetState(uint8_t n)
{
	if (n == KEY_1)
	{
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1)  == 0)
		{
			return KEY_PRESSED;
		}
	}
	else if (n == KEY_2)
	{
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == 0)
		{
			return KEY_PRESSED;
		}
	}
	else if (n == KEY_3)
	{
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == 0)
		{
			return KEY_PRESSED;
		}
	}
	else if (n == KEY_4)
	{
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == 0)
		{
			return KEY_PRESSED;
		}
	}
	return KEY_UNPRESSED;
}

/**
  * @brief  检查按键标志位（读后自动清除，HOLD除外）
  * @param  n 按键编号
  * @param  Flag 要检查的标志：KEY_HOLD/DOWN/UP/SINGLE/DOUBLE/LONG/REPEAT
  * @retval 1=该标志已置位，0=未置位
  */
uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
	if (Key_Flag[n] & Flag)
	{
		if (Flag != KEY_HOLD)		// HOLD标志不清除（持续按下状态）
		{
			Key_Flag[n] &= ~Flag;
		}
		return 1;
	}
	return 0;
}

/**
  * @brief  清除所有按键的标志位
  * @param  无
  * @retval 无
  */
void Key_Clear(void)
{
	uint8_t i;
	for (i = 0; i < KEY_COUNT; i ++)
	{
		Key_Flag[i] = 0;
	}
}

/**
  * @brief  按键状态机（需每1ms定时调用一次）
  * @param  无
  * @retval 无
  * @note   状态机说明（S[i]状态值）：
  *         0: 空闲，等待按下
  *         1: 已按下，正在计时长按
  *         2: 已松开，等待双击判定或超时→单击
  *         3: 双击已触发（再次按下），等待松开
  *         4: 长按已触发，等待松开或连发
  */
void Key_Tick(void)
{
	static uint8_t Count, i;						// Count: 消抖计数器
	static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];
	static uint8_t S[KEY_COUNT];					// 状态机当前状态
	static uint16_t Time[KEY_COUNT];				// 计时器

	/* 计时器递减 */
	for (i = 0; i < KEY_COUNT; i ++)
	{
		if (Time[i] > 0)
		{
			Time[i] --;
		}
	}

	Count ++;
	if (Count >= 20)		// 约20ms消抖
	{
		Count = 0;

		for (i = 0; i < KEY_COUNT; i ++)
		{
			/* 更新按键状态 */
			PrevState[i] = CurrState[i];
			CurrState[i] = Key_GetState(i);

			/* 持续按下标志 */
			if (CurrState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KEY_HOLD;
			}
			else
			{
				Key_Flag[i] &= ~KEY_HOLD;
			}

			/* 按下边沿检测 */
			if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
			{
				Key_Flag[i] |= KEY_DOWN;
			}

			/* 松开边沿检测 */
			if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
			{
				Key_Flag[i] |= KEY_UP;
			}

			/* 状态机 */
			if (S[i] == 0)                              // 状态0：空闲，等待按下
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Time[i] = KEY_TIME_LONG;            // 开始长按计时
					S[i] = 1;                           // → 状态1
				}
			}
			else if (S[i] == 1)                         // 状态1：等待长按超时或松开
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					Time[i] = KEY_TIME_DOUBLE;          // 开始双击窗口计时
					S[i] = 2;                           // → 状态2（等待双击）
				}
				else if (Time[i] == 0)                  // 长按超时
				{
					Time[i] = KEY_TIME_REPEAT;          // 设置连发间隔
					Key_Flag[i] |= KEY_LONG;            // 触发长按
					S[i] = 4;                           // → 状态4（长按连发）
				}
			}
			else if (S[i] == 2)                         // 状态2：等待双击或超时
			{
				if (CurrState[i] == KEY_PRESSED)
				{
					Key_Flag[i] |= KEY_DOUBLE;          // 触发双击
					S[i] = 3;                           // → 状态3（双击已触发）
				}
				else if (Time[i] == 0)                  // 双击窗口超时
				{
					Key_Flag[i] |= KEY_SINGLE;          // 触发单击
					S[i] = 0;                           // → 状态0（回到空闲）
				}
			}
			else if (S[i] == 3)                         // 状态3：双击已触发，等待松开
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					S[i] = 0;                           // → 状态0（回到空闲）
				}
			}
			else if (S[i] == 4)                         // 状态4：长按连发模式
			{
				if (CurrState[i] == KEY_UNPRESSED)
				{
					S[i] = 0;                           // → 状态0（回到空闲）
				}
				else if (Time[i] == 0)                  // 连发间隔到
				{
					Time[i] = KEY_TIME_REPEAT;          // 重置连发计时
					Key_Flag[i] |= KEY_REPEAT;          // 触发连发
					S[i] = 4;                           // 保持状态4
				}
			}
		}
	}
}

/**
  * @brief  初始化按键模块
  * @param  无
  * @retval 无
  */
void Key_Init(void)
{
	Key_Clear();
}
