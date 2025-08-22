/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */
static uint8_t buttonWasPressed = 0;   // флаг "было нажатие"
static uint32_t buttonLastChangeTick = 0;
static GPIO_PinState lastRawState = GPIO_PIN_SET; // заведомо "отпущено", если контакт подтянут
static uint8_t stableState = 0; // 0 - отпущено, 1 - нажато (стабильное)

#define DEBOUNCE_DELAY_MS 50

void Button_Update(void)
{
    GPIO_PinState raw = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);

    if (raw != lastRawState) {
        // состояние пина поменялось
        lastRawState = raw;
        buttonLastChangeTick = HAL_GetTick(); // ждём подтверждения дребезга
    } else {
        // если пин стабильно в новом состоянии не меньше DEBOUNCE_DELAY_MS
        if ((HAL_GetTick() - buttonLastChangeTick) >= DEBOUNCE_DELAY_MS) {
            uint8_t newStable = (raw == GPIO_PIN_RESET) ? 1 : 0;
            // Предположим, что "нажатая кнопка" = GPIO_PIN_RESET (т.к. внутренний pull-up)
            if (newStable != stableState) {
                stableState = newStable;
                if (stableState == 1) {
                    // Переход в "нажато" => фиксируем событие
                    buttonWasPressed = 1;
                }
            }
        }
    }
}

// Возвращает "1", если произошло новое (однократное) нажатие кнопки с учётом дебаунса
uint8_t Button_WasPressed(void)
{
    if (buttonWasPressed) {
        buttonWasPressed = 0;
        return 1;
    }
    return 0;
}

uint32_t getPinByColor(LightColors color) {
    switch (color) {
        case GREEN:
            return GPIO_PIN_13;
        case YELLOW:
            return GPIO_PIN_14;
        case RED:
            return GPIO_PIN_15;
        default:
            return 0;
    }
}

uint8_t isLight(LightColors color){
    GPIO_PinState pinState = HAL_GPIO_ReadPin(GPIOD, getPinByColor(color));
    return (pinState == GPIO_PIN_SET) ? 1 : 0;
}

void turnOffAllLights(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);
}

void turnOnLight(LightColors color)
{
    turnOffAllLights();
    HAL_GPIO_WritePin(GPIOD, getPinByColor(color), GPIO_PIN_SET);
}

void setLight(LightColors color, uint8_t onOff)
{
    // Включить или выключить указанный цвет, не затрагивая остальные
    if (onOff) {
        HAL_GPIO_WritePin(GPIOD, getPinByColor(color), GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOD, getPinByColor(color), GPIO_PIN_RESET);
    }
}


/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
