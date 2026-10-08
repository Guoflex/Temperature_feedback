#include "rm1002.h"

// I2C_HandleTypeDef hi2c1;
// I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;

volatile uint8_t RM1002B_1_X = 0; // 锐盟1地址
volatile uint8_t RM1002B_2_X = 0; // 锐盟2地址
volatile uint8_t RM1002B_3_X = 0; // 锐盟3地址

/**
 * @brief RM1002B系列寄存器配置
 */
struct RM1X01_REG_CFG
{
    uint8_t reg_address;
    uint8_t reg_value;
};

/**
 * @brief RM1002B系列默认配置
 */
struct RM1X01_REG_CFG rm1002_defaultcfg[] = {
    {0x05, 0xf9},

    {0x0c, 0x40},
    {0x0d, 0x40},
    {0x0e, 0x40},
    {0x0f, 0x40},
    {0x14, 0x83},

    {0x16, 0x0f}, // 选择通道，CH0:0001  CH1:0010 CH2:0100 CH3:1000
    {0x17, 0x00}, // Scan周期配置
    {0x19, 0x06}, // ADC单次采样时间。值越小，变化越大
    {0x1a, 0x06}, // ADC单次积分时间。速度

    {0x1b, 0x00}, // CH0和CH1对应工作模式选择
    {0x1c, 0x00}, // CH2和CH3对应工作模式选择

    {0x1d, 0x02}, // CH0对应的物理通道配置
    {0x1e, 0x08}, // CH1对应的物理通道配置
    {0x1f, 0x20}, // CH2对应的物理通道配置
    {0x20, 0x80}, // CH3对应的物理通道配置

    {0x21, 0x03}, // 采样率，改小速度快，可能有噪声
    {0x22, 0x03},
    {0x23, 0x03},
    {0x24, 0x03},

    {0x25, 0x00},
    {0x26, 0x00},
    {0x27, 0x00},
    {0x28, 0x00},
    {0x29, 0x00},

    {0x2a, 0x80},
    {0x2b, 0x81},
    {0x2c, 0x81},
    {0x2d, 0x81},
    {0x2e, 0x81},

    {0x2f, 0xff}, // 0x00：使用人工设置的offset，0xFF：使能自动补偿
    {0x30, 0x41},
    {0x31, 0xff & AVG_TARGET_COMP_CH0},
    {0x32, 0xff & (AVG_TARGET_COMP_CH0 >> 8)},
    {0x33, 0xff & AVG_TARGET_COMP_CH1},
    {0x34, 0xff & (AVG_TARGET_COMP_CH1 >> 8)},
    {0x35, 0xff & AVG_TARGET_COMP_CH2},
    {0x36, 0xff & (AVG_TARGET_COMP_CH2 >> 8)},
    {0x37, 0xff & AVG_TARGET_COMP_CH3},
    {0x38, 0xff & (AVG_TARGET_COMP_CH3 >> 8)},

    {0x39, 0x00},
    {0x3a, 0x00},
    {0x3b, 0x00},
    {0x3c, 0x00},

    {0x49, 0xff & PROXTH_CH0_CLS},
    {0x4a, 0xff & (PROXTH_CH0_CLS >> 8)},
    {0x4b, 0xff & PROXTH_CH1_CLS},
    {0x4c, 0xff & (PROXTH_CH1_CLS >> 8)},
    {0x4d, 0xff & PROXTH_CH2_CLS},
    {0x4e, 0xff & (PROXTH_CH2_CLS >> 8)},
    {0x4f, 0xff & PROXTH_CH3_CLS},
    {0x50, 0xff & (PROXTH_CH3_CLS >> 8)},

    {0x51, 0xff & PROXTH_CH0_FAR},
    {0x52, 0xff & (PROXTH_CH0_FAR >> 8)},
    {0x53, 0xff & PROXTH_CH1_FAR},
    {0x54, 0xff & (PROXTH_CH1_FAR >> 8)},
    {0x55, 0xff & PROXTH_CH2_FAR},
    {0x56, 0xff & (PROXTH_CH2_FAR >> 8)},
    {0x57, 0xff & PROXTH_CH3_FAR},
    {0x58, 0xff & (PROXTH_CH3_FAR >> 8)},

    {0x65, 0x00}, // 中断使能寄存器
    {0x66, 0x00},
    {0x67, 0x07}, // 中断配置寄存器0
    {0x68, 0x01}, // 中端配置寄存器1
};

/**
 * @brief  RM1002B系列连续写寄存器函数
 * RM1002B系列连续写寄存器函数；建议一次性写寄存器不超过10个
 */
static bool rm1002_reg_write(I2C_HandleTypeDef *hi2c, uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t rm1002b_addr)
{
    if (HAL_I2C_Mem_Write(hi2c, (uint16_t)rm1002b_addr << 1, reg_start_addr, 1, (uint8_t *)reg_array, num, 5000) != HAL_OK)
    {
        return false;
    }
    while (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY);

    return true;
}

/**
 * @brief  RM1002B系列连续读寄存器函数
 * RM1002B系列连续读寄存器；建议一次性读寄存器不超过3个
 */
static bool rm1002_reg_read(I2C_HandleTypeDef *hi2c, uint8_t reg_start_addr, uint8_t *reg_array, uint8_t num, uint8_t rm1002b_addr)
{
    if (HAL_I2C_Mem_Read(hi2c, (uint16_t)rm1002b_addr << 1, reg_start_addr, 1, reg_array, num, 5000) != HAL_OK)
    {
        return false;
    }
    while (HAL_I2C_GetState(hi2c) != HAL_I2C_STATE_READY);

    return true;
}

/**
 * @brief 设置RM1002B寄存器配置
 *
 * @param hi2c
 */
static void rm1002_setconfig(I2C_HandleTypeDef *hi2c, uint8_t rm1002b_addr)
{
    int i = 0;
    uint8_t tx_buf[4] = {0};

    for (i = 0; i < sizeof(rm1002_defaultcfg) / sizeof(rm1002_defaultcfg[0]); i++)
    {
        tx_buf[0] = rm1002_defaultcfg[i].reg_value;
        rm1002_reg_write(hi2c, rm1002_defaultcfg[i].reg_address, tx_buf, 1, rm1002b_addr);
    }

    for (i = 0; i <= 0x68; i++)
    {
        if (rm1002_reg_read(hi2c, i, tx_buf, 1, rm1002b_addr) == false)
        {
            printf("read reg error\n");
        }
    }
}

/**
 * @brief 通用I2C初始化函数
 *
 * @param hi2c I2C句柄指针
 * @param i2c_instance I2C实例 (I2C1, I2C2等)
 * @param device_addr 设备地址
 */
void IIC_Init(I2C_HandleTypeDef *hi2c, I2C_TypeDef *i2c_instance, uint8_t device_addr)
{
    /* I2C基本配置 */
    hi2c->Instance = i2c_instance;                        // 选择I2C外设
    hi2c->Init.Timing = I2C_FAST_SPEEDCLOCK;              // 时序配置，对应快速模式(约1MHz)
    hi2c->Init.OwnAddress1 = device_addr << 1;            // 设置自身地址1（左移1位因为地址格式要求）
    hi2c->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;  // 使用7位地址模式
    hi2c->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; // 禁用双地址模式
    hi2c->Init.OwnAddress2 = 0;                           // 自身地址2(未使用)
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;         // 无地址2掩码
    hi2c->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; // 禁用广播呼叫模式
    hi2c->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;     // 禁用时钟拉伸

    /* 初始化I2C */
    if (HAL_I2C_Init(hi2c) != HAL_OK)
    {
        printf("I2C init failed\r\n");
        Error_Handler();
    }

    /* 配置模拟滤波器 */
    if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        printf("I2C analog filter config failed\r\n");
        Error_Handler();
    }

    /* 配置数字滤波器 */
    if (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) != HAL_OK)
    {
        printf("I2C digital filter config failed\r\n");
        Error_Handler();
    }
}

/**
 * @brief 初始化I2C1
 *
 * @param rm1002_1_addr 设备地址
 */
void IIC1_Init(uint8_t rm1002_1_addr)
{
    IIC_Init(&hi2c1, I2C1, rm1002_1_addr);
}

/**
 * @brief 初始化I2C2
 *
 * @param rm1002_2_addr 设备地址
 */
void IIC2_Init(uint8_t rm1002_2_addr)
{
    IIC_Init(&hi2c2, I2C2, rm1002_2_addr);
}

/**
 * @brief 初始化I2C3
 *
 * @param rm1002_3_addr 设备地址
 */
void IIC3_Init(uint8_t rm1002_3_addr)
{
    IIC_Init(&hi2c3, I2C3, rm1002_3_addr);
}

/**
 * @brief 通用RM1002芯片初始化函数---RM最快跑400kHz速度
 * @param hi2c I2C句柄指针
 * @param rm1002_addr RM1002设备地址
 */
void rm1002_init(I2C_HandleTypeDef *hi2c, uint8_t rm1002_addr)
{
    uint8_t rxbuf[4] = {0};
    uint8_t wdata[4] = {0x80, 0x80, 0x80, 0x80};
    uint8_t data = 0x00;

    data = 0xcc; // RM1002B复位
    if (rm1002_reg_write(hi2c, 0x6f, &data, 1, RM_DEFAULE_I2C_ADDR) == false)
    {
        printf("write reset error\r\n");
    }
    else
    {
        printf("write reset success\r\n");
    }

    HAL_Delay(200);

    if (rm1002_addr == DEFAULE_I2C_ADDR_0)
    {
        data = 0x03; // 11   2b
        rm1002_reg_write(hi2c, 0x59, &data, 1, RM_DEFAULE_I2C_ADDR);
    }
    if (rm1002_addr == DEFAULE_I2C_ADDR_1)
    {
        data = 0x02; // 10   2a
        rm1002_reg_write(hi2c, 0x59, &data, 1, RM_DEFAULE_I2C_ADDR);
    }
    if (rm1002_addr == DEFAULE_I2C_ADDR_2)
    {
        data = 0x01; // 01   29
        rm1002_reg_write(hi2c, 0x59, &data, 1, RM_DEFAULE_I2C_ADDR);
    }
    if (rm1002_addr == DEFAULE_I2C_ADDR_3)
    {
        data = 0x00; // 00   28
        rm1002_reg_write(hi2c, 0x59, &data, 1, RM_DEFAULE_I2C_ADDR);
    }
    HAL_Delay(200);

    // 读取who am I 地址
    if (rm1002_reg_read(hi2c, 0x70, rxbuf, 1, rm1002_addr) == false)
    {
        printf("read id error\r\n");
    }
    else
    {
        printf("id: %x work fine\r\n", rxbuf[0]);
    }

    data = 0x00;
    rm1002_reg_write(hi2c, 0x17, &data, 1, rm1002_addr);
    HAL_Delay(100); // 原本是70ms
    rm1002_setconfig(hi2c, rm1002_addr);
    HAL_Delay(100);

    data = 0xff;
    rm1002_reg_write(hi2c, 0x2f, &data, 1, rm1002_addr);
    HAL_Delay(1000); // 原本是800ms

    rm1002_reg_write(hi2c, 0x39, wdata, 4, rm1002_addr);
}

/**
 * @brief 初始化RM1芯片，配置I2C1接口
 *
 * @param rm1002_1_addr RM1设备地址
 */
void rm1002_1_init(uint8_t rm1002_1_addr)
{
    rm1002_init(&hi2c1, rm1002_1_addr);
}

/**
 * @brief 初始化RM2芯片，配置I2C2接口
 *
 * @param rm1002_2_addr RM2设备地址
 */
void rm1002_2_init(uint8_t rm1002_2_addr)
{
    rm1002_init(&hi2c2, rm1002_2_addr);
}

/**
 * @brief 初始化RM3芯片，配置I2C3接口
 *
 * @param rm1002_3_addr RM3设备地址
 */
void rm1002_3_init(uint8_t rm1002_3_addr)
{
    rm1002_init(&hi2c3, rm1002_3_addr);
}

void rm1002_init_slave_addr(I2C_HandleTypeDef* hi2c, uint8_t addr, uint8_t add_conf)
{
    HAL_StatusTypeDef rtn;
    uint8_t txbuf[6] = {0};
    uint8_t rxbuf[4] = {0};

    // 00 28
    txbuf[0] = 0x59;
    txbuf[1] = add_conf;
    HAL_I2C_Master_Transmit(hi2c, RM_DEFAULE_I2C_ADDR << 1, txbuf, 2, 1000); // 软复位
    HAL_Delay(10);

    // 读取who am I 地址
    rtn = HAL_I2C_Mem_Read(hi2c, addr << 1, 0x70, 1, rxbuf, 1, 1000); // 确认地址是否修改成功
    if (rtn != HAL_OK)
    {
        printf("read id error\r\n");
    }
    else
    {
        printf("id: %x work fine\r\n", rxbuf[0]);
    }

    rm1002_setconfig(hi2c, addr);
    HAL_Delay(10);

    txbuf[0] = 0x39;
    txbuf[1] = 0x80;
    txbuf[2] = 0x80;
    txbuf[3] = 0x80;
    txbuf[4] = 0x80;

    HAL_I2C_Master_Transmit(hi2c, addr << 1, txbuf, 5, 1000);
    HAL_Delay(10);
}

void rm1002_init_new(void)
{
    // IIC 1
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c1, DEFAULE_I2C_ADDR_3, DEFAULE_I2C_ADDR_3_CONF);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c1,DEFAULE_I2C_ADDR_2, DEFAULE_I2C_ADDR_2_CONF);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c1,DEFAULE_I2C_ADDR_1, DEFAULE_I2C_ADDR_1_CONF);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c1,DEFAULE_I2C_ADDR_0, DEFAULE_I2C_ADDR_0_CONF);

    // IIC 2
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c2, DEFAULE_I2C_ADDR_3, DEFAULE_I2C_ADDR_3_CONF);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c2, DEFAULE_I2C_ADDR_2, DEFAULE_I2C_ADDR_2_CONF);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c2, DEFAULE_I2C_ADDR_1, DEFAULE_I2C_ADDR_1_CONF);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    HAL_Delay(10);
    rm1002_init_slave_addr(&hi2c2, DEFAULE_I2C_ADDR_0, DEFAULE_I2C_ADDR_0_CONF);
}

void RM1002_Init(void)
{
    // 初始化RM的EN GPIO引脚
    RM1002_RST1_Init();
    RM1002_RST2_Init();

    HAL_Delay(10);

    rm1002_init_new();
}

/**
 * @brief 初始化8个RM1002B
 *
 */
void RM1002_Init_(void)
{
    /* 初始化RM引脚*/
    RM1002_RST1_Init();
    RM1002_RST2_Init();
    HAL_Delay(10);

    /* 我不明白为什么DEFAULE_I2C_ADDR_1和DEFAULE_I2C_ADDR_0的地址昨天还是好的，今天就得换一下顺序 */
    /* 所以保险起见，我一个一个初始化 */
    // I2C1
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
    RM1002B_1_X = DEFAULE_I2C_ADDR_1;
    rm1002_1_init(RM1002B_1_X); // 这里初始化的是I2C1

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    RM1002B_1_X = DEFAULE_I2C_ADDR_3;
    rm1002_1_init(RM1002B_1_X);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    RM1002B_1_X = DEFAULE_I2C_ADDR_2;
    rm1002_1_init(RM1002B_1_X);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET);
    RM1002B_1_X = DEFAULE_I2C_ADDR_0;
    rm1002_1_init(RM1002B_1_X);

    // I2C2
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    RM1002B_2_X = DEFAULE_I2C_ADDR_1;
    rm1002_2_init(RM1002B_2_X);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    RM1002B_2_X = DEFAULE_I2C_ADDR_3;
    rm1002_2_init(RM1002B_2_X);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
    RM1002B_2_X = DEFAULE_I2C_ADDR_2;
    rm1002_2_init(RM1002B_2_X);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    RM1002B_2_X = DEFAULE_I2C_ADDR_0;
    rm1002_2_init(RM1002B_2_X);
}

/**
 * @brief 将24位数据转换为32位数据
 *
 * @param bit24
 * @return int32_t
 */
int32_t sign24To32(uint32_t bit24)
{
    if ((bit24 & 0x800000) == 0x800000)
    {
        bit24 |= 0xff000000;
    }
    return bit24;
}

/**
 * @brief 获取ADC数据
 *
 * @param dataType
 * @param channel
 * @param rm1002b_addr
 * @return uint32_t
 */
uint32_t rm1002_adc_data_get(I2C_HandleTypeDef *hi2c, int dataType, int channel, uint8_t rm1002b_addr)
{
    uint8_t rx_buf[3] = {0};
    uint8_t reg_base_addr;
    uint32_t ret_data = 0;

    switch (dataType)
    {
    case RAW_DATA:
        reg_base_addr = channel * 3 + RM1X01_RAW0_CH0;
        break;
    case USE_DATA:
        reg_base_addr = channel * 3 + RM1X01_USE0_CH0;
        break;
    case AVG_DATA:
        reg_base_addr = channel * 3 + RM1X01_AVG0_CH0;
        break;
    case DIF_DATA:
        reg_base_addr = channel * 3 + RM1X01_DIF0_CH0;
        break;
    default:
        break;
    }
    if (rm1002_reg_read(hi2c, reg_base_addr, rx_buf, 3, rm1002b_addr) == true)
    {
        ret_data = rx_buf[0] | (rx_buf[1] << 8) | (rx_buf[2] << 16);
    }
    return ret_data;
}

float *rm1002_c_func(I2C_HandleTypeDef *hi2c, uint8_t rm1002_addr)
{
    int i = 0;
    int32_t rawdata[4] = {0};
    uint8_t cxoff[4] = {0};

    static float c1[4] = {0.0}; // 传感器值
    float c2[4] = {0.0};        // 与传感器串联电容值
    float cxx[4] = {0};         // 引脚寄生电容
    float cx0[4] = {0.0};       //
    float cx2[4] = {0.0};       // 1002测出总电容值
    float coff[4] = {0};        // pF  芯片内部减去的电容值

    c2[0] = 10.0; // pF
    c2[1] = 10.0; // pF
    c2[2] = 10.0; // pF
    c2[3] = 10.0; // pF

    // 当前硬件测量模型: 传感器与外部设置电容串联，再与引脚寄生电容并联,如下
    //                C1     C2
    //    CDC --------||----||----------- GND
    //             |            |
    //             |-----||-----|
    //                   Cx

    // 参考电容大小为5pF, ADC 为23bit
    //
    // 引脚寄生电容值每个芯片，每个通道都不通
    // 寄生电容获取方式：将传感器短路，然后获取cx2的值为c2与cxx的并联值

    // 定义寄生电容数组
    float *par_cap[2][4] = {
        {par1_0_cap, par1_1_cap, par1_2_cap, par1_3_cap},  // I2C1的寄生电容
        {par2_0_cap, par2_1_cap, par2_2_cap, par2_3_cap}   // I2C2的寄生电容
    };

    // 确定使用哪个I2C和地址
    int i2c_index = (hi2c == &hi2c1) ? 0 : 1;
    int addr_index;
    switch(rm1002_addr) {
        case DEFAULE_I2C_ADDR_0: addr_index = 0; break;
        case DEFAULE_I2C_ADDR_1: addr_index = 1; break;
        case DEFAULE_I2C_ADDR_2: addr_index = 2; break;
        case DEFAULE_I2C_ADDR_3: addr_index = 3; break;
        default: addr_index = 0; break;
    }

    // 计算寄生电容值
    for(int i = 0; i < 4; i++) {
        cxx[i] = par_cap[i2c_index][addr_index][i] - c2[i];
    }

    rawdata[0] = sign24To32(rm1002_adc_data_get(hi2c, RAW_DATA, 0, rm1002_addr));
    rawdata[1] = sign24To32(rm1002_adc_data_get(hi2c, RAW_DATA, 1, rm1002_addr));
    rawdata[2] = sign24To32(rm1002_adc_data_get(hi2c, RAW_DATA, 2, rm1002_addr));
    rawdata[3] = sign24To32(rm1002_adc_data_get(hi2c, RAW_DATA, 3, rm1002_addr));

#if 1
    // 动态阈值
    uint8_t cal_data[] = {0x1f, 0x2f, 0x4f, 0x8f};  // 各通道的校准值
    uint8_t cal_mask[] = {0x01, 0x02, 0x04, 0x08};  // 各通道的掩码
    
    for(int i = 0; i < 4; i++) {
        if(rawdata[i] > 8300000 || rawdata[i] < 20000) {
            rm1002_reg_write(hi2c, 0x2f, &cal_data[i], 1, rm1002_addr);
            
            uint8_t status = 0;
            while(!(status & cal_mask[i])) {
                rm1002_reg_read(hi2c, 0x73, &status, 1, rm1002_addr);
            }
        }
    }
#endif

    rm1002_reg_read(hi2c, 0x41, cxoff, 4, rm1002_addr);
#if 1
    for (i = 0; i < 4; i++)
    {
        coff[i] = (256.0 - cxoff[i]) * 0.2; // 修改coff的值增大量程
        cx0[i] = rawdata[i] / 1677721.6;
        cx0[i] = cx0[i] + coff[i];
        cx2[i] = cx0[i];
        cx0[i] = cx0[i] - cxx[i];
        c1[i] = (c2[i] * cx0[i]) / (c2[i] - cx0[i]);
    }
#endif

#if FLEX_FLAG
    if (hi2c == &hi2c1)
    {
        if (rm1002_addr == DEFAULE_I2C_ADDR_0)
        {
            printf("float par1_0_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
        else if (rm1002_addr == DEFAULE_I2C_ADDR_1)
        {
            printf("float par1_1_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
        else if (rm1002_addr == DEFAULE_I2C_ADDR_2)
        {
            printf("float par1_2_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
        else if (rm1002_addr == DEFAULE_I2C_ADDR_3)
        {
            printf("float par1_3_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
    }
    else if (hi2c == &hi2c2)
    {
        if (rm1002_addr == DEFAULE_I2C_ADDR_0)
        {
            printf("float par2_0_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
        else if (rm1002_addr == DEFAULE_I2C_ADDR_1)
        {
            printf("float par2_1_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
        else if (rm1002_addr == DEFAULE_I2C_ADDR_2)
        {
            printf("float par2_2_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
        else if (rm1002_addr == DEFAULE_I2C_ADDR_3)
        {
            printf("float par2_3_cap[4] = {%f, %f ,%f, %f};\r\n", cx2[0], cx2[1], cx2[2], cx2[3]);
        }
    }
#endif

    return c1;
}

