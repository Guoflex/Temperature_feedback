#include "usart.h"

UART_HandleTypeDef Uart1Handle;
GPIO_InitTypeDef GPIO_InitStruct;

// 定义接收缓冲区和相关变量
uint8_t  G_USART1_RX_Buffer[USART1_RX_BUFFER_SIZE] = {0};  // 串口1接收缓冲区
uint32_t G_USART1_RX_Count                         = 0;    // 串口1接收计数
volatile uint8_t  G_USART1_RX_IDLE_Flag            = 0;    // 空闲中断标记
volatile uint8_t  G_setFlag                        = 0x00; // 状态标识
volatile uint8_t UART_RX_CMD_1  = 0;
volatile uint8_t UART_RX_CMD_3  = 0;
volatile uint8_t UART_RX_CMD_5  = 0;
volatile uint8_t UART_RX_CMD_7  = 0;
volatile uint8_t UART_RX_CMD_9  = 0;
volatile uint8_t UART_RX_CMD_11 = 0;
volatile uint8_t UART_RX_CMD_13 = 0;
volatile uint8_t UART_RX_CMD_15 = 0;
volatile uint8_t UART_RX_CMD_71 = 0; // FF协议收发
float *c_read_value;                 // 传感器值
volatile float c_value[SENSOR_NUM] = {0.0};  // 传感器值
float coe_buffer[32] = {1.0};

/* 协议发送-------------------------------------------------------------------*/
uint8_t Generic_Reply[] = {
    FRAME_HEADER          // 帧头
    ,FRAME_NUMBER         // 帧序号
    ,FRAME_LENGTH         // 帧长
    ,DEVICE_ID            // 设备ID
    ,TARGET_ID            // 目标ID
    ,RESPONSE_CODE        // 命令字
    ,STATUS_CODE          // 数据
    ,SYC_STRING           // 设备类型
    ,RESERVED_SPACE       // 保留
};

// 32个点
uint8_t UDP_11[] = {
    FRAME_HEADER                                //0,1,2,3         固定：帧头
    ,0x00,0x00                                  //4,5             变量：帧序列号
    ,0x00,0xCC                                  //6,7             固定：帧长
    ,DEVICE_ID                                  //8,9,10,11       固定：设备ID
    ,TARGET_ID                                  //12,13,14,15     固定：目标ID
    ,0x00,0x12                                  //16,17           固定：命令字
    ,0x00,0x20                                  //18,19           固定：状态字---传感器个数
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00  
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00  
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00  
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00  
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00
    ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00
};

uint8_t UDP_15[] = {
    FRAME_HEADER
    ,0x00,0x01
    ,0x00,0x0c
    ,DEVICE_ID
    ,TARGET_ID
    ,0x00,0x16
    ,0x00,0x03
};

uint8_t UDP_71[] = {
    FRAME_HEADER  // 0-3   帧头
    ,0x00,0x00    // 4-5   帧序号
    ,0x00,0xCE    // 6-7   帧长
    ,DEVICE_ID    // 8-11  设备ID
    ,TARGET_ID    // 12-15 目标ID
    ,0x00,0x72    // 16-17 命令字
    ,0x00,0x02    // 18-19 状态字：00-失败，01-所有通道，02-单通道
    ,0x00,0x00    // 20-21 通道
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor0 22-27
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor1 28-33
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor2 34-39
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor3 40-45
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor4 46-51
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor5 52-57
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor6 58-63
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor7 64-69
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor8 70-75
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor9 76-81
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor10 82-87
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor11 88-93
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor12 94-99
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor13 100-105
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor14 106-111
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor15 112-117
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor16 118-123
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor17 124-129
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor18 130-135
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor19 136-141
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor20 142-147
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor21 148-153
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor22 154-159
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor23 160-165
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor24 166-171
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor25 172-177
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor26 178-183
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor27 184-189
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor28 190-195
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor29 196-201
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor30 202-207
    ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor31 208-213
};


// USART1 GPIO 初始化
void USART1_GPIO_Init(void)
{
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;   
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


/**
 * @brief 串口初始化
 *        初始化串口1，使用PA9和PA10
 *        TX:PA9,RX:PA10
 * @param baudrate 串口波特率
 * @retval None
 */
void USART1_Init(uint32_t baudrate)
{
    USART1_GPIO_Init();
    
    /* 配置USART1参数 */
    Uart1Handle.Instance             = USART1;                // 选择USART1
    Uart1Handle.Init.BaudRate        = baudrate;              // 设置波特率
    Uart1Handle.Init.WordLength      = UART_WORDLENGTH_8B;    // 设置数据位
    Uart1Handle.Init.StopBits        = UART_STOPBITS_1;       // 设置停止位
    Uart1Handle.Init.Parity          = UART_PARITY_NONE;      // 设置校验位
    Uart1Handle.Init.Mode            = UART_MODE_TX_RX;       // 设置模式
    Uart1Handle.Init.HwFlowCtl       = UART_HWCONTROL_NONE;   // 设置硬件流
    Uart1Handle.Init.OverSampling    = UART_OVERSAMPLING_16;
    Uart1Handle.Init.OneBitSampling  = UART_ONE_BIT_SAMPLE_DISABLE;
    Uart1Handle.Init.ClockPrescaler  = UART_PRESCALER_DIV1;

    /* 初始化USART1 */
    if (HAL_UART_Init(&Uart1Handle) != HAL_OK)
    {
        Error_Handler();
    }

    /* 清除可能存在的错误标志 */
    __HAL_UART_CLEAR_PEFLAG(&Uart1Handle);
    __HAL_UART_CLEAR_FEFLAG(&Uart1Handle);
    __HAL_UART_CLEAR_OREFLAG(&Uart1Handle);
    __HAL_UART_CLEAR_NEFLAG(&Uart1Handle);
    
    /* 初始化接收缓冲区和标志位 */
    memset(G_USART1_RX_Buffer, 0, USART1_RX_BUFFER_SIZE);
    G_USART1_RX_Count = 0;
    G_USART1_RX_IDLE_Flag = 0;
    
    /* 启用接收中断 */
    USART1_EnableRxInterrupt();
}

/**
 * @brief 通过USART1发送一个字节
 * @param byte 要发送的字节
 * @retval None
 */
void USART1_SendByte(uint8_t byte)
{
    HAL_UART_Transmit(&Uart1Handle, &byte, 1, 1000);
}

/**
 * @brief 通过USART1发送字符串
 * @param str 要发送的字符串
 * @retval None
 */
void USART1_SendString(char *str)
{
    while (*str)
    {
        USART1_SendByte(*str++);
    }
}

/**
 * @brief 串口1中断函数
 * 使用空闲中断接收数据
 */
void USART1_IRQHandler(void)
{
    uint32_t isrflags   = READ_REG(USART1->ISR);
    uint32_t cr1its     = READ_REG(USART1->CR1);
    uint8_t received_byte;

    /* 检查RXNE中断 - 接收数据寄存器非空 */
    if(((isrflags & USART_ISR_RXNE_RXFNE) != 0) && ((cr1its & USART_CR1_RXNEIE_RXFNEIE) != 0))
    {
        /* 读取接收到的数据 */
        received_byte = (uint8_t)(USART1->RDR & 0xFF);
        
        /* 判断缓冲区是否已满 */
        if(G_USART1_RX_Count < USART1_RX_BUFFER_SIZE)
        {
            /* 将接收到的字节存入缓冲区 */
            G_USART1_RX_Buffer[G_USART1_RX_Count++] = received_byte;
        }
    }
    
    /* 检查IDLE中断 - 总线空闲检测 */
    if(((isrflags & USART_ISR_IDLE) != 0) && ((cr1its & USART_CR1_IDLEIE) != 0))
    {
        /* 清除IDLE标志位 */
        __HAL_UART_CLEAR_IDLEFLAG(&Uart1Handle);
        
        /* 设置空闲中断标志，表示一帧数据接收完成 */
        G_USART1_RX_IDLE_Flag = 1;
        
        /* 临时调试信息: 接收到的字节数 */
        // 注释掉下面这行代码，避免在中断中打印影响性能
        // printf("接收: %d字节\r\n", G_USART1_RX_Count);
    }
    
    /* 处理错误中断 */
    uint32_t errorflags = (isrflags & (USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
    if (errorflags != 0)
    {
        /* 清除错误标志 */
        __HAL_UART_CLEAR_PEFLAG(&Uart1Handle);
        __HAL_UART_CLEAR_FEFLAG(&Uart1Handle);
        __HAL_UART_CLEAR_OREFLAG(&Uart1Handle);
        __HAL_UART_CLEAR_NEFLAG(&Uart1Handle);
        
        /* 读取DR寄存器以清除标志 */
        (void)USART1->RDR;
    }
    
    /* 调用HAL库中断处理函数 */
    HAL_UART_IRQHandler(&Uart1Handle);
}


/**
 * @brief 处理FF协议收发
 * 
 */
void process_received_data(void)
{
    uint16_t USART1_RX_Len = G_USART1_RX_Count; // 使用实际接收到的字节数
    
    if(USART1_RX_Len > 0)
    {
        if (G_USART1_RX_Buffer[0] == 0x23 && G_USART1_RX_Buffer[1] == 0x24)
        {
            // 串口调试
            switch(G_USART1_RX_Buffer[2])
            {
                case 0x01:
                    G_setFlag = 0x01; // printf打印
                    break;
            }
        }
        else if (G_USART1_RX_Buffer[0] == 0xFF && G_USART1_RX_Buffer[1] == 0xFF && 
                G_USART1_RX_Buffer[2] == 0x06 && G_USART1_RX_Buffer[3] == 0x09)
        {
            // 处理FF FF 06 09数据包，确保数据长度足够
            switch(G_USART1_RX_Buffer[17])
            {
                case 0x01:
                    UART_RX_CMD_1 = 1;
                    break;
                case 0x03:
                    UART_RX_CMD_3 = 1;
                    break;
                case 0x05:
                    UART_RX_CMD_5 = 1;
                    break;
                case 0x07:
                    UART_RX_CMD_7 = 1;
                    break;
                case 0x09:
                    UART_RX_CMD_9 = 1;
                    break;
                case 0x11:
                    UART_RX_CMD_11 = 1;
                    break;
                case 0x13:
                    UART_RX_CMD_13 = 1;
                    break;
                case 0x15:
                    UART_RX_CMD_15 = 1;
                    break;
                case 0x71:// 新增的协议处理
                    UART_RX_CMD_71 = 1;
                    break;
            }
        }
        // 清空缓冲区和标志位放在函数末尾
        memset(G_USART1_RX_Buffer, 0, USART1_RX_BUFFER_SIZE); // 使用实际缓冲区大小
        G_USART1_RX_Count = 0;
        G_USART1_RX_IDLE_Flag = 0;
    }
}

/**
 * @brief 读取所有RM1002传感器数据
 * 
 */
void read_all_rm1002_sensors(float *c_value)
{
    float *temp;
    int offset = 0;
    
    // 定义I2C和地址配置
    I2C_HandleTypeDef *i2c_handles[] = {&hi2c1, &hi2c2};
    uint8_t addresses[] = {
        DEFAULE_I2C_ADDR_3, DEFAULE_I2C_ADDR_2,
        DEFAULE_I2C_ADDR_1, DEFAULE_I2C_ADDR_0
    };
    
    // 读取所有传感器数据
    for(int i2c = 0; i2c < 2; i2c++) {
        for(int addr = 0; addr < 4; addr++) {
            temp = rm1002_c_func(i2c_handles[i2c], addresses[addr]);
            memcpy(&c_value[offset], temp, 4 * sizeof(float));
            offset += 4;
        }
    }
}

/**
 * @brief 将浮点数转换为UDP_11的格式
 * 
 * @param value 
 * @param udp_array 
 * @param start_index 
 */
void process_value(float value, uint8_t *udp_array, int start_index) 
{
    int int_part = (int)value; // 整数部分
    float flo_part = value - int_part; // 小数部分
    unsigned long int flo_to_int = (unsigned long int)(flo_part * 1000000); // 放大成整数处理

    udp_array[start_index]     = (int_part   & 0xFF0000) >> 16; // 整数部分
    udp_array[start_index + 1] = (int_part   & 0x00FF00) >> 8;
    udp_array[start_index + 2] = (int_part   & 0x0000FF);
    udp_array[start_index + 3] = (flo_to_int & 0xFF0000) >> 16; // 小数部分
    udp_array[start_index + 4] = (flo_to_int & 0x00FF00) >> 8;
    udp_array[start_index + 5] = (flo_to_int & 0x0000FF);
}


/**
 * @brief 启用串口接收中断
 * 需要在USART1_Init函数的最后调用
 */
void USART1_EnableRxInterrupt(void)
{
    /* 使能USART1接收中断 */
    __HAL_UART_ENABLE_IT(&Uart1Handle, UART_IT_RXNE);
    
    /* 重新确认IDLE中断已正确使能 */
    __HAL_UART_CLEAR_IDLEFLAG(&Uart1Handle);
    __HAL_UART_ENABLE_IT(&Uart1Handle, UART_IT_IDLE);
    
    /* 配置NVIC优先级并使能中断 */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * @brief 重置串口接收缓冲区
 * 在处理完数据后调用此函数清空缓冲区
 */
void USART1_ResetRxBuffer(void)
{
    G_USART1_RX_Count = 0;
    G_USART1_RX_IDLE_Flag = 0;
}

// 重定向printf
int fputc(int ch, FILE *f)
{
    USART1_SendByte((uint8_t)ch);
    return ch;
}

int fgetc(FILE *f)
{
    uint8_t ch = 0;
    // 在这里添加接收一个字节的代码
    return ch;
}

/**
 * @brief 启用DMA接收
 * 使用DMA方式接收数据
 */
void USART1_DMA_Receive_Start(void)
{
    /* 使用HAL_UART_Receive_DMA启动接收 */
    HAL_UART_Receive_DMA(&Uart1Handle, G_USART1_RX_Buffer, USART1_RX_BUFFER_SIZE);
    
    /* 使能空闲中断 */
    __HAL_UART_ENABLE_IT(&Uart1Handle, UART_IT_IDLE);
}


// 处理71命令的函数实现
void process_command_71(uint16_t channel, uint16_t operation, float value, uint16_t *Cap_Frame_Num)
{
    uint8_t original_flag;
    
    switch(operation) 
    {
        case 0x0000:  // 读取单个通道系数
            UDP_71[7] = 0x14; // 修改帧长度

            // 状态位
            UDP_71[18] = 0x00;
            UDP_71[19] = 0x02; // 0x02表示读取单个通道系数

            // 通道
            UDP_71[20] = channel >> 8;
            UDP_71[21] = channel & 0xff;
            
            // 先读取flash
            // Flash_Read(FLASH_COE_ADDR, sizeof(coe_buffer), (uint32_t *)coe_buffer);
            // HAL_Delay(100);

            // 将对应通道的系数转成6字节数据
            process_value(coe_buffer[channel], UDP_71, 22);
            
            // 发送回复
            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)UDP_71, UDP_71_ONE_SENSOR_LENGTH, 20);
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET);
            break;
            
        case 0x0001:  // 读取所有通道系数
        {
            // 状态位
            UDP_71[18] = 0x00;
            UDP_71[19] = 0x01; // 0x01表示读取所有通道系数

            // 先读取flash
            // Flash_Read(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);

            // 处理采集到的16个电容值 --- 指尖的对应位置
            for(int i=0; i<32; i++)
            {
                process_value(coe_buffer[i], UDP_71,  22+i*6);
            }

            // 发送数据
            HAL_UART_Transmit(&Uart1Handle, (uint8_t *)UDP_71, sizeof(UDP_71), 10);
            while (__HAL_UART_GET_FLAG(&Uart1Handle, UART_FLAG_TC) == RESET);
            break;
        }
            
        case 0x0002:  // 设置单个通道系数(临时修改)
        {
            coe_buffer[channel] = value;
            break;
        }

        case 0x0003:  // 保存所有通道系数
        {
            // 临时停止数据发送
            original_flag = G_setFlag;
            G_setFlag = 0x00;
            
            // 等待最后一帧发送完成
            HAL_Delay(20);
            
            // 写入flash
            // Flash_Write(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);
            // HAL_Delay(100);
            
            // 读取flash
            // Flash_Read(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);
            
            // 恢复原来的发送状态
            G_setFlag = original_flag;
            
            // 重置帧号和帧头
            *Cap_Frame_Num = 0;
            break;
        }
    }
}


