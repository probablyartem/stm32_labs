#include "keypad.h"
#include "pca9538.h"
#include "stm32f4xx_hal.h"

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3

/* Карта кодов: row, col -> 1..12 */
static const uint8_t KeyMap[KEYPAD_ROWS][KEYPAD_COLS] = {
    { 1,  2,  3 },
    { 4,  5,  6 },
    { 7,  8,  9 },
    {10, 11, 12}
};

/* Текущее "стабильное" состояние: 0=нет нажатий, либо код */
static uint8_t stableKey = 0;
/* Храним "новонажатую" кнопку (edge) */
static uint8_t lastPressed = 0;

/* Для антидребезга: */
static uint8_t prevScanCode = 0;
static uint32_t lastScanTick = 0;
#define SCAN_INTERVAL_MS 10
#define DEBOUNCE_CYCLES  2  // 2 стабильных сканирования

static uint8_t consecutiveCount = 0;

/* -----------------------------------------
   Инициализация PCA9538:
   - Все линии = inputs,
   - За исключением момента, когда одну строку делаем output=0
   -----------------------------------------*/
void Keypad_Init(void)
{
    /* PCA9538 по умолчанию после сброса всё входы.
       Можно явно сконфигурировать,
       но иногда достаточно писать OutputReg = ...
       и ConfigReg = ...
    */
    PCA9538_Init(); // низкоуровневый (адрес, etc)
    stableKey = 0;
    lastPressed = 0;
    prevScanCode = 0;
    consecutiveCount = 0;
    lastScanTick = HAL_GetTick();
}

/* -----------------------------------------
   Читаем матрицу:
   Возвращаем "код" одной нажатой кнопки,
   или 0, если ничего/более одной/ошибка.
   -----------------------------------------*/
static uint8_t Keypad_ScanRaw(void)
{
    uint8_t foundRow = 255, foundCol = 255;
    uint8_t pressCount = 0;

    for (int row = 0; row < 4; row++) {
        // 1) Сформировать байт для OutputReg
        //    Выставим P_row=0 (output), остальные строки input (tri-state),
        //    столбцы тоже input.
        //    На PCA9538 есть 2 рег-ра:
        //      - Configuration (1=input, 0=output)
        //      - Output (valid только для тех, где config=0)
        //    Или можно "trюк": P_row=0 => config=0, output=0,
        //                      остальные rows => config=1 => input,
        //                      cols => config=1 => input
        //    P7 ? - пусть тоже input.

        uint8_t config = 0xFF;  // всё входы
        uint8_t output = 0xFF;  // всё "1"
        // row = i => P_i=0 => config[i] = 0, output[i] = 0
        config &= ~(1 << row);  // => bit row=0 => output mode
        output &= ~(1 << row);  // => bit row=0 => write 0

        PCA9538_WriteConfigReg(config);
        PCA9538_WriteOutputReg(output);

        HAL_Delay(1); // Микропауза ~1 ms (или можно без задержек, но иногда лучше подождать)

        // 2) Считать вход
        uint8_t inputByte = PCA9538_ReadInputReg();
        // col => P4,P5,P6
        // "нажатая" => col_j=0

        for (int col = 0; col < 3; col++) {
            uint8_t colBit = (4 + col);  // P4..P6
            uint8_t val = (inputByte >> colBit) & 0x1;
            if (val == 0) {
                // кнопка row,col нажата
                pressCount++;
                foundRow = row;
                foundCol = col;
                if (pressCount > 1) {
                    // более одной кнопки => возврат 0
                    return 0;
                }
            }
        }
    }

    if (pressCount == 1) {
        return KeyMap[foundRow][foundCol];
    } else {
        // Либо 0 кнопок, либо >1
        return 0;
    }
}

void Keypad_Update(void)
{
    uint32_t now = HAL_GetTick();
    if ((now - lastScanTick) < SCAN_INTERVAL_MS) {
        return; // сканируем раз в 10 мс (пример)
    }
    lastScanTick = now;

    // Сканируем
    uint8_t code = Keypad_ScanRaw();

    // Дебаунс: ждём, чтобы code совпадало N раз подряд
    if (code == prevScanCode && code != stableKey) {
        consecutiveCount++;
        if (consecutiveCount >= DEBOUNCE_CYCLES) {
            // Стабильное новое состояние
            stableKey = code;
            consecutiveCount = 0;
            if (stableKey != 0) {
                // событие: новая кнопка нажата
                lastPressed = stableKey;
            }
        }
    } else if (code != prevScanCode) {
        // сброс
        consecutiveCount = 1;
        prevScanCode = code;
    }
}

/* -----------------------------------------
   Возвращает код последней нажатой кнопки (1..12),
   или 0, если нет нового нажатия.
   Однократно.
-----------------------------------------*/
uint8_t Keypad_GetLastPressed(void)
{
    uint8_t tmp = lastPressed;
    lastPressed = 0;
    return tmp;
}
