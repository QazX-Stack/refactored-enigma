#ifndef __MYFLASH_H
#define __MYFLASH_H

/* ====================== FLASH读取 ====================== */
uint32_t MyFLASH_ReadWord(uint32_t Address);		// 读取32位（地址需4字节对齐）
uint16_t MyFLASH_ReadHalfWord(uint32_t Address);	// 读取16位（地址需2字节对齐）
uint8_t MyFLASH_ReadByte(uint32_t Address);			// 读取8位

/* ====================== FLASH擦除 ====================== */
void MyFLASH_EraseAllPages(void);					// 擦除全部64页（64KB）
void MyFLASH_ErasePage(uint32_t PageAddress);		// 擦除指定页

/* ====================== FLASH编程 ====================== */
void MyFLASH_ProgramWord(uint32_t Address, uint32_t Data);				// 编程32位（实际仅写入低16位）
void MyFLASH_ProgramHalfWord(uint32_t Address, uint16_t Data);		// 编程16位

#endif
