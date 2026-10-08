#include "iwdg.h"

/* 独立看门狗句柄 */
IWDG_HandleTypeDef IwdgHandle;

/**
 * @brief  初始化独立看门狗，设置为1小时超时
 * @note   LSI时钟(约32KHz) / 分频系数(256) = 125Hz
 *         计数器重载值 = 125Hz * 3600秒 = 450000
 *         由于最大重载值为0xFFF(4095)，需要在软件中定期刷新
 *         大约是30秒刷新一次
 * @param  None
 * @retval None
 */
void IWDG_Init(void)
{
    // 初始化IWDG - 设置分频为256
    IwdgHandle.Instance = IWDG;
    IwdgHandle.Init.Prescaler = IWDG_PRESCALER_256;  // 选择最大分频
    IwdgHandle.Init.Reload = 4095;                   // 设置最大重载值
    IwdgHandle.Init.Window = 0;                      // 不使用窗口功能
    
    if (HAL_IWDG_Init(&IwdgHandle) != HAL_OK)
    {
        printf("IWDW Init Error\n");
        Error_Handler();
    }
}

/**
 * @brief  刷新独立看门狗
 * @note   此函数应该在主循环中周期性调用
 *         由于最大计数时间约为33秒(4095/125Hz)
 *         应该每30秒左右调用一次此函数
 * @param  None
 * @retval None
 */
void IWDG_Refresh(void)
{
    HAL_IWDG_Refresh(&IwdgHandle);
}



