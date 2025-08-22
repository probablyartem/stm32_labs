/*
 * traffic_light.c
 *
 *  Created on: Feb 21, 2025
 */
#include "traffic_light.h"

#define LED_RED_PORT	GPIOD
#define LED_RED_PIN		GPIO_PIN_15

#define LED_YELLOW_PORT	GPIOD
#define	LED_YELLOW_PIN	GPIO_PIN_14

#define LED_GREEN_PORT	GPIOD
#define	LED_GREEN_PIN	GPIO_PIN_13

#define BUTTON_PORT		GPIOC
#define	BUTTON_PIN		GPIO_PIN_15

/* --------------------------------------------
 * Локальные переменные
 * --------------------------------------------*/
static TrafficState_t currentState = STATE_RED;
static uint32_t timeStateEnteredMs = 0;  // время входа в текущее состояние (мс SysTick)
static uint8_t trafficMode = 1;         // 1 = учитываем кнопку, 2 = игнорируем
static uint32_t redTimeoutSec = 4;      // по умолчанию 4 секунды красного
static uint32_t currentRedDurationMs = 4000;
static uint8_t nextRedShortened = 0;

static uint8_t greenBlinkLedOn = 0;
static uint32_t greenBlinkLastTglMs = 0;

/* --------------------------------------------
 * Дребезг кнопки
 * --------------------------------------------*/
#define DEBOUNCE_DELAY_MS 50
static uint8_t buttonWasPressed = 0;
static GPIO_PinState lastRawState = GPIO_PIN_SET; // считаем, что не нажато (pull-up)
static uint8_t stableState = 0; // 0=отпущено,1=нажато
static uint32_t buttonLastChangeTick = 0;

/* --------------------------------------------
 * Функции управления светодиодами
 * --------------------------------------------*/
static void LED_OffAll(void)
{
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
}

static void LED_SetRed(uint8_t onOff)
{
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, onOff ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void LED_SetYellow(uint8_t onOff)
{
    HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, onOff ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void LED_SetGreen(uint8_t onOff)
{
    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, onOff ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* --------------------------------------------
 * Инициализация светофора (GPIO + нач. сост)
 * --------------------------------------------*/
void TrafficLight_Init(void)
{
    // По идее, GPIO тактирование уже включено в MX_GPIO_Init()
    // Установим изначальное состояние
    currentState = STATE_RED;
    timeStateEnteredMs = HAL_GetTick();
    trafficMode = 1;
    redTimeoutSec = 4;
    currentRedDurationMs = redTimeoutSec * 1000;
    nextRedShortened = 0;

    LED_OffAll();
    LED_SetRed(1);
    greenBlinkLedOn = 0;
    greenBlinkLastTglMs = 0;

    // Инициализация кнопки (считаем, что уже сконфигурирована как input pull-up)
    buttonWasPressed = 0;
    lastRawState = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
    stableState = (lastRawState == GPIO_PIN_RESET) ? 1 : 0;
    buttonLastChangeTick = HAL_GetTick();
}

/* --------------------------------------------
 * Дребезг кнопки: вызывать в main() часто
 * --------------------------------------------*/
void Button_Update(void)
{
    GPIO_PinState raw = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN);
    if (raw != lastRawState) {
        lastRawState = raw;
        buttonLastChangeTick = HAL_GetTick();
    } else {
        if ((HAL_GetTick() - buttonLastChangeTick) >= DEBOUNCE_DELAY_MS) {
            uint8_t newStable = (raw == GPIO_PIN_RESET) ? 1 : 0;
            if (newStable != stableState) {
                stableState = newStable;
                // переход 0->1 = нажато
                if (stableState == 1) {
                    buttonWasPressed = 1;
                }
            }
        }
    }
}

// Однократно вернуть флаг нажатия
static uint8_t Button_WasPressed(void)
{
    if (buttonWasPressed) {
        buttonWasPressed = 0;
        return 1;
    }
    return 0;
}

/* --------------------------------------------
 * Главный цикл светофора: вызываем в main
 * --------------------------------------------*/
void TrafficLight_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - timeStateEnteredMs;

    // Проверяем кнопку, только если mode=1
    if (trafficMode == 1) {
        if (Button_WasPressed()) {
            // Логика: если мы в красном
            if (currentState == STATE_RED) {
                // Если уже горим больше 1/4
                if (elapsed > (currentRedDurationMs / 4)) {
                    // Сразу переходим на зелёный
                    nextRedShortened = 0;
                    LED_OffAll();
                    LED_SetGreen(1);
                    currentState = STATE_GREEN;
                    timeStateEnteredMs = now;
                } else {
                    // Запоминаем, что следующий красный будет короче
                    nextRedShortened = 1;
                }
            }
            // Если нажато в зелёном/мигающем/жёлтом —
            // тогда просто nextRedShortened = 1
            else {
                nextRedShortened = 1;
            }
        }
    }

    // Переходы по таймерам
    switch (currentState) {
    case STATE_RED:
        if (elapsed >= currentRedDurationMs) {
            // в зелёный
            LED_OffAll();
            LED_SetGreen(1);
            currentState = STATE_GREEN;
            timeStateEnteredMs = now;
        }
        break;
    case STATE_GREEN:
        // пусть зелёный горит 1 сек (просто пример)
        if (elapsed >= 1000) {
            // переходим на мигающий
            LED_OffAll();
            currentState = STATE_GREEN_BLINK;
            timeStateEnteredMs = now;
            greenBlinkLedOn = 0;
            greenBlinkLastTglMs = now;
        }
        break;
    case STATE_GREEN_BLINK:
        // мигаем 1 сек (частота 250мс), потом желтый
        if ((now - greenBlinkLastTglMs) >= 250) {
            greenBlinkLastTglMs = now;
            greenBlinkLedOn = !greenBlinkLedOn;
            LED_SetGreen(greenBlinkLedOn);
        }
        if (elapsed >= 1000) {
            // желтый
            LED_OffAll();
            LED_SetYellow(1);
            currentState = STATE_YELLOW;
            timeStateEnteredMs = now;
        }
        break;
    case STATE_YELLOW:
        // пусть желтый горит 1 сек
        if (elapsed >= 1000) {
            // снова красный
            LED_OffAll();
            currentState = STATE_RED;
            timeStateEnteredMs = now;
            // Проверим флаг укороченного красного
            if (nextRedShortened) {
                currentRedDurationMs = (redTimeoutSec * 1000) / 4;
                nextRedShortened = 0;
            } else {
                currentRedDurationMs = redTimeoutSec * 1000;
            }
            LED_SetRed(1);
        }
        break;
    default:
        // fallback
        break;
    }
}

/* --------------------------------------------
 * Настройка "mode 1"/"mode 2"
 * --------------------------------------------*/
void TrafficLight_SetMode(uint8_t mode)
{
    // 1 или 2
    trafficMode = (mode == 2) ? 2 : 1;
}

uint8_t TrafficLight_GetMode(void)
{
    return trafficMode;
}

/* --------------------------------------------
 * Timeout (сколько секунд горит красный)
 * --------------------------------------------*/
void TrafficLight_SetRedTimeoutSec(uint32_t sec)
{
    if (sec < 1) sec = 1;
    if (sec > 3600) sec = 3600; // например, ограничим
    redTimeoutSec = sec;
    // при следующем заходе в красный учтётся
    // (или мгновенно, если сейчас красный?)
    // Для простоты, пусть вступает в силу при следующем полном цикле.
}

uint32_t TrafficLight_GetRedTimeoutSec(void)
{
    return redTimeoutSec;
}

/* --------------------------------------------
 * Текущее состояние
 * --------------------------------------------*/
TrafficState_t TrafficLight_GetState(void)
{
    return currentState;
}

/* --------------------------------------------
 * Строковое представление состояния
 * --------------------------------------------*/
const char* TrafficLight_GetStateString(void)
{
    switch (currentState) {
    case STATE_RED:          return "red";
    case STATE_GREEN:        return "green";
    case STATE_GREEN_BLINK:  return "blinking green";
    case STATE_YELLOW:       return "yellow";
    default:                 return "???";
    }
}


