#ifndef __RM1002_H__
#define __RM1002_H__

#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_i2c.h"
#include "stm32g0b1xx.h"
#include "main.h"
#include <stdio.h>
#include <stdbool.h>

#define I2C_FASTPLUS_SPEEDCLOCK   0x00910B1C          /* 1MHz */
#define I2C_FAST_SPEEDCLOCK       0x00C12166          /* 400KHz */
#define I2C_STANDARD_SPEEDCLOCK   0x10B17DB5          /* 100KHz */

// ch0
#define PROXTH_CH0_CLS_REAL_VAL    1000000
#define PROXTH_CH0_FAR_REAL_VAL    1000000
// ch1
#define PROXTH_CH1_CLS_REAL_VAL    1000000
#define PROXTH_CH1_FAR_REAL_VAL    1000000
// ch2
#define PROXTH_CH2_CLS_REAL_VAL    1000000
#define PROXTH_CH2_FAR_REAL_VAL    1000000
// ch3
#define PROXTH_CH3_CLS_REAL_VAL	   1000000
#define PROXTH_CH3_FAR_REAL_VAL	   1000000

#define AVG_TARGET_COMP_CH0_REAL_VAL    2000000
#define AVG_TARGET_COMP_CH1_REAL_VAL    2000000
#define AVG_TARGET_COMP_CH2_REAL_VAL    2000000
#define AVG_TARGET_COMP_CH3_REAL_VAL    2000000

// 通道0
#define PROXTH_CH0_CLS          (PROXTH_CH0_CLS_REAL_VAL >> 8)
#define PROXTH_CH0_FAR          (PROXTH_CH0_FAR_REAL_VAL >> 8)
// 通道1
#define PROXTH_CH1_CLS          (PROXTH_CH1_CLS_REAL_VAL >> 8)
#define PROXTH_CH1_FAR          (PROXTH_CH1_FAR_REAL_VAL >> 8)
// 通道2
#define PROXTH_CH2_CLS          (PROXTH_CH2_CLS_REAL_VAL >> 8)
#define PROXTH_CH2_FAR          (PROXTH_CH2_FAR_REAL_VAL >> 8)
// 通道3
#define PROXTH_CH3_CLS          (PROXTH_CH3_CLS_REAL_VAL >> 8)
#define PROXTH_CH3_FAR          (PROXTH_CH3_FAR_REAL_VAL >> 8)

#define AVG_TARGET_COMP_CH0     (AVG_TARGET_COMP_CH0_REAL_VAL >> 8)
#define AVG_TARGET_COMP_CH1     (AVG_TARGET_COMP_CH2_REAL_VAL >> 8)
#define AVG_TARGET_COMP_CH2     (AVG_TARGET_COMP_CH2_REAL_VAL >> 8)
#define AVG_TARGET_COMP_CH3     (AVG_TARGET_COMP_CH2_REAL_VAL >> 8)

#define RM1X01_PROXTH_CH0_CLS           0x49
#define RM1X01_PROXTH_CH0_FAR           0x51

#define RM1X01_RAW0_CH0                 0X75
#define RM1X01_USE0_CH0                 0X81
#define RM1X01_AVG0_CH0                 0X8D
#define RM1X01_DIF0_CH0                 0X99

typedef enum {
    RAW_DATA,
    USE_DATA,
    AVG_DATA,
    DIF_DATA
} RM1X01_DataType;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;

extern volatile uint8_t RM1002B_1_X; // 锐盟1地址
extern volatile uint8_t RM1002B_2_X; // 锐盟2地址
extern volatile uint8_t RM1002B_3_X; // 锐盟3地址

void IIC_Init(I2C_HandleTypeDef *hi2c, I2C_TypeDef *instance, uint8_t device_addr);
void IIC1_Init(uint8_t rm1002_1_addr);
void IIC2_Init(uint8_t rm1002_2_addr);
void rm1002_init(I2C_HandleTypeDef *hi2c, uint8_t rm1002_addr);
void rm1002_1_init(uint8_t rm1002_addr);
void rm1002_2_init(uint8_t rm1002_addr);
void RM1002_Init(void);
float* rm1002_c_func(I2C_HandleTypeDef *hi2c, uint8_t rm1002_addr);
uint32_t rm1002_adc_data_get(I2C_HandleTypeDef *hi2c, int dataType, int channel, uint8_t rm1002b_addr);
int32_t sign24To32(uint32_t data);

#endif

