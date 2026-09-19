#ifndef __PID_H
#define __PID_H

#define PI (3.14159265f)

typedef struct {
	float Target;
	float Pre_Target;
	float Actual;		
	float Actual1;		
	float Out;			

	float Kp;			
	float Ki;			
	float Kd;
	float Kf;

	float Error0;		
	float Error1;		

	float POut;			
	float IOut;			
	float DOut;
	float FOut;

	float OutMax;		
	float OutMin;		

	float OutOffset;	
} PID_t;

void PID_Init(PID_t *p);		
void PID_Update(PID_t *p);		



#endif

