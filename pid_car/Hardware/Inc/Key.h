#ifndef __KEY_H
#define __KEY_H

#define KEY_COUNT				4		// 按键数量

/* 按键编号 */
#define KEY_1					0
#define KEY_2					1
#define KEY_3					2
#define KEY_4					3

/* 按键事件标志位 */
#define KEY_HOLD				0x01	// 持续按下（不清除）
#define KEY_DOWN				0x02	// 按下边沿
#define KEY_UP					0x04	// 松开边沿
#define KEY_SINGLE				0x08	// 单击
#define KEY_DOUBLE				0x10	// 双击
#define KEY_LONG				0x20	// 长按
#define KEY_REPEAT				0x40	// 长按连发

extern uint8_t Key_Flag[];

void Key_Init(void);							// 初始化按键
uint8_t Key_Check(uint8_t n, uint8_t Flag);		// 检查按键标志（读后清除，HOLD除外）
void Key_Clear(void);							// 清除所有按键标志
void Key_Tick(void);							// 按键状态机（需每1ms调用）

#endif
