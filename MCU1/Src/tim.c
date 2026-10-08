#include "tim.h"

TIM_HandleTypeDef htim1; // 定时器句柄

volatile uint8_t t_10ms_Flag = 0;   // 10ms flag
volatile uint16_t t_10ms_count = 0; // 10ms count

/**
 * @brief 初始化TIM1生成1ms定时
 * @note 系统时钟为64MHz
 * 计算公式：定时时间 = (Prescaler + 1) × (Period + 1) / 时钟频率
 */
void TIM1_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    // 系统时钟为64MHz
    uint32_t timer_clock = 64000000; // 64MHz
    uint32_t target_time = 1;        // 1ms

    // 防止未使用变量警告
    (void)timer_clock;
    (void)target_time;

    htim1.Instance = TIM1;                                        // 定时器1
    htim1.Init.Prescaler = 63;                                    // 预分频器设为63，实际分频系数为64
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;                  // 向上计数
    htim1.Init.Period = 999;                                      // 周期设为999，实际计数为1000
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;            // 时钟分频
    htim1.Init.RepetitionCounter = 0;                             // 重复计数器
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // 启用自动重装载预装载

    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // 内部时钟
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;          // 主输出触发
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;        // 主输出触发2
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE; // 主从模式

    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // 启用定时器更新中断
    if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief TIM1中断回调函数
 * @param htim: TIM句柄指针
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        t_10ms_count++;
        if (t_10ms_count >= 10)
        {
            t_10ms_count = 0;
            t_10ms_Flag = 1; // 可用于生成10ms标志
        }
    }
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if (tim_baseHandle->Instance == TIM1)
    {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1;
        PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_RCC_TIM1_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle)
{

    if (tim_baseHandle->Instance == TIM1)
    {
        __HAL_RCC_TIM1_CLK_DISABLE();

        HAL_NVIC_DisableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    }
}
