#ifndef __TIM_H
#define __TIM_H

#include "main.h"
#include "stm32g0xx_hal_rcc_ex.h"

extern TIM_HandleTypeDef htim1;
extern volatile uint8_t t_10ms_Flag;

void TIM1_Init(void);


#endif





