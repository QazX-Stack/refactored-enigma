#ifndef __CHASSIS_H
#define __CHASSIS_H

#include "motor.h"

#define CHASSIS_SIDE    0.700f
#define WHEEL_RADIUS    0.050f

#define CHASSIS_DT      0.001f

#define CHASSIS_ACCEL   2.0f
#define CHASSIS_ACCEL_W 4.0f

#define INPUT_CH_VX          1
#define INPUT_CH_VY          0
#define INPUT_CH_WZ          3
#define INPUT_CH_SWITCH      4

#define INPUT_VX_SIGN        (+1.0f)
#define INPUT_VY_SIGN        (-1.0f)
#define INPUT_WZ_SIGN        (+1.0f)

#define INPUT_VX_MAX         0.8f
#define INPUT_VY_MAX         0.8f
#define INPUT_WZ_MAX         1.0f

#define INPUT_DEADZONE       0.03f
#define INPUT_SWITCH_ON_MIN  1500

void Chassis_Set_Velocity(float Vx, float Vy, float Wz);

void Chassis_Input_Calibrate(void);

void Chassis_Input_Update(void);

void Chassis_Update(void);

void Chassis_Get_Velocity(float *Vx, float *Vy, float *Wz);

void Chassis_Get_Command(float *Vx, float *Vy, float *Wz);

#endif
