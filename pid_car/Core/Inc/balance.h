#ifndef __BALANCE_H
#define __BALANCE_H

#include "PID.h"
#include "MPU6050.h"

extern I2C_HandleTypeDef hi2c2;

extern uint8_t RunFlag;

extern uint8_t RunFlagUpdate;

extern int16_t AX, AY, AZ, GX, GY, GZ;
extern int16_t AX_Offset, AZ_Offset, GY_Offset;
extern int16_t GY_Cali;

extern float AngleAcc;
extern float AngleAcc_Offset;
extern float AngleAcc_Cali;
extern float AngleAcc_Filter;
extern float AngleDelta;
extern float Angle;

extern float SpeedLeft, SpeedRight;
extern float AveSpeed, DifSpeed;

extern float AvePWM, DifPWM;
extern int16_t PWML, PWMR;

extern uint16_t SpeedLevel ;

extern uint8_t DebugFlag;

#define ANGLE_T				10
#define SPEED_T				50

/*角度环PID参数*/
extern PID_t AnglePID;

/*速度环PID参数*/
extern PID_t SpeedPID;

/*转向环PID参数*/
extern PID_t TurnPID;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void Balance_Init(void);
void Balance_Start(void);
void Balance_Stop(void);
#endif 
