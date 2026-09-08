#include "stm32f1xx_hal.h"                  // Device header

/**
  * @brief  从FLASH读取一个字（32位）
  * @param  Address 读取地址（必须4字节对齐）
  * @retval 读取到的32位数据
  */
uint32_t MyFLASH_ReadWord(uint32_t Address)
{
	return *((__IO uint32_t *)(Address));
}

/**
  * @brief  从FLASH读取一个半字（16位）
  * @param  Address 读取地址（必须2字节对齐）
  * @retval 读取到的16位数据
  */
uint16_t MyFLASH_ReadHalfWord(uint32_t Address)
{
	return *((__IO uint16_t *)(Address));
}

/**
  * @brief  从FLASH读取一个字节（8位）
  * @param  Address 读取地址
  * @retval 读取到的8位数据
  */
uint8_t MyFLASH_ReadByte(uint32_t Address)
{
	return *((__IO uint8_t *)(Address));
}

/**
  * @brief  擦除全部FLASH页面（64页×1KB=64KB，STM32F103C8T6全部FLASH）
  * @param  无
  * @retval 无
  * @note   擦除后所有数据变为0xFFFF
  */
void MyFLASH_EraseAllPages(void)
{
	FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError = 0;

  EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = 0x08000000;  // 从第0页开始
  EraseInitStruct.NbPages = 64;               // STM32F103C8T6共64页
	HAL_FLASH_Unlock();
  HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
	HAL_FLASH_Lock();
}

/**
  * @brief  擦除指定FLASH页面
  * @param  PageAddress 页起始地址
  * @retval 无
  */
void MyFLASH_ErasePage(uint32_t PageAddress)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = PageAddress;
    EraseInitStruct.NbPages = 1;                // 仅擦除1页

    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    HAL_FLASH_Lock();
}

/**
  * @brief  编程一个字（32位）到FLASH
  * @param  Address 写入地址（必须2字节对齐）
  * @param  Data 32位数据
  * @retval 无
  * @note   实际使用半字编程（STM32F1系列FLASH编程单位为16位），
  *         仅写入低16位，高16位将被丢弃。
  *         如需完整写入32位，请调用两次ProgramHalfWord。
  */
void MyFLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, Data);
	HAL_FLASH_Lock();
}

/**
  * @brief  编程一个半字（16位）到FLASH
  * @param  Address 写入地址（必须2字节对齐）
  * @param  Data 16位数据
  * @retval 无
  */
void MyFLASH_ProgramHalfWord(uint32_t Address, uint16_t Data)
{
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, Data);
	HAL_FLASH_Lock();
}
