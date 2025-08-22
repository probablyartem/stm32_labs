/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include <app_uart.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "beep.h"
/* USER CODE BEGIN 0 */
#define RX_BUF_SIZE 128
static uint8_t rxBuf[RX_BUF_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;

static uint8_t rxTemp;

static void ProcessChar(uint8_t ch);
static void SendUnknownChar(uint8_t ch);

/* USER CODE END 0 */

UART_HandleTypeDef huart6;

/* USART6 init function */

void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */
  __HAL_RCC_USART6_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 57600;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_UART_Receive_IT(&huart6, &rxTemp, 1);
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART6)
  {
  /* USER CODE BEGIN USART6_MspInit 0 */

  /* USER CODE END USART6_MspInit 0 */
    /* USART6 clock enable */
    __HAL_RCC_USART6_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART6 interrupt Init */
    HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
  /* USER CODE BEGIN USART6_MspInit 1 */

  /* USER CODE END USART6_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART6)
  {
  /* USER CODE BEGIN USART6_MspDeInit 0 */

  /* USER CODE END USART6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART6_CLK_DISABLE();

    /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);

    /* USART6 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART6_IRQn);
  /* USER CODE BEGIN USART6_MspDeInit 1 */

  /* USER CODE END USART6_MspDeInit 1 */
  }
}

void APP_UART_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart6);
}

void APP_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart6) {
        // Записываем символ в кольцевой буфер
        uint16_t nextHead = (rxHead + 1) % RX_BUF_SIZE;
        if (nextHead != rxTail) {
            rxBuf[rxHead] = rxTemp;
            rxHead = nextHead;
        }
        // Перезапуск
        HAL_UART_Receive_IT(&huart6, &rxTemp, 1);
    }
}

void APP_UART_Process(void)
{
    while (rxTail != rxHead) {
        uint8_t ch = rxBuf[rxTail];
        rxTail = (rxTail + 1) % RX_BUF_SIZE;

        // Обрабатываем
        // 1) эхо
        HAL_UART_Transmit(&huart6, &ch, 1, 10);
        // 2) вызываем ProcessChar
        ProcessChar(ch);
    }
}
void APP_UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart6, (uint8_t*)str, strlen(str), 100);
}
static void ProcessChar(uint8_t ch)
{
    // Управляем глобальными переменными beep: octave, duration, и т.д.
    // См. beep.c
    switch (ch)
    {
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7':
        {
            // Сыграем одиночную ноту
            // Определим индекс ноты (0=до,1=ре,...6=си)
            uint8_t noteIndex = (uint8_t)(ch - '1'); // '1'->0, '2'->1,...
            //float freq = BEEP_GetNoteFrequency(noteIndex);
            // Запустим воспроизведение
            BEEP_PlayNoteOnce(noteIndex);
            // Выводим сообщение
            char msg[64];
            snprintf(msg, sizeof(msg), " -> Note %s (octave=%u) for %.1f s\r\n",
                     BEEP_GetNoteName(noteIndex),
                     BEEP_GetOctave(),
                     BEEP_GetDuration());
            APP_UART_SendString(msg);
        }
        break;

    case '+':
        {
            BEEP_IncreaseOctave();
            char msg[64];
            snprintf(msg, sizeof(msg), " -> New octave=%u\r\n", BEEP_GetOctave());
            APP_UART_SendString(msg);
        }
        break;

    case '-':
        {
            BEEP_DecreaseOctave();
            char msg[64];
            snprintf(msg, sizeof(msg), " -> New octave=%u\r\n", BEEP_GetOctave());
            APP_UART_SendString(msg);
        }
        break;

    case 'A': // Увеличение длительности
        {
            BEEP_ChangeDuration(+0.1f);
            char msg[64];
            snprintf(msg, sizeof(msg), " -> duration=%.1f s\r\n", BEEP_GetDuration());
            APP_UART_SendString(msg);
        }
        break;

    case 'a': // Уменьшение длительности
        {
            BEEP_ChangeDuration(-0.1f);
            char msg[64];
            snprintf(msg, sizeof(msg), " -> duration=%.1f s\r\n", BEEP_GetDuration());
            APP_UART_SendString(msg);
        }
        break;

    case '\n':
        // Игнорируем просто перевод строки
        break;

    case 13: // Enter (обычно \r), воспроизвести подряд все 7 нот
        {
            BEEP_PlayAllNotesInOctave();
            char msg[64];
            snprintf(msg, sizeof(msg), " -> Play all 7 notes (octave=%u) each %.1f s\r\n",
                     BEEP_GetOctave(), BEEP_GetDuration());
            APP_UART_SendString(msg);
        }
        break;

    default:
        // Неверный символ
        // Выводим код
        SendUnknownChar(ch);
        break;
    }
}
static void SendUnknownChar(uint8_t ch)
{
    char msg[64];
    snprintf(msg, sizeof(msg), " -> Неверный символ '%c' (code=%u)\r\n", ch, ch);
    APP_UART_SendString(msg);
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
