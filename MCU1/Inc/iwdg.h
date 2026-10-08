#ifndef __IWDG_H__
#define __IWDG_H__

#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_iwdg.h"
#include "stm32g0xx_hal_rcc.h"
#include <stdio.h>
#include "main.h"

/* 独立看门狗句柄 */
extern IWDG_HandleTypeDef IwdgHandle;

/* 函数声明 */
void IWDG_Init(void);
void IWDG_Refresh(void);

/* 如果宏未定义，手动定义 */
#ifndef __HAL_RCC_IWDG_CLK_ENABLE
#define __HAL_RCC_IWDG_CLK_ENABLE()  /* 空定义，因为G0系列可能不需要此操作 */
#endif

#endif /* __IWDG_H__ */



