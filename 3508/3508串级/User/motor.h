#ifndef __MOTOR_H
#define __MOTOR_H

#include "pid.h"
#include "My_can.h"
#include "vofa.h"

extern PID_t PID_Angle;
extern PID_t PID_Omega;

void CAN_Motor_Call_Back(Struct_CAN_Rx_Buffer *Rx_Buffer);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif
