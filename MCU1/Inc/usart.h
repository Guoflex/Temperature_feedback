#ifndef __USART_H__
#define __USART_H__

#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_usart.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

#define USART1_RX_BUFFER_SIZE 64
#define SENSOR_NUM 32
#define UDP_71_ONE_SENSOR_LENGTH 28        // 单个传感器数据长度

// 定义帧头和协议相关的固定内容
#define FRAME_HEADER           0xFF, 0xFF, 0x06, 0x09 // 帧头       0-3
#define FRAME_NUMBER           0x00, 0x00             // 帧序列号   4-5
#define FRAME_LENGTH           0x00, 0x00             // 帧长度     6-7
#define DEVICE_ID              0x33, 0xCC, 0x00, 0x00 // 设备 ID    8-11
#define TARGET_ID              0x12, 0x34, 0x56, 0x78 // 目标 ID    12-15
#define RESPONSE_CODE          0x00, 0x00             // 回复       16-17
#define STATUS_CODE            0x00, 0x01             // 状态字     18-19  53 59 43 33 33 33 33 32 2d 33 32
#define SYC_STRING             0x53, 0x59, 0x43, 0x33, 0x33, 0x33, 0x32, 0x2D, 0x33, 0x32 // SYC3333-32     20-29
#define RESERVED_SPACE         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // 保留空间      30-49

/* 外部变量声明 */
extern UART_HandleTypeDef Uart1Handle;
extern uint8_t  G_USART1_RX_Buffer[USART1_RX_BUFFER_SIZE];
extern uint32_t G_USART1_RX_Count;
extern volatile uint8_t G_USART1_RX_IDLE_Flag;
extern volatile uint8_t  G_setFlag; // 状态标识
extern volatile uint8_t UART_RX_CMD_1;
extern volatile uint8_t UART_RX_CMD_3;
extern volatile uint8_t UART_RX_CMD_5;
extern volatile uint8_t UART_RX_CMD_7;
extern volatile uint8_t UART_RX_CMD_9;
extern volatile uint8_t UART_RX_CMD_11;
extern volatile uint8_t UART_RX_CMD_13;
extern volatile uint8_t UART_RX_CMD_15;
extern volatile uint8_t UART_RX_CMD_71;
extern float *c_read_value; // 传感器值
extern volatile float c_value[SENSOR_NUM]; // 传感器值
extern uint8_t Generic_Reply[50]; // 协议回复
extern uint8_t UDP_11[212];
extern uint8_t UDP_15[20];
extern uint8_t UDP_71[214];

void USART1_Init(uint32_t baudrate);
void USART1_SendByte(uint8_t byte);
void USART1_SendString(char *str);

/* 启用串口接收中断的函数声明 */
void USART1_EnableRxInterrupt(void);

/* 重置接收缓冲区的函数声明 */
void USART1_ResetRxBuffer(void);

/* 处理接收到的数据 */
void process_received_data(void);

/* 读取所有RM1002传感器数据 */
void read_all_rm1002_sensors(float *c_value);

/* 将浮点数转换为16进制格式并存入数组的指定位置 */
void process_value(float value, uint8_t *udp_array, int start_index);

/* 处理71命令 */
void process_command_71(uint16_t channel, uint16_t operation, float value, uint16_t *Cap_Frame_Num);

#endif





