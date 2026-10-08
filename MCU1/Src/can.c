#include "can.h"

/* FDCAN句柄 */
FDCAN_HandleTypeDef hfdcan1;

/* 发送和接收消息头结构体 */
FDCAN_TxHeaderTypeDef TxHeader1; // 发送消息头
FDCAN_RxHeaderTypeDef RxHeader1; // 接收消息头

/* 发送和接收缓冲区 */
uint8_t can1_txbuf[64]; // 发送缓冲区
uint8_t can1_rxbuf[64]; // 接收缓冲区

/* FDCAN时钟使能计数器 */
static uint32_t HAL_RCC_FDCAN_CLK_ENABLED = 0;

/**
 * @brief FDCAN1初始化函数---初始化基本硬件参数
 * @note  配置FDCAN1为CAN FD模式，支持高速数据传输
 *        仲裁段波特率 = 64M / 1 / 8 / (1 + 10 + 5) = 500k
 *        数据段波特率 = 64M / 1 / 2 / (1 + 10 + 5) = 2M
 */
void FDCAN1_Init(void)
{
    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_FD_BRS; // 必须配置为支持 BRS
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;     // 建议启用自动重传

    hfdcan1.Init.NominalPrescaler = 8;     // 仲裁段时钟预分频系数
    hfdcan1.Init.NominalSyncJumpWidth = 1; // 仲裁段同步跳转宽度
    hfdcan1.Init.NominalTimeSeg1 = 10;     // 仲裁段时间段1
    hfdcan1.Init.NominalTimeSeg2 = 5;      // 仲裁段时间段2

    hfdcan1.Init.DataPrescaler = 2;     // 数据段时钟预分频系数
    hfdcan1.Init.DataSyncJumpWidth = 1; // 数据段同步跳转宽度
    hfdcan1.Init.DataTimeSeg1 = 10;     // 数据段时间段1
    hfdcan1.Init.DataTimeSeg2 = 5;      // 数据段时间段2

    hfdcan1.Init.StdFiltersNbr = 28;                        // 标准帧滤波器数量
    hfdcan1.Init.ExtFiltersNbr = 8;                         // 扩展帧滤波器数量
    hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION; // 发送FIFO队列模式

    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) // 初始化FDCAN
    {
        Error_Handler();
    }
}

/**
 * @brief FDCAN1 MSP初始化函数
 * @note  配置FDCAN1的GPIO和时钟
 *        CANTX->PD1
 *        CANRX->PD0
 * @param fdcanHandle: FDCAN句柄
 */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *fdcanHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (fdcanHandle->Instance == FDCAN1)
    {
        /* 配置FDCAN时钟源为PCLK1 */
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        /* 使能FDCAN时钟 */
        HAL_RCC_FDCAN_CLK_ENABLED++;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
        {
            __HAL_RCC_FDCAN_CLK_ENABLE();
        }

        /* 使能GPIOD时钟 */
        __HAL_RCC_GPIOD_CLK_ENABLE();

        /* 配置FDCAN1的GPIO */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1; // PD0(RX)和PD1(TX)
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;        // 推挽输出
        GPIO_InitStruct.Pull = GPIO_NOPULL;            // 无上拉下拉
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;   // 低速模式
        GPIO_InitStruct.Alternate = GPIO_AF3_FDCAN1;   // 复用为FDCAN1功能
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* 配置FDCAN中断 */
//        HAL_NVIC_SetPriority(TIM16_FDCAN_IT0_IRQn, 0, 0); // 设置中断优先级
//        HAL_NVIC_EnableIRQ(TIM16_FDCAN_IT0_IRQn);         // 使能中断
    }
}

/**
 * @brief FDCAN1 MSP反初始化函数
 * @note  反初始化FDCAN1的GPIO和时钟
 * @param fdcanHandle: FDCAN句柄
 */
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *fdcanHandle)
{
    if (fdcanHandle->Instance == FDCAN1)
    {
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 0)
        {
            __HAL_RCC_FDCAN_CLK_DISABLE();
        }

        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_0 | GPIO_PIN_1);
    }
}



/**
 * @brief FDCAN1配置函数---配置过滤器参数
 * @note  配置FDCAN1的过滤器和发送/接收消息头
 *        1. 配置标准ID和扩展ID的过滤器
 *        2. 配置全局过滤器
 *        3. 激活接收FIFO0新消息通知
 */
void FDCAN1_Config(void)
{
    FDCAN_FilterTypeDef sFilterConfig;

    /* 配置标准ID过滤器 */
    sFilterConfig.IdType = FDCAN_STANDARD_ID;             // 标准ID类型
    sFilterConfig.FilterIndex = 0;                        // 过滤器索引
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;        // 范围过滤器
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // 过滤器输出到RX FIFO0
    sFilterConfig.FilterID1 = 0x0000;                 // 起始ID
    sFilterConfig.FilterID2 = 0x7FF;                 // 结束ID (标准ID最大值为0x7FF)
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* 配置全局过滤器 */
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, 
        FDCAN_ACCEPT_IN_RX_FIFO0,  // 标准帧
        FDCAN_REJECT,              // 扩展帧
        FDCAN_FILTER_REMOTE,       // 远程帧
        FDCAN_FILTER_REMOTE) != HAL_OK)
    {
        Error_Handler();
    }

    /* 启动CAN外围设备 */
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 激活RX FIFO0新消息通知中断 */
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
        Error_Handler();
    }

    /* 激活错误中断 */
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_ERROR_WARNING | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_BUS_OFF, 0) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief FDCAN1 RX FIFO0回调函数
 * @note  处理RX FIFO0中的新消息
 * @param hfdcan: FDCAN句柄
 * @param RxFifo0ITs: RX FIFO0中断标志
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    printf("RxFifo0Callback\r\n");
    if (hfdcan->Instance == FDCAN1)
    {
        /* 从RX FIFO0中获取消息 */
        if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &RxHeader1, can1_rxbuf) != HAL_OK)
        {
            printf("get rx message error\r\n");
            Error_Handler();
        }

        /* 打印接收结果 */
        printf("CAN1 RxData(ID:0x%X): ", RxHeader1.Identifier);
        for (uint32_t i = 0; i < RxHeader1.DataLength; i++)
        {
            printf("%02X ", can1_rxbuf[i]);
        }
        printf("\r\n");
    }
}

uint8_t fdcan1_receive_msg(uint8_t *buf)
{
    if(HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &RxHeader1, buf) != HAL_OK)    /* 读取数据 */
    {
//        printf("receive msg error\r\n");
        return 0;
    }
    else
    {
        printf("receive msg success\r\n");
    }

    
    /* 打印接收到的消息信息 */
    printf("receive msg:\r\n");
    printf("ID: 0x%X\r\n", RxHeader1.Identifier);
    printf("data length: %d\r\n", RxHeader1.DataLength);
    printf("data: ");
    for(uint8_t i = 0; i < (RxHeader1.DataLength); i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
    
    return RxHeader1.DataLength;
}


/**
 * @brief FDCAN1发送数据函数
 * @param id: CAN ID
 * @param data: 发送数据指针
 * @param len: 数据长度
 */
void FDCAN1_SendData(uint32_t id, uint8_t *data, uint8_t len)
{
    TxHeader1.Identifier = id;                         // CAN ID
    TxHeader1.IdType = FDCAN_STANDARD_ID;              // 标准ID类型
    TxHeader1.TxFrameType = FDCAN_DATA_FRAME;          // 数据帧
    TxHeader1.DataLength = len;                        // 数据长度
    TxHeader1.ErrorStateIndicator = FDCAN_ESI_PASSIVE; // 错误状态指示器
    TxHeader1.BitRateSwitch = FDCAN_BRS_OFF;           // 关闭比特率切换
    TxHeader1.FDFormat = FDCAN_FD_CAN;                 // CAN FD 格式
    TxHeader1.TxEventFifoControl = FDCAN_NO_TX_EVENTS; // 不生成发送事件

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader1, data) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan->Instance == FDCAN1)
    {
        printf("FDCAN Error: ");
        if (__HAL_FDCAN_GET_FLAG(hfdcan, FDCAN_FLAG_BUS_OFF))
        {
            printf("Bus Off\r\n");
        }
        if (__HAL_FDCAN_GET_FLAG(hfdcan, FDCAN_FLAG_ERROR_PASSIVE))
        {
            printf("Error Passive\r\n");
        }
        if (__HAL_FDCAN_GET_FLAG(hfdcan, FDCAN_FLAG_ERROR_WARNING))
        {
            printf("Error Warning\r\n");
        }
    }
}

// 添加中断处理函数
void FDCAN1_IT0_IRQHandler(void)
{
    HAL_FDCAN_IRQHandler(&hfdcan1);
}
