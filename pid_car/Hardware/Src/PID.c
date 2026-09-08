#include "stm32f1xx_hal.h"                  // Device header
#include "PID.h"

void PID_Init(PID_t *p)
{
	p->Target = 0;
	p->Actual = 0;
	p->Actual1 = 0;
	p->Out = 0;
	p->Error0 = 0;
	p->Error1 = 0;
	p->POut = 0;
	p->IOut = 0;
	p->DOut = 0;
}

void PID_Update(PID_t *p)
{
	/*误差传递*/
	p->Error1 = p->Error0;
	p->Error0 = p->Target - p->Actual;
	
	/*P项*/
	p->POut =  p->Kp * p->Error0;
	
	/*I项*/
	if (p->Ki != 0)
	{
		p->IOut +=  p->Ki * p->Error0;
	}
	else
	{
		p->IOut = 0;
	}
	
	/*积分限幅*/
	if (p->IOut > p->OutMax) {p->IOut = p->OutMax;}
	if (p->IOut < p->OutMin) {p->IOut = p->OutMin;}
	
	/*D项 + 微分先行*/
	p->DOut = - p->Kd * (p->Actual - p->Actual1);
	
	/*输出值*/
	p->Out = p->POut + p->IOut + p->DOut;
	
	/*输出偏移*/
	if (p->Out > 0) {p->Out += p->OutOffset;}
	if (p->Out < 0) {p->Out -= p->OutOffset;}

	/*输出限幅 + 抗积分饱和 (back-calculation) */
	if (p->Out > p->OutMax) {
		p->IOut -= (p->Out - p->OutMax);  /* 超出的部分从积分中扣除 */
		p->Out = p->OutMax;
	}
	if (p->Out < p->OutMin) {
		p->IOut -= (p->Out - p->OutMin);  /* 不足的部分从积分中补回 */
		p->Out = p->OutMin;
	}

	/*实际值传递*/
	p->Actual1 = p->Actual;
}
