#include "pca9538.h"
#include "i2c.h"
#include "stm32f4xx_hal.h"

// регистры PCA9538
#define REG_INPUT     0x00
#define REG_OUTPUT    0x01
#define REG_POLARITY  0x02
#define REG_CONFIG    0x03

extern I2C_HandleTypeDef hi2c1;

void PCA9538_Init(void)
{
    // Можно, например, сбросить PolarityReg=0
    // и всё входы (Config=0xFF), Output=0xFF
    uint8_t tmp = 0;
    tmp = 0x00; // Polarity
    HAL_I2C_Mem_Write(&hi2c1, PCA9538_I2C_ADDR, REG_POLARITY, 1, &tmp, 1, 100);

    tmp = 0xFF; // всё входы
    HAL_I2C_Mem_Write(&hi2c1, PCA9538_I2C_ADDR, REG_CONFIG, 1, &tmp, 1, 100);

    tmp = 0xFF; // output=1
    HAL_I2C_Mem_Write(&hi2c1, PCA9538_I2C_ADDR, REG_OUTPUT, 1, &tmp, 1, 100);
}

void PCA9538_WriteConfigReg(uint8_t config)
{
    HAL_I2C_Mem_Write(&hi2c1, PCA9538_I2C_ADDR, REG_CONFIG, 1, &config, 1, 100);
}
void PCA9538_WriteOutputReg(uint8_t output)
{
    HAL_I2C_Mem_Write(&hi2c1, PCA9538_I2C_ADDR, REG_OUTPUT, 1, &output, 1, 100);
}
uint8_t PCA9538_ReadInputReg(void)
{
    uint8_t val = 0;
    HAL_I2C_Mem_Read(&hi2c1, PCA9538_I2C_ADDR, REG_INPUT, 1, &val, 1, 100);
    return val;
}
