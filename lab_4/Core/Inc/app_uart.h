/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart6;
void APP_UART_Init(void);
/* Вызывается в главном цикле,
   выгружает кольцевой буфер
   и обрабатывает принятые символы. */
void APP_UART_Process(void);

/* Отправка строки */
void APP_UART_SendString(const char *str);

/* IRQ Handler */
void APP_UART_IRQHandler(void);

/* Rx Complete Callback */
void APP_UART_RxCpltCallback(UART_HandleTypeDef *huart);
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART6_UART_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */
