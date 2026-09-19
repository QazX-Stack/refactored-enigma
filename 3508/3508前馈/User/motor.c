#include "stm32f4xx_hal.h"
#include "motor.h"
#include "math.h"

int16_t delta_encoder;
uint16_t Pre_Encoder = 0;
int32_t Total_Encoder,Total_Round = 0;
uint16_t Encoder_Num_Per_Round = 8192;

//PID_t PID_Angle = {
//	.Kp = 1.0,
//	.Ki = 0.0,
//	.Kd = 0.0,
//	
//	.Target = 4.0f * PI,
//	.Actual=0.0,
//	.Out=0.0, 

//	.OutMax = 25.0,
//	.OutMin = -25.0,

//	.OutOffset = 0.0,
//};

PID_t PID_Omega = {
	.Kp = 300.0,
	.Ki = 3000.0,
	.Kd = 0.0,
	.Kf=21000.0,
	
	.Target = 0.0,
	.Actual=0.0,
	.Out=0.0, 

	.OutMax = 16384.0,
	.OutMin = -16384.0,

	.OutOffset = 260.0,
};

int16_t Rx_Encoder, Rx_Omega, Rx_Torque, Rx_Temperature;
float Tx_Encoder, Tx_Omega, Tx_Torque, Tx_Temperature;
uint32_t Counter = 0;
int32_t Output;

void CAN_Motor_Call_Back(Struct_CAN_Rx_Buffer *Rx_Buffer)
{
    uint8_t *Rx_Data = Rx_Buffer->Data;
	
    switch (Rx_Buffer->Header.StdId)
    {
    case (0x201):
			{
				Pre_Encoder=Rx_Encoder;
				
        Rx_Encoder = (Rx_Data[0] << 8) | Rx_Data[1];
        Rx_Omega = (Rx_Data[2] << 8) | Rx_Data[3];
        Rx_Torque = (Rx_Data[4] << 8) | Rx_Data[5];
        Rx_Temperature = Rx_Data[6];
				
				delta_encoder=Rx_Encoder - Pre_Encoder;
			
				if (delta_encoder < -4096)
				{
					Total_Round++;
				}
				else if (delta_encoder > 4096)
				{
					Total_Round--;
				}
				Total_Encoder = Total_Round * Encoder_Num_Per_Round + Rx_Encoder;
			}
		break;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
//      PID_Angle.Actual = (float)Total_Encoder / (float)Encoder_Num_Per_Round * 2.0f * PI / 19.2032f;
//			PID_Update(&PID_Angle);
	
			Counter++;
			PID_Omega .Target   = 10 * sinf((float)Counter / 1000.0f * 3.0f);
			PID_Omega .Actual =(float)Rx_Omega * (2.0f * PI / 60.0f)/ 19.2032f;
			PID_Update(&PID_Omega);
			Output = (int32_t)PID_Omega .Out ;
			
			vofa_send_motor_data(PID_Omega.Target,PID_Omega.Actual,PID_Omega .FOut ,PID_Omega .POut );
			  
			CAN1_0x200_Tx_Data[0] = (uint8_t)(Output >> 8);
			CAN1_0x200_Tx_Data[1] = (uint8_t)Output;

			TIM_CAN_PeriodElapsedCallback();
}

