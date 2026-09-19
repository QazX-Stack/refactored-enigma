#include "stm32f4xx_hal.h"

#include "PID.h"

void PID_Init(PID_t *p)
{
	p->Target = 0;
	p->Pre_Target=0;
	p->Actual = 0;
	p->Actual1 = 0;
	p->Out = 0;
	p->Error0 = 0;
	p->Error1 = 0;
	p->POut = 0;
	p->IOut = 0;
	p->DOut = 0;
	p->FOut = 0;
}

void PID_Update(PID_t *p)
{
	p->Error1 = p->Error0;
	p->Error0 = p->Target - p->Actual;
	
	p->POut =  p->Kp * p->Error0;
	
	if (p->Ki != 0)
	{
		p->IOut +=  p->Ki * p->Error0* 0.001f;
	}
	else
	{
		p->IOut = 0;
	}
	
	if (p->IOut > p->OutMax) {p->IOut = p->OutMax;}
	if (p->IOut < p->OutMin) {p->IOut = p->OutMin;}
	
	p->DOut = - p->Kd * (p->Actual - p->Actual1);
	
	p->FOut = (p->Target - p->Pre_Target) * p->Kf;
	
	p->Out = p->POut + p->IOut + p->DOut + p->FOut;
	
	if (p->Actual > 0.5f) {p->Out += p->OutOffset;}
	if (p->Actual < -0.5f) {p->Out -= p->OutOffset;}

	if (p->Out > p->OutMax) {
		p->IOut -= (p->Out - p->OutMax);  
		p->Out = p->OutMax;
	}
	if (p->Out < p->OutMin) {
		p->IOut -= (p->Out - p->OutMin);
		p->Out = p->OutMin;
	}

	p->Actual1 = p->Actual;
	p->Pre_Target =p->Target ;
}
