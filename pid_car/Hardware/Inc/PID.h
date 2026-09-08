#ifndef __PID_H
#define __PID_H

/**
  * @brief  PID控制器结构体
  * @note   支持：位置式PID、积分限幅、微分先行、抗积分饱和、输出偏移
  */
typedef struct {
	float Target;		// 目标值
	float Actual;		// 当前实际值
	float Actual1;		// 上一次实际值（用于微分先行）
	float Out;			// 计算输出值

	float Kp;			// 比例系数
	float Ki;			// 积分系数
	float Kd;			// 微分系数

	float Error0;		// 当前误差
	float Error1;		// 上一次误差

	float POut;			// 比例项输出
	float IOut;			// 积分项输出
	float DOut;			// 微分项输出

	float OutMax;		// 输出上限
	float OutMin;		// 输出下限

	float OutOffset;	// 输出偏移量（用于产生死区效果）
} PID_t;

void PID_Init(PID_t *p);		// PID结构体初始化（清零所有变量）
void PID_Update(PID_t *p);		// PID计算更新（每次控制周期调用一次）

#endif
