#include "stm32f1xx_hal.h"                  // Device header
#include "MyFLASH.h"

#define STORE_START_ADDRESS		0x0800FC00		// 存储起始地址（FLASH最后一页）
#define STORE_COUNT				512				// 存储半字数量（512×2=1024字节）

#define STORE_KEY				0xB5B5			// 存储有效标志（用于判断是否已初始化）

uint16_t Store_Data[STORE_COUNT];				// 全局存储数据数组（运行时在RAM中操作）

/**
  * @brief  初始化存储模块（从FLASH加载数据到RAM）
  * @param  无
  * @retval 1=首次初始化（FLASH数据无效，已格式化），0=正常加载
  */
uint8_t Store_Init(void)
{
	uint8_t Flag = 0;

	/* 检查FLASH存储区是否有效（通过校验关键字） */
	if (MyFLASH_ReadHalfWord(STORE_START_ADDRESS) != STORE_KEY)
	{
		/* 首次使用或数据损坏，擦除并格式化 */
		MyFLASH_ErasePage(STORE_START_ADDRESS);
		MyFLASH_ProgramHalfWord(STORE_START_ADDRESS, STORE_KEY);		// 写入校验关键字
		for (uint16_t i = 1; i < STORE_COUNT; i ++)
		{
			MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i * 2, 0x0000);	// 其余位置清零
		}
		Flag = 1;
	}

	/* 从FLASH加载全部数据到RAM数组 */
	for (uint16_t i = 0; i < STORE_COUNT; i ++)
	{
		Store_Data[i] = MyFLASH_ReadHalfWord(STORE_START_ADDRESS + i * 2);
	}

	return Flag;
}

/**
  * @brief  将RAM中的数据保存到FLASH
  * @param  无
  * @retval 无
  * @note   先擦除整页再写入（FLASH写入前必须先擦除）
  */
void Store_Save(void)
{
	MyFLASH_ErasePage(STORE_START_ADDRESS);
	for (uint16_t i = 0; i < STORE_COUNT; i ++)
	{
		MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i * 2, Store_Data[i]);
	}
}

/**
  * @brief  清除存储数据（保留校验关键字）
  * @param  无
  * @retval 无
  */
void Store_Clear(void)
{
	for (uint16_t i = 1; i < STORE_COUNT; i ++)
	{
		Store_Data[i] = 0x0000;
	}
	Store_Save();
}
