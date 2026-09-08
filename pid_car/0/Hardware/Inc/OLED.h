#ifndef __OLED_H
#define __OLED_H

/* ====================== 基础显示函数 ====================== */
void OLED_Init(void);			// OLED初始化（上电后需先调用）
void OLED_Clear(void);			// 清屏（所有像素熄灭）

/* ====================== 字符/字符串显示 ====================== */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);					// 显示单个字符（8×16）
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);				// 显示字符串

/* ====================== 数字显示（支持多种进制） ====================== */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);		// 显示无符号十进制数
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);	// 显示有符号十进制数（带+/-号）
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);	// 显示十六进制数
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);	// 显示二进制数
void OLED_ShowFloat(uint8_t Line, uint8_t Column, double Number, uint8_t intLen, uint8_t fracLen);	// 显示浮点数

#endif
