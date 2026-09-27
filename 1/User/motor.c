#include "stm32f4xx_hal.h"
#include "motor.h"
#include "chassis.h"
#include "math.h"

Motor_t Motor[MOTOR_NUM];

uint8_t Motor_Vofa_Select = MOTOR_FL;

static const PID_t PID_Omega_Param =
{
    .Kp = 2500.0f,
    .Ki = 4000.0f,
    .Kd = 0.0f,
    .Kf = 21000.0f,

    .OutMax = 16384.0f,
    .OutMin = -16384.0f,

    .OutOffset = 0.0f,
};

void Motor_Init(void)
{
    uint8_t i;

    for (i = 0; i < MOTOR_NUM; i++)
    {
        Motor[i].PID_Omega = PID_Omega_Param;

        PID_Init(&Motor[i].PID_Omega);

        Motor[i].Target = 0.0f;
        Motor[i].Output = 0;
    }
}

void Motor_Set_Target(float FL, float FR, float RR, float RL)
{
    Motor[MOTOR_FL].Target = FL;
    Motor[MOTOR_FR].Target = FR;
    Motor[MOTOR_RR].Target = RR;
    Motor[MOTOR_RL].Target = RL;
}

void Motor_Omega_Loop(Motor_t *m)
{
    m->PID_Omega.Target = m->Target;
    m->PID_Omega.Actual = (float)m->Omega * (2.0f * PI / 60.0f) / GEAR_RATIO;
    PID_Update(&m->PID_Omega);
    m->Output = (int32_t)m->PID_Omega.Out;
}

void Motor_Pack_0x200(void)
{
    uint8_t i;

    for (i = 0; i < MOTOR_NUM; i++)
    {
        CAN1_0x200_Tx_Data[2 * i]     = (uint8_t)(Motor[i].Output >> 8);
        CAN1_0x200_Tx_Data[2 * i + 1] = (uint8_t)(Motor[i].Output);
    }
}

void CAN_Motor_Call_Back(Struct_CAN_Rx_Buffer *Rx_Buffer)
{
    uint8_t  *Rx_Data = Rx_Buffer->Data;
    uint32_t  StdId   = Rx_Buffer->Header.StdId;
    Motor_t  *m;

    if (StdId < 0x201 || StdId > 0x204)
    {
        return;
    }

    m = &Motor[StdId - 0x201];

    m->Pre_Encoder = m->Encoder;

    m->Encoder     = (int16_t)((Rx_Data[0] << 8) | Rx_Data[1]);
    m->Omega       = (int16_t)((Rx_Data[2] << 8) | Rx_Data[3]);
    m->Torque      = (int16_t)((Rx_Data[4] << 8) | Rx_Data[5]);
    m->Temperature = Rx_Data[6];

    m->Delta_Encoder = (int16_t)(m->Encoder - m->Pre_Encoder);

    if (m->Delta_Encoder < -4096)
    {
        m->Total_Round++;
    }
    else if (m->Delta_Encoder > 4096)
    {
        m->Total_Round--;
    }

    m->Total_Encoder = m->Total_Round * ENCODER_PER_ROUND + m->Encoder;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    Motor_t *v;
    float    Cmd_Vx;
    float    Cmd_Vy;
    float    Cmd_Wz;
    float    Act_Vx;
    float    Act_Vy;
    float    Act_Wz;

    if (htim->Instance != TIM6)
    {
        return;
    }

    Chassis_Update();

    Motor_Omega_Loop(&Motor[MOTOR_FL]);
    Motor_Omega_Loop(&Motor[MOTOR_FR]);
    Motor_Omega_Loop(&Motor[MOTOR_RR]);
    Motor_Omega_Loop(&Motor[MOTOR_RL]);

    Motor_Pack_0x200();

    if (Motor_Vofa_Select == MOTOR_NUM)
    {
        Chassis_Get_Command(&Cmd_Vx, &Cmd_Vy, &Cmd_Wz);
        Chassis_Get_Velocity(&Act_Vx, &Act_Vy, &Act_Wz);

        vofa_send_motor_data(Cmd_Vx, Cmd_Vy, Cmd_Wz, 0.0f);
    }
    else
    {
        v = &Motor[Motor_Vofa_Select];
        vofa_send_motor_data(v->PID_Omega.Target, v->PID_Omega.Actual, 0.0f, 0.0f);
    }

    TIM_CAN_PeriodElapsedCallback();
}
