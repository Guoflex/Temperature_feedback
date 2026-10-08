#ifndef __MAIN_H
#define __MAIN_H

#include "stm32g0xx_hal.h"
#include <stdio.h>
#include "gpio.h"
#include "iwdg.h"
#include "usart.h"
#include "rm1002.h"
#include "i2c.h"
#include "tim.h"
#include "can.h"
#include "flash.h"

/* 锐盟地址 */
#define RM_DEFAULE_I2C_ADDR 0x2B
#define DEFAULE_I2C_ADDR_0  (0x2B) // 00101011
#define DEFAULE_I2C_ADDR_1  (0x2A) // 00101010
#define DEFAULE_I2C_ADDR_2  (0x29) // 00101001
#define DEFAULE_I2C_ADDR_3  (0x28) // 00101000

#define DEFAULE_I2C_ADDR_0_CONF 0x03
#define DEFAULE_I2C_ADDR_3_CONF 0x00
#define DEFAULE_I2C_ADDR_2_CONF 0x01
#define DEFAULE_I2C_ADDR_1_CONF 0x02

#define FLEX_FLAG 0

extern float par1_0_cap[4];
extern float par1_1_cap[4];
extern float par1_2_cap[4];
extern float par1_3_cap[4];
extern float par2_0_cap[4];
extern float par2_1_cap[4];
extern float par2_2_cap[4];
extern float par2_3_cap[4];

void Error_Handler(void);

#endif
