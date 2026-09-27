#include "stm32f4xx_hal.h"
#include "chassis.h"
#include "math.h"

#define CHASSIS_L    (CHASSIS_SIDE * 0.70710678f)
#define INV_SQRT2    (0.70710678f)

static float Vx_Now = 0.0f;
static float Vy_Now = 0.0f;
static float Wz_Now = 0.0f;

static float Vx_Goal = 0.0f;
static float Vy_Goal = 0.0f;
static float Wz_Goal = 0.0f;

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
