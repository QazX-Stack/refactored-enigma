#include "stm32f4xx_hal.h"
#include "chassis.h"
#include "sbus.h"
#include "math.h"

#define CHASSIS_L    (CHASSIS_SIDE * 0.70710678f)
#define INV_SQRT2    (0.70710678f)

static float Vx_Now = 0.0f;
static float Vy_Now = 0.0f;
static float Wz_Now = 0.0f;

static float Vx_Goal = 0.0f;
static float Vy_Goal = 0.0f;
static float Wz_Goal = 0.0f;

static uint8_t Input_Was_Connected = 0;

static uint16_t Input_Center_Vx = SBUS_VALUE_MID;
static uint16_t Input_Center_Vy = SBUS_VALUE_MID;
static uint16_t Input_Center_Wz = SBUS_VALUE_MID;
static uint8_t  Input_Center_Ready = 0;

static float Input_Normalize(uint16_t Raw, uint16_t Center)
{
    float v;

    if (Raw >= Center)
    {
        v = (float)(Raw - Center) / (float)(SBUS_VALUE_MAX - Center);
    }
    else
    {
        v = (float)(Raw - Center) / (float)(Center - SBUS_VALUE_MIN);
    }

    if (v > 1.0f)
    {
        v = 1.0f;
    }

    if (v < -1.0f)
    {
        v = -1.0f;
    }

    if ((v < INPUT_DEADZONE) && (v > -INPUT_DEADZONE))
    {
        v = 0.0f;
    }

    return v;
}

void Chassis_Input_Calibrate(void)
{
    if (!Sbus_Is_Connected())
    {
        return;
    }

    Input_Center_Vx = Sbus_Channel[INPUT_CH_VX];
    Input_Center_Vy = Sbus_Channel[INPUT_CH_VY];
    Input_Center_Wz = Sbus_Channel[INPUT_CH_WZ];

    Input_Center_Ready = 1;
}

static float Chassis_Ramp(float Now, float Goal, float Step)
{
    float Delta = Goal - Now;

    if (Delta > Step)
    {
        return Now + Step;
    }

    if (Delta < -Step)
    {
        return Now - Step;
    }

    return Goal;
}

void Chassis_Set_Velocity(float Vx, float Vy, float Wz)
{
    Vx_Goal = Vx;
    Vy_Goal = Vy;
    Wz_Goal = Wz;
}

void Chassis_Input_Update(void)
{
    float vx;
    float vy;
    float wz;

    if (!Sbus_Is_Connected())
    {
        if (Input_Was_Connected)
        {
            Chassis_Set_Velocity(0.0f, 0.0f, 0.0f);
        }

        Input_Was_Connected = 0;
        return;
    }

    Input_Was_Connected = 1;

    if (!Input_Center_Ready)
    {
        Chassis_Input_Calibrate();
    }

    if (Sbus_Channel[INPUT_CH_SWITCH] < INPUT_SWITCH_ON_MIN)
    {
        Chassis_Set_Velocity(0.0f, 0.0f, 0.0f);
        return;
    }

    vx = Input_Normalize(Sbus_Channel[INPUT_CH_VX], Input_Center_Vx) * INPUT_VX_SIGN * INPUT_VX_MAX;
    vy = Input_Normalize(Sbus_Channel[INPUT_CH_VY], Input_Center_Vy) * INPUT_VY_SIGN * INPUT_VY_MAX;
    wz = Input_Normalize(Sbus_Channel[INPUT_CH_WZ], Input_Center_Wz) * INPUT_WZ_SIGN * INPUT_WZ_MAX;

    Chassis_Set_Velocity(vx, vy, wz);
}

void Chassis_Update(void)
{
    float FL;
    float FR;
    float RR;
    float RL;

    Vx_Now = Chassis_Ramp(Vx_Now, Vx_Goal, CHASSIS_ACCEL * CHASSIS_DT);
    Vy_Now = Chassis_Ramp(Vy_Now, Vy_Goal, CHASSIS_ACCEL * CHASSIS_DT);
    Wz_Now = Chassis_Ramp(Wz_Now, Wz_Goal, CHASSIS_ACCEL_W * CHASSIS_DT);

    FL = ((Vx_Now - Vy_Now) * INV_SQRT2 - CHASSIS_L * Wz_Now) / WHEEL_RADIUS;
    FR = ((-Vx_Now - Vy_Now) * INV_SQRT2 - CHASSIS_L * Wz_Now) / WHEEL_RADIUS;
    RR = ((-Vx_Now + Vy_Now) * INV_SQRT2 - CHASSIS_L * Wz_Now) / WHEEL_RADIUS;
    RL = ((Vx_Now + Vy_Now) * INV_SQRT2 - CHASSIS_L * Wz_Now) / WHEEL_RADIUS;

    Motor_Set_Target(FL, FR, RR, RL);
}

void Chassis_Get_Velocity(float *Vx, float *Vy, float *Wz)
{
    float FL;
    float FR;
    float RR;
    float RL;

    FL = Motor[MOTOR_FL].PID_Omega.Actual;
    FR = Motor[MOTOR_FR].PID_Omega.Actual;
    RR = Motor[MOTOR_RR].PID_Omega.Actual;
    RL = Motor[MOTOR_RL].PID_Omega.Actual;

    *Vx = WHEEL_RADIUS * ( FL - FR - RR + RL) * 0.5f * INV_SQRT2;
    *Vy = WHEEL_RADIUS * (-FL - FR + RR + RL) * 0.5f * INV_SQRT2;
    *Wz = -WHEEL_RADIUS * (FL + FR + RR + RL) * 0.25f / CHASSIS_L;
}

void Chassis_Get_Command(float *Vx, float *Vy, float *Wz)
{
    *Vx = Vx_Now;
    *Vy = Vy_Now;
    *Wz = Wz_Now;
}
