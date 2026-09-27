#ifndef __MOTOR_H
#define __MOTOR_H

#include "pid.h"
#include "My_can.h"
#include "vofa.h"

#define MOTOR_NUM           4
#define ENCODER_PER_ROUND   8192
#define GEAR_RATIO          19.2032f

enum
{
    MOTOR_FL = 0,
    MOTOR_FR = 1,
    MOTOR_RR = 2,
    MOTOR_RL = 3,
};

typedef struct
{
    int16_t  Encoder;
    int16_t  Omega;
    int16_t  Torque;
    uint8_t  Temperature;

    uint16_t Pre_Encoder;
    int16_t  Delta_Encoder;
    int32_t  Total_Round;
    int32_t  Total_Encoder;

    float    Target;

    PID_t    PID_Omega;

    int32_t  Output;
} Motor_t;

extern Motor_t Motor[MOTOR_NUM];

extern uint8_t Motor_Vofa_Select;

void Motor_Init(void);

void Motor_Set_Target(float FL, float FR, float RR, float RL);

void Motor_Omega_Loop(Motor_t *m);

void Motor_Pack_0x200(void);

void CAN_Motor_Call_Back(Struct_CAN_Rx_Buffer *Rx_Buffer);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif
