#ifndef PCA9538_H
#define PCA9538_H

#include <stdint.h>

/*
 * И2С-адрес PCA9538. Может быть 0x70, 0x77 и т.д.,
 * зависит от адресных ножек.
 * Посмотрите схему стенда (A0..A2).
 */
#define PCA9538_I2C_ADDR   (0x71 << 1)  // пример: 0x70 + R/W-бит => 0xE0

void PCA9538_Init(void);

void PCA9538_WriteConfigReg(uint8_t config);
void PCA9538_WriteOutputReg(uint8_t output);
uint8_t PCA9538_ReadInputReg(void);

#endif
