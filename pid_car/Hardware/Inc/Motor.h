#ifndef __MOTOR_H
#define __MOTOR_H

void Motor_Init(void);								// 初始化电机GPIO
void Motor_SetPWM(uint8_t n, int16_t Duty);			// 设置电机PWM（n=1/2, Duty=-99~99）

#endif
