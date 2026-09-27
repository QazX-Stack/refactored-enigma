#ifndef __CHASSIS_H
#define __CHASSIS_H

#include "motor.h"

#define CHASSIS_SIDE    0.700f
#define WHEEL_RADIUS    0.050f

#define CHASSIS_DT      0.001f

#define CHASSIS_ACCEL   2.0f
#define CHASSIS_ACCEL_W 4.0f

void Chassis_Set_Velocity(float Vx, float Vy, float Wz);

void Chassis_Update(void);

void Chassis_Get_Velocity(float *Vx, float *Vy, float *Wz);

void Chassis_Get_Command(float *Vx, float *Vy, float *Wz);

#endif
