#include "MPU6050.h"
#include <stdint.h>

// 写寄存器（内部调用）
static uint8_t MPU6050_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t RegAddress, uint8_t Data)
{
	return HAL_I2C_Mem_Write(hi2c, MPU6050_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, &Data, 1, 10);
}

// 读寄存器
static uint8_t MPU6050_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t RegAddress)
{
	uint8_t Data = 0;
	HAL_I2C_Mem_Read(hi2c, MPU6050_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, &Data, 1, 10);
	return Data;
}


// 读多个寄存器（带超时，返回状态）
static uint8_t MPU6050_ReadRegs(I2C_HandleTypeDef *hi2c, uint8_t RegAddress, uint8_t *DataArray, uint8_t Count)
{
	return HAL_I2C_Mem_Read(hi2c, MPU6050_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, DataArray, Count, 5);
}

// MPU6050初始化
void MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
	uint8_t data;

	// 唤醒设备，选择PLL时钟源（CLKSEL=1，使用X轴陀螺仪作为PLL参考）
	data = 0x01;
	MPU6050_WriteReg(hi2c, MPU6050_PWR_MGMT_1, data);
	HAL_Delay(10);  // 等待时钟稳定

	// 唤醒所有轴，不进入待机模式
	data = 0x00;
	MPU6050_WriteReg(hi2c, MPU6050_PWR_MGMT_2, data);

	// 设置采样率分频（1kHz采样）
	data = 0x07;  // 8分频:8kHz/8=1kHz
	MPU6050_WriteReg(hi2c, MPU6050_SMPLRT_DIV, data);

	// 低通滤波配置：DLPF=0，带宽最宽（加速度~260Hz，陀螺仪~256Hz）
	data = 0x00;
	MPU6050_WriteReg(hi2c, MPU6050_CONFIG, data);

	// 陀螺仪量程:±2000°/s
	data = 0x18;
	MPU6050_WriteReg(hi2c, MPU6050_GYRO_CONFIG, data);

	// 加速度计量程:±16g
	data = 0x18;
	MPU6050_WriteReg(hi2c, MPU6050_ACCEL_CONFIG, data);
}

// 读取ID
uint8_t MPU6050_GetID(I2C_HandleTypeDef *hi2c)
{
	return MPU6050_ReadReg(hi2c, MPU6050_WHO_AM_I);
}

// 读取数据（连续14字节）
void MPU6050_GetData(I2C_HandleTypeDef *hi2c, int16_t *AccX, int16_t *AccY, int16_t *AccZ,
					 int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t Data[14];

	if (MPU6050_ReadRegs(hi2c, MPU6050_ACCEL_XOUT_H, Data, 14) == HAL_OK)
	{
		*AccX = (Data[0] << 8) | Data[1];
		*AccY = (Data[2] << 8) | Data[3];
		*AccZ = (Data[4] << 8) | Data[5];

		*GyroX = (Data[8] << 8) | Data[9];
		*GyroY = (Data[10] << 8) | Data[11];
		*GyroZ = (Data[12] << 8) | Data[13];
	}
}

// 快速读取（跳过温度6字节，减少I2C通信量）
uint8_t MPU6050_GetData_Fast(I2C_HandleTypeDef *hi2c, int16_t *AccX, int16_t *AccY, int16_t *AccZ,
							 int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t Data[12];  // 仅读取12字节:ACC(6) + GYRO(6)

	// 从0x3B连续读取12字节（加速度6字节+跳过温度2字节+陀螺仪6字节）
	if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDRESS, MPU6050_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, Data, 12, 5) != HAL_OK)
	{
		return 0;  // 读取失败
	}

	*AccX = (Data[0] << 8) | Data[1];
	*AccY = (Data[2] << 8) | Data[3];
	*AccZ = (Data[4] << 8) | Data[5];

	// 跳过温度寄存器(0x41,0x42)
	*GyroX = (Data[6] << 8) | Data[7];
	*GyroY = (Data[8] << 8) | Data[9];
	*GyroZ = (Data[10] << 8) | Data[11];

	return 1;  // 读取成功
}
