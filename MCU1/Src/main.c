#include "main.h"
#include "flash.h"

float par1_3_cap[4] = {13.507404, 11.861684 ,12.116448, 11.813702};
float par1_2_cap[4] = {13.852436, 12.487850 ,12.657818, 12.339934};
float par1_1_cap[4] = {13.360336, 11.787779 ,11.929258, 11.906664};
float par1_0_cap[4] = {14.072913, 12.307257 ,12.473142, 12.404386};
float par2_3_cap[4] = {13.499845, 11.769927 ,11.938437, 11.821710};
float par2_2_cap[4] = {13.714656, 12.041621 ,12.246987, 12.053676};
float par2_1_cap[4] = {13.710574, 12.174135 ,12.344558, 12.187529};
float par2_0_cap[4] = {13.966125, 12.283472 ,12.589819, 12.313161};

int main(void)
{
    HAL_Init();
    System_Clock_Config_HSE_64Mhz();

    LED_GPIO_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();

    USART1_Init(2000000);
    TIM1_Init();
    FDCAN1_Init();      // 先初始化硬件
    FDCAN1_Config();    // 再配置过滤器

    RM1002_Init();
    printf("System boot\r\n");
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);
    uint16_t Cap_Frame_Num = 0; // 帧序列号

    while (1)
    {
#if 1
        if (G_USART1_RX_IDLE_Flag == 1)
        {
            process_received_data();
        }

        // 根据上位机数据发送
        if (G_setFlag == 0x01) // 电脑输出
        {
            if (t_10ms_Flag == 1)
            {
                t_10ms_Flag = 0;
                read_all_rm1002_sensors((float *)c_value);
                // 设置帧号
                UDP_11[4] = (Cap_Frame_Num >> 8);
                UDP_11[5] = (Cap_Frame_Num & 0xff);
                UDP_11[17] = 0x12;

                for (int i = 0; i < 32; i++)
                {
                    process_value(c_value[i], UDP_11, 20 + i * 6);
                }
            }
        }
        if (G_setFlag == 0x0F) // 赛感协议发送数据
        {
            if (t_10ms_Flag == 1)
            {
                t_10ms_Flag = 0;
                read_all_rm1002_sensors((float *)c_value);
                // 设置帧号
                UDP_11[4] = (Cap_Frame_Num >> 8);
                UDP_11[5] = (Cap_Frame_Num & 0xff);
                UDP_11[17] = 0x12;

                for (int i = 0; i < 32; i++)
                {
                    process_value(c_value[i], UDP_11, 20 + i * 6);
                }

                HAL_UART_Transmit(&Uart1Handle, (uint8_t *)UDP_11, sizeof(UDP_11), 20); // 串口发送数据
                while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET); // 等待发送完成
                Cap_Frame_Num++;

                // 如果超出范围则重置
                if (Cap_Frame_Num >= 65534) // 使用更精确的边界值
                {
                    Cap_Frame_Num = 0;
                    // 添加同步标记
                    UDP_11[4] = 0;
                    UDP_11[5] = 0;
                }
            }
        }
        if (UART_RX_CMD_1)
        {
            UART_RX_CMD_1 = 0;        // 清除标志位
            Generic_Reply[5] = 0x01;  // 设置帧号
            Generic_Reply[7] = 0x2a;  // 设置帧长
            Generic_Reply[17] = 0x02; // 设置回复

            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)Generic_Reply, sizeof(Generic_Reply), 20); // 串口发送数据
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_3)
        {
            UART_RX_CMD_3 = 0;

            UDP_11[4] = (Cap_Frame_Num >> 8); // 设置帧号
            UDP_11[5] = (Cap_Frame_Num & 0xff);
            UDP_11[17] = 0x04;

            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)UDP_11, sizeof(UDP_11), 20); // 串口发送数据
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_5)
        {
            UART_RX_CMD_5 = 0;
            Generic_Reply[5] = 0x06;  // 设置帧号
            Generic_Reply[7] = 0x2a;  // 设置帧长
            Generic_Reply[17] = 0x06; // 设置回复

            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)Generic_Reply, sizeof(Generic_Reply), 20);
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_7)
        {
            UART_RX_CMD_7 = 0;
            Generic_Reply[5] = 0x01;  // 设置帧号
            Generic_Reply[7] = 0x2a;  // 设置帧长
            Generic_Reply[17] = 0x08; // 设置回复

            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)Generic_Reply, sizeof(Generic_Reply), 20);
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_9)
        {
            UART_RX_CMD_9 = 0;
            Generic_Reply[5] = 0x01;  // 设置帧号
            Generic_Reply[7] = 0x2a;  // 设置帧长
            Generic_Reply[17] = 0x09; // 设置回复

            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)Generic_Reply, sizeof(Generic_Reply), 20);
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_11)
        {
            // 一直发数据---flag
            G_setFlag = 0x0F;
            UART_RX_CMD_11 = 0;
        }
        if (UART_RX_CMD_13) // 停止发送数据
        {
            // 停止发送数据
            G_setFlag = 0x00;

            UART_RX_CMD_13 = 0;
            Generic_Reply[5] = 0x0E;  // 设置帧号
            Generic_Reply[7] = 0x2a;  // 设置帧长
            Generic_Reply[17] = 0x14; // 设置回复

            // 回复指令
            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)Generic_Reply, sizeof(Generic_Reply), 20); // 串口发送数据
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_15)
        {
            UART_RX_CMD_15 = 0;
            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)UDP_15, sizeof(UDP_15), 20); // 串口发送数据
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET)
                ; // 等待发送完成
        }
        if (UART_RX_CMD_71)
        {
            UART_RX_CMD_71 = 0; // 清除标志位

            // 解析操作类型 (18-19位)
            uint16_t operation = ((uint16_t)G_USART1_RX_Buffer[18] << 8) | (uint16_t)G_USART1_RX_Buffer[19];

            // 解析通道号 (20-21位)
            uint16_t channel = ((uint16_t)G_USART1_RX_Buffer[20] << 8) | (uint16_t)G_USART1_RX_Buffer[21];

            // 解析数值 (22-27位)
            uint32_t int_part = ((uint32_t)G_USART1_RX_Buffer[22] << 16) |
                                ((uint32_t)G_USART1_RX_Buffer[23] << 8) |
                                G_USART1_RX_Buffer[24];

            uint32_t dec_part = ((uint32_t)G_USART1_RX_Buffer[25] << 16) |
                                ((uint32_t)G_USART1_RX_Buffer[26] << 8) |
                                G_USART1_RX_Buffer[27];

            float value = (float)int_part + (float)dec_part / 1000000.0f;

            // 调用封装的函数处理71命令
            process_command_71(channel, operation, value, &Cap_Frame_Num);
        }
#endif
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

