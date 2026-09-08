#include "stm32f1xx_hal.h" 
#include "OLED.h"
#include "LED.h"
#include "Key.h"
#include "Motor.h"
#include "Encoder.h"
#include "MPU6050.h"
#include "tim.h"
#include "PID.h"
#include "Serial.h"
#include "DebugMode.h"
#include "Balance.h"
#include <math.h>
#include <stdio.h>

uint8_t RunFlag;

uint8_t RunFlagUpdate;

int16_t AX, AY, AZ, GX, GY, GZ;
int16_t AX_Offset, AZ_Offset, GY_Offset;
int16_t GY_Cali;

float AngleAcc;
float AngleAcc_Offset;
float AngleAcc_Cali;
float AngleAcc_Filter;
float AngleDelta;
float Angle;

float SpeedLeft, SpeedRight;
float AveSpeed, DifSpeed;

float AvePWM, DifPWM;
int16_t PWML, PWMR;

uint16_t SpeedLevel = 4;

uint8_t DebugFlag;

#define ANGLE_T				10
#define SPEED_T				50

/*角度环PID参数*/
PID_t AnglePID = {
	.Kp = -6.0,
	.Ki = 0.0,
	.Kd = -30.0,

	.OutMax = 100,
	.OutMin = -100,

	.OutOffset = 2.0,
};

/*速度环PID参数*/
PID_t SpeedPID = {
	.Kp = 2.0,
	.Ki = 0.02,
	.Kd = 0,

	.OutMax = 10,
	.OutMin = -10,
};

/*转向环PID参数*/
PID_t TurnPID = {
	.Kp =0.0,
	.Ki = 0.0,
	.Kd = 0,

	.OutMax =10.0,
	.OutMin = -10.0,
};

void Balance_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);

    HAL_TIM_Base_Start_IT(&htim1);
}

void Balance_Start(void)
{
    PID_Init(&AnglePID);
    PID_Init(&SpeedPID);
    PID_Init(&TurnPID);
    RunFlag = 1;
}

void Balance_Stop(void)
{
    RunFlag = 0;
    Motor_SetPWM(1, 0);
    Motor_SetPWM(2, 0);
}

void Balance_TimerCallback(void)
{
	static uint16_t SensorCount0, SensorCount1;
	static uint16_t RunCount0, RunCount1;
	
		Key_Tick();
	
		SensorCount0 ++;
		if (SensorCount0 >= ANGLE_T)		//每隔ANGLE_T定义的时间执行一次
		{
			SensorCount0 = 0;
			/*姿态解析，得到平衡车角度*/
			
			/*获取陀螺仪加速度计原始数据*/
			MPU6050_GetData(&hi2c2,&AX, &AY, &AZ, &GX, &GY, &GZ);
			
			/*陀螺仪数据校准*/
			GY_Cali = GY + GY_Offset;
			
			/*计算加速度计角度*/
			AngleAcc = -atan2(AX, AZ) / 3.1415926535 * 180;
			
			/*计算陀螺仪角度增量*/
			AngleDelta = GY_Cali / 32768.0 * 2000.0 * (ANGLE_T / 1000.0);
			
			/*中心角度校准*/
			AngleAcc_Cali = AngleAcc + AngleAcc_Offset;
			
			/*对加速度计角度进行一阶低通滤波，使其更平滑*/
			float Alpha0 = 0.6;
			AngleAcc_Filter = Alpha0 * AngleAcc_Filter + (1 - Alpha0) * AngleAcc_Cali;
			
			/*角度累加陀螺仪角度增量，得到新的角度值*/
			Angle += AngleDelta;
			
			/*新的角度值与加速度计角度进行互补滤波，抑制漂移*/
			float Alpha1 = fabs(DifSpeed) / 5.0 * 0.03 + 0.01;		//根据差速动态调整滤波参数
			if (Alpha1 > 0.04) {Alpha1 = 0.04;}						//参数限幅
			Angle = Alpha1 * AngleAcc_Filter + (1 - Alpha1) * Angle;//互补滤波得到角度值
			
			/*平衡车倒地后，自动停止*/
			if (Angle > 50 || Angle < -50)
			{
				if (RunFlag)
				{
					RunFlag = 0;
					RunFlagUpdate = 1;
				}
			}
		}
		
		SensorCount1 ++;
		if (SensorCount1 >= SPEED_T)		//每隔SPEED_T定义的时间执行一次
		{
			SensorCount1 = 0;
			
			/*通过编码器获取电机旋转速度*/
			SpeedLeft = Encoder_Get(1) / 408.0 / (SPEED_T / 1000.0);
			SpeedRight = Encoder_Get(2) / 408.0 / (SPEED_T / 1000.0);
			
			/*获取均速和差速*/
			AveSpeed = (SpeedLeft + SpeedRight) / 2.0;
			DifSpeed = SpeedLeft - SpeedRight;
		}
		
		/*PID控制*/
		if (RunFlag)
		{
			RunCount0 ++;
			if (RunCount0 >= ANGLE_T)		//每隔ANGLE_T定义的时间执行一次
			{
				RunCount0 = 0;
				
				/*角度环PID计算*/
				AnglePID.Actual = Angle;
				PID_Update(&AnglePID);
				AvePWM = -AnglePID.Out;		//角度环输出作用于平均PWM
				
				/*平均PWM和差分PWM，合成左轮PWM和右轮PWM*/
				PWML = AvePWM + DifPWM;
				PWMR = AvePWM - DifPWM;
				
				/*PWM限幅*/
				if (PWML > 100) {PWML = 100;} else if (PWML < - 100) {PWML = -100;}
				if (PWMR > 100) {PWMR = 100;} else if (PWMR < - 100) {PWMR = -100;}
				
				/*PWM输出至电机*/
				Motor_SetPWM(1, PWML);
				Motor_SetPWM(2, PWMR);
			}
			
			RunCount1 ++;
			if (RunCount1 >= SPEED_T)		//每隔SPEED_T定义的时间执行一次
			{
				RunCount1 = 0;
				
				/*速度环PID计算*/
				SpeedPID.Actual = AveSpeed;
				PID_Update(&SpeedPID);
				AnglePID.Target = SpeedPID.Out;	//速度环输出作用于角度环输入
				
				/*转向环PID计算*/
				TurnPID.Actual = DifSpeed;
				PID_Update(&TurnPID);
				DifPWM = TurnPID.Out;		//转向环输出作用于差分PWM
			}
		}
		else
		{
			Motor_SetPWM(1, 0);				//RunFlag为0时，电机停止
			Motor_SetPWM(2, 0);
		}
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        Balance_TimerCallback();
    }
}
