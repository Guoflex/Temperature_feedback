#ifndef __GPIO_H__
#define __GPIO_H__

#include "stm32g0xx_hal.h"
#include "main.h"

void LED_GPIO_Init(void);
void System_Clock_Config_HSE_64Mhz(void);
void RM1002_RST1_Init(void);
void RM1002_RST2_Init(void);

#endif

