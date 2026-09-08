#ifndef __DEBUG_MODE_H
#define __DEBUG_MODE_H

void SaveParam(void);			// 保存校准参数到FLASH
void LoadParam(void);			// 从FLASH加载校准参数
void DebugMode(void);			// 调试模式主循环（OLED菜单）
void accelerate(void);			// 加速（速度级别+1）
void decelerate(void);			// 减速（速度级别-1）

#endif
