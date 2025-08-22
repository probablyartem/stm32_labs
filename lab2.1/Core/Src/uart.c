#include "uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Ссылка на UART_HandleTypeDef
// Допустим, используем USART2.
static UART_HandleTypeDef huart6;

// Флаг: 0 = polling, 1 = interrupt
static uint8_t isInterruptMode = 0;

/* -----------------------------------
   Буфер для приёма по прерыванию
   -----------------------------------*/
#define RX_BUFFER_SIZE 128
static uint8_t rxBuf[RX_BUFFER_SIZE];
static volatile uint16_t rxHead = 0;
static volatile uint16_t rxTail = 0;
// Промежуточный байт, который HAL заполняет
static uint8_t rxTempByte = 0;

/* -----------------------------------
   Локальный парсер команд:
   будем накапливать строку до \r или \n
   -----------------------------------*/
static char cmdBuf[64];
static uint8_t cmdIdx = 0;

// Функция-обработчик готовой команды
static void APP_UART_ProcessCommand(const char *cmd);

// Внешние зависимости (задействуем светофор)
#include "traffic_light.h"

/* -----------------------------------
   ИНИЦИАЛИЗАЦИЯ UART (57600 8N1)
   -----------------------------------*/
void APP_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    // Настраиваем huart2
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 57600;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart6) != HAL_OK) {
        // handle error
        while (1) { }
    }

    // По умолчанию в polling-режиме
    isInterruptMode = 0;
}

/* -----------------------------------
   Включение/выключение interrupt-режима
   -----------------------------------*/
void APP_UART_SetInterruptMode(uint8_t onOff)
{
    if (onOff) {
        // Включаем прерывания
        rxHead = rxTail = 0;
        isInterruptMode = 1;
        // Запускаем приём
        HAL_UART_Receive_IT(&huart6, &rxTempByte, 1);
    } else {
        // Выключаем
        isInterruptMode = 0;
        // Можно отключить NVIC или просто игнорировать
        // Для простоты — просто перестаём вызывать Receive_IT
    }
}

uint8_t APP_UART_IsInterruptMode(void)
{
    return isInterruptMode;
}

/* -----------------------------------
   Процесс в polling-режиме:
   - пытаемся считать 1 байт (timeout=0)
   - если есть байт, эхо + парсим
   -----------------------------------*/
void APP_UART_ProcessPolling(void)
{
    if (!isInterruptMode) {
        uint8_t ch;
        if (HAL_UART_Receive(&huart6, &ch, 1, 0) == HAL_OK) {
            // Эхо
            HAL_UART_Transmit(&huart6, &ch, 1, 10);
            // Собираем в cmdBuf
            if (ch == '\r' || ch == '\n') {
                cmdBuf[cmdIdx] = 0;
                if (cmdIdx > 0) {
                    APP_UART_ProcessCommand(cmdBuf);
                }
                const char *nl = "\r\n";
                HAL_UART_Transmit(&huart6, (uint8_t*)nl, 2, 10);
                cmdIdx = 0;
            } else {
                if (cmdIdx < sizeof(cmdBuf)-1) {
                    cmdBuf[cmdIdx++] = (char)ch;
                }
            }
        }
    }
}

/* -----------------------------------
   IRQ Handler (в main.c -> USART2_IRQHandler -> сюда)
   -----------------------------------*/
void APP_UART_IRQ_Handler(void)
{
    HAL_UART_IRQHandler(&huart6);
}

/* -----------------------------------
   Callback при приёме байта (RxCplt)
   HAL сам вызовет из HAL_UART_IRQHandler
   -----------------------------------*/
void APP_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart6 && isInterruptMode) {
        // Пишем в кольцевой буфер
        uint16_t nextHead = (rxHead + 1) % RX_BUFFER_SIZE;
        if (nextHead != rxTail) {
            rxBuf[rxHead] = rxTempByte;
            rxHead = nextHead;
        }
        // Перезапуск приёма
        HAL_UART_Receive_IT(&huart6, &rxTempByte, 1);
    }
}

/* -----------------------------------
   Обработка в interrupt-режиме:
   - выгребаем все байты из rxBuf
   - эхо + парсер
   -----------------------------------*/
void APP_UART_ProcessInterrupt(void)
{
    if (isInterruptMode) {
        while (rxTail != rxHead) {
            uint8_t ch = rxBuf[rxTail];
            rxTail = (rxTail + 1) % RX_BUFFER_SIZE;

            // Эхо
            HAL_UART_Transmit(&huart6, &ch, 1, 10);

            if (ch == '\r' || ch == '\n') {
                cmdBuf[cmdIdx] = 0;
                if (cmdIdx > 0) {
                    APP_UART_ProcessCommand(cmdBuf);
                }
                const char *nl = "\r\n";
                HAL_UART_Transmit(&huart6, (uint8_t*)nl, 2, 10);
                cmdIdx = 0;
            } else {
                if (cmdIdx < sizeof(cmdBuf)-1) {
                    cmdBuf[cmdIdx++] = (char)ch;
                }
            }
        }
    }
}

/* -----------------------------------
   Вспомогательные ф-ции для вывода
   -----------------------------------*/
void APP_UART_SendString(const char *s)
{
    HAL_UART_Transmit(&huart6, (uint8_t*)s, strlen(s), 100);
}

void APP_UART_SendOK(void)
{
    APP_UART_SendString("OK\r\n");
}

void APP_UART_SendUnknown(void)
{
    APP_UART_SendString("unknown command\r\n");
}

/* -----------------------------------
   Разбор команд:
   ?
   set mode 1 / set mode 2
   set timeout X
   set interrupts on/off
   -----------------------------------*/
static void APP_UART_ProcessCommand(const char *cmd)
{
    // Команда "?"
    if (strcmp(cmd, "?") == 0) {
        char buf[128];
        const char *stStr = TrafficLight_GetStateString();
        uint8_t mode = TrafficLight_GetMode();
        uint32_t tout = TrafficLight_GetRedTimeoutSec();
        char cIntr = (isInterruptMode ? 'I' : 'P');
        // Пример: "Current: red, mode=1, timeout=4, I"
        sprintf(buf, "Current: %s, mode=%u, timeout=%lu, %c\r\n", stStr, mode, (unsigned long)tout, cIntr);
        APP_UART_SendString(buf);
        return;
    }

    // "set mode 1" / "set mode 2"
    if (strncmp(cmd, "set mode ", 9) == 0) {
        int m = atoi(&cmd[9]);
        if (m == 1 || m == 2) {
            TrafficLight_SetMode((uint8_t)m);
            APP_UART_SendOK();
        } else {
            APP_UART_SendUnknown();
        }
        return;
    }

    // "set timeout X"
    if (strncmp(cmd, "set timeout ", 12) == 0) {
        int t = atoi(&cmd[12]);
        if (t > 0 && t < 3600) {
            TrafficLight_SetRedTimeoutSec((uint32_t)t);
            APP_UART_SendOK();
        } else {
            APP_UART_SendUnknown();
        }
        return;
    }

    // "set interrupts on/off"
    if (strncmp(cmd, "set interrupts ", 15) == 0) {
        const char *param = &cmd[15];
        if (strcmp(param, "on") == 0) {
            APP_UART_SetInterruptMode(1);
            APP_UART_SendOK();
            return;
        } else if (strcmp(param, "off") == 0) {
            APP_UART_SetInterruptMode(0);
            APP_UART_SendOK();
            return;
        } else {
            APP_UART_SendUnknown();
            return;
        }
    }

    // иначе
    APP_UART_SendUnknown();
}
