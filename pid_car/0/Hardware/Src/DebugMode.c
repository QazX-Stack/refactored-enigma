#include "stm32f1xx_hal.h"                     // Device header
#include "OLED.h"
#include "LED.h"
#include "Key.h"
#include "Balance.h"
#include "Motor.h"
#include "Encoder.h"
#include "MPU6050.h"
#include "tim.h"
#include "PID.h"
#include "Store.h"
#include <math.h>
#include <stdio.h>
#include "BlueSerial.h"

extern int16_t AX, AY, AZ, GX, GY, GZ;
extern int16_t GY_Offset;

extern float AngleAcc;
extern float AngleAcc_Offset;

extern uint8_t DebugFlag;
extern uint16_t SpeedLevel; 

/**
  * @brief  保存校准参数到FLASH（掉电不丢失）
  * @param  无
  * @retval 无
  */
void SaveParam(void)
{
	/* 将运行参数写入Store_Data数组，然后保存到FLASH */
	Store_Data[1] = SpeedLevel;
	Store_Data[2] = GY_Offset;
	Store_Data[3] = *((uint16_t *)&AngleAcc_Offset);;
	Store_Data[4] = *((uint16_t *)&AngleAcc_Offset + 1);;
	Store_Save();
}

/**
  * @brief  从FLASH加载校准参数到运行变量
  * @param  无
  * @retval 无
  */
void LoadParam(void)
{
	/* 从Store_Data数组恢复参数（Store_Init已从FLASH读取） */
	uint32_t Temp;
	SpeedLevel = Store_Data[1];
	GY_Offset = Store_Data[2];
	Temp = (Store_Data[4] << 16) | Store_Data[3];
	AngleAcc_Offset = *(float *)&Temp;
}


/**
  * @brief  传感器校准：测量陀螺仪零漂和加速度计角度偏移
  * @note   需保持平衡车绝对竖直且静止，按K4确认、长按K4退出
  * @param  无
  * @retval 无
  */
void SensorCalibration(void)
{
	float GY_Array[100] = {0};
	float AngleAcc_Array[100] = {0};
	uint8_t p = 0;
	float GY_Sum, GY_Ave;
	float AngleAcc_Sum, AngleAcc_Ave;
	
	OLED_Clear();
	OLED_ShowString(1, 1, "GY:+00000.00");
	OLED_ShowString(2, 1,  "Angle:+000.00");
	OLED_ShowString(3, 1, "  Keep Vertical ");
	OLED_ShowString(4, 1,  "  Press K4 OK   ");
	
	/*测试前保持平衡车绝对竖直且静止，否则校准值可能有误*/
	/*校准项目，第一个是静止时陀螺仪的漂移，第二个是竖直时中心角度的偏差*/
	
	while (1)
	{
		if (Key_Check(KEY_4, KEY_SINGLE)) {Key_Clear(); break;}		//K4键确认
		if (Key_Check(KEY_4, KEY_LONG)) {Key_Clear(); return;}		//K4键长按退出
		
		MPU6050_GetData(&hi2c2,&AX, &AY, &AZ, &GX, &GY, &GZ);		//读取MPU6050原始数据
		AngleAcc = -atan2(AX, AZ) / 3.1415926535 * 180;		// 根据加速度计计算倾斜角度
		
		/*取100次测量结果，计算平均值，避免噪声干扰*/
		GY_Array[p] = GY;
		AngleAcc_Array[p] = AngleAcc;
		p ++;
		p %= 100;
		
		GY_Sum = 0;
		AngleAcc_Sum = 0;
		for (uint8_t i = 0; i < 100; i ++)
		{
			GY_Sum += GY_Array[i];
			AngleAcc_Sum += AngleAcc_Array[i];
		}
		GY_Ave = GY_Sum / 100.0;
		AngleAcc_Ave = AngleAcc_Sum / 100.0;
		
		OLED_ShowFloat(1, 1, GY_Ave,3,2);
		OLED_ShowFloat(2, 1, AngleAcc_Ave,3,2);
		 OLED_ShowString(3, 1, "  Keep Vertical ");  
    OLED_ShowString(4, 1, "  Press K4 OK   ");  
    HAL_Delay(50);  // 
	}
	/*确认后，退出循环，校准完成*/
	OLED_Clear ();
	OLED_ShowString(2, 1, "Calibration OK");

	
	HAL_Delay(300);
	
	OLED_Clear();
	OLED_ShowString(2, 1, "Param Saved");
	HAL_Delay(300);
	
	GY_Offset = -GY_Ave;				//绝对静止时，负的陀螺仪漂移为校准值
	AngleAcc_Offset = -AngleAcc_Ave;	//绝对竖直时，负的中心角度为校准值
	
	SaveParam();		// 保存参数至FLASH，掉电不丢失
}

/**
  * @brief  调试模式主循环（OLED交互菜单）
  * @param  无
  * @retval 无
  * @note   DebugFlag用于控制进出调试模式
  */
void DebugMode(void)
{
	while (1)
	{ 
		OLED_ShowString(1, 1, "Debug Mode");
		OLED_ShowString(2, 1, "K2 Calibrate");
		OLED_ShowString(3, 1, "long K4:Exit");  
		
		if (Key_Check(KEY_2, KEY_SINGLE)) {Key_Clear();SensorCalibration();}	//K2键传感器校准
		if (Key_Check(KEY_4, KEY_LONG)) {DebugFlag = 0; OLED_Clear(); Key_Clear(); break;}	//K4键长按退出
	}
}

/**
  * @brief  加速 — 速度级别+1（最大6级）
  * @param  无
  * @retval 无
  */
void accelerate(void)
{
	if (SpeedLevel < 6)
			{
				SpeedLevel ++;
				SaveParam();
			}
			BlueSerial_Printf("SpeedLevel=%d\r\n", SpeedLevel);
}

/**
  * @brief  减速 — 速度级别-1（最小1级）
  * @param  无
  * @retval 无
  */
void decelerate(void)
{
	if (SpeedLevel > 1) 
			{
				SpeedLevel --;
				SaveParam();
			}
			BlueSerial_Printf("SpeedLevel=%d\r\n", SpeedLevel);
}

