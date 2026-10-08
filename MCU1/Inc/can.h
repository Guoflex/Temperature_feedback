#ifndef __CAN_H
#define __CAN_H

#include "main.h"
#include "stm32g0xx_hal_fdcan.h"

extern FDCAN_HandleTypeDef hfdcan1;     // CAN句柄
extern FDCAN_TxHeaderTypeDef TxHeader1; // 发送消息头
extern FDCAN_RxHeaderTypeDef RxHeader1; // 接收消息头
extern uint8_t can1_txbuf[64]; // 发送缓冲区
extern uint8_t can1_rxbuf[64]; // 接收缓冲区

void FDCAN1_Init(void); // CAN初始化
void FDCAN1_Config(void); // CAN配置
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs); // CAN接收回调函数
void FDCAN1_SendData(uint32_t id, uint8_t *data, uint8_t len); // CAN发送数据
uint8_t fdcan1_receive_msg(uint8_t *buf); // CAN接收数据

#endif /* __CAN_H */
