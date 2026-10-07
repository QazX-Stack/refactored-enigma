#include "stm32f4xx_hal.h"
#include "My_can.h"

Struct_CAN_Manage_Object CAN1_Manage_Object = {0};
Struct_CAN_Manage_Object CAN2_Manage_Object = {0};

uint8_t CAN1_0x1ff_Tx_Data[8];
uint8_t CAN1_0x200_Tx_Data[8];
uint8_t CAN1_0x2ff_Tx_Data[8];

uint8_t CAN2_0x1ff_Tx_Data[8];
uint8_t CAN2_0x200_Tx_Data[8];
uint8_t CAN2_0x2ff_Tx_Data[8];

uint8_t CAN1_0x220_Tx_Data[8];


void Can_Init(CAN_HandleTypeDef *hcan,CAN_Call_Back Callback_Function)
{
    HAL_CAN_Start(hcan);
    __HAL_CAN_ENABLE_IT(hcan,CAN_IT_RX_FIFO0_MSG_PENDING);
    __HAL_CAN_ENABLE_IT(hcan,CAN_IT_RX_FIFO1_MSG_PENDING);
		CAN1_Manage_Object.Callback_Function = Callback_Function;

}

void CAN_Filter_Mask_Config(CAN_HandleTypeDef *hcan,uint8_t Object_Para,uint32_t ID,uint32_t Mask_ID)
{
    CAN_FilterTypeDef can_filter_init_structure;

    if(Object_Para & 0x01)
    {
        return;
    }

    if((Object_Para & 0x02)>>1)
    {
        return;
    }

    can_filter_init_structure.FilterIdHigh=(ID & 0x7FF)<<5;
    can_filter_init_structure.FilterIdLow=0x0000;
    can_filter_init_structure.FilterMaskIdHigh=(Mask_ID & 0x7FF)<<5;
    can_filter_init_structure.FilterMaskIdLow=0x0000;

    can_filter_init_structure.FilterBank=(Object_Para >> 3) & 0x1F;
    can_filter_init_structure.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter_init_structure.FilterScale=CAN_FILTERSCALE_32BIT;
    can_filter_init_structure.FilterActivation=ENABLE;

    can_filter_init_structure.SlaveStartFilterBank=14;

    can_filter_init_structure.FilterFIFOAssignment = (Object_Para >> 2) & 0x01;

    HAL_CAN_ConfigFilter(hcan,&can_filter_init_structure);

}

uint8_t CAN_Send_Data(CAN_HandleTypeDef *hcan,uint16_t ID,uint8_t *Data, uint16_t Length)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t used_mailbox;
    assert_param (hcan != NULL);

    tx_header.StdId = ID;
    tx_header.ExtId = 0;
    tx_header.IDE = 0;
    tx_header.RTR = 0;
    tx_header.DLC = Length;

    return (HAL_CAN_AddTxMessage(hcan,&tx_header,Data,&used_mailbox));
}

void TIM_CAN_PeriodElapsedCallback(void)
{
    static int mod10 = 0;

    mod10++;

    CAN_Send_Data(&hcan1, 0x1ff, CAN1_0x1ff_Tx_Data, 8);
    CAN_Send_Data(&hcan1, 0x200, CAN1_0x200_Tx_Data, 8);
    CAN_Send_Data(&hcan1, 0x2ff, CAN1_0x2ff_Tx_Data, 8);

    if (mod10 == 10 - 1)
    {
        mod10 = 0;
        CAN_Send_Data(&hcan1, 0x220, CAN1_0x220_Tx_Data, 8);
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    static Struct_CAN_Rx_Buffer can_rx_buffer;

    if (hcan->Instance == CAN1)
    {
        HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO0, &can_rx_buffer.Header, can_rx_buffer.Data);
        CAN1_Manage_Object.Callback_Function(&can_rx_buffer);
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  
    static Struct_CAN_Rx_Buffer can_rx_buffer;

 
    if (hcan->Instance == CAN1)
    {
        HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO1, &can_rx_buffer.Header, can_rx_buffer.Data);
        CAN1_Manage_Object.Callback_Function(&can_rx_buffer);
    }
}
