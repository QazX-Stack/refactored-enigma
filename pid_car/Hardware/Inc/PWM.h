#ifndef __PWM_H
#define __PWM_H

void PWM_Init(void);					// 初始化PWM（TIM2_CH1/CH2）
void PWM_SetCompare1(uint16_t Compare);	// 设置通道1比较值（0~999，对应0%~100%）
void PWM_SetCompare2(uint16_t Compare);	// 设置通道2比较值（0~999，对应0%~100%）

#endif
