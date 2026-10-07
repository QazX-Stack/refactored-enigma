#ifndef __My_can_H
#define __My_can_H

#include "main.h"
#include "stm32f4xx_hal.h"

#define CAN_FILTER(x) ((x)<<3)

#define CAN_FIFO_0 (0<<2)
#define CAN_FIFO_1 (1<<2)

#define CAN_STDID (0<<1)
#define CAN_EXDID (1<<1)

#define CAN_DATA_TYPE (0<<0)
#define CAN_REMOTE_TPYE (1<<0)

typedef struct {
		CAN_RxHeaderTypeDef Header; 
    uint8_t Data[8];
} Struct_CAN_Rx_Buffer;

typedef void (*CAN_Call_Back)(Struct_CAN_Rx_Buffer *);

typedef struct {
    CAN_HandleTypeDef *hcan;
    Struct_CAN_Rx_Buffer Rx_Buffer;
 void (*Callback_Function)(Struct_CAN_Rx_Buffer *);
	uint8_t Tx_State;
} Struct_CAN_Manage_Object;

extern CAN_HandleTypeDef hcan1;

extern Struct_CAN_Manage_Object CAN1_Manage_Object;

extern uint8_t CAN1_0x1ff_Tx_Data[];
extern uint8_t CAN1_0x200_Tx_Data[];
extern uint8_t CAN1_0x2ff_Tx_Data[];


extern uint8_t CAN1_0x220_Tx_Data[];

void Can_Init(CAN_HandleTypeDef *hcan, CAN_Call_Back Callback_Function);

void CAN_Filter_Mask_Config(CAN_HandleTypeDef *hcan, uint8_t Object_Para, uint32_t ID, uint32_t Mask_ID);

uint8_t CAN_Send_Data(CAN_HandleTypeDef *hcan, uint16_t ID, uint8_t *Data, uint16_t Length);

void TIM_CAN_PeriodElapsedCallback(void);


#endif
