#ifndef __STORE_H
#define __STORE_H

extern uint16_t Store_Data[];			// 全局存储数据数组（运行时在RAM中操作）

uint8_t Store_Init(void);				// 初始化：从FLASH加载数据，首次使用则格式化（返回0=正常, 1=首次初始化）
void Store_Save(void);					// 保存：将RAM数据写入FLASH
void Store_Clear(void);					// 清除：清零所有存储数据

#endif
