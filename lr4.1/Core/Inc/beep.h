/*
 * beep.h
 *

 */



#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/*
 * Инициализация модуля BEEP:
 * - Подготовка TIM3 (уже инициализирован в tim.c, просто запускаем PWM).
 */
void BEEP_Init(void);
void BEEP_HandleKey(uint8_t key);
/*
 * Вызывать из main-цикла часто (например, 1 раз за итерацию).
 * Проверяет, не пора ли остановить одиночную ноту
 * или перейти к следующей ноте в последовательном проигрывании.
 */
void BEEP_Update(void);

/* Установить/получить текущую октаву [1..5], по заданию */
void BEEP_IncreaseOctave(void);
void BEEP_DecreaseOctave(void);
uint8_t BEEP_GetOctave(void);

/*
 * Установить частоту PWM (в Гц).
 * Внутренне подбирает TIM3->PSC и TIM3->ARR.
 */
void BEEP_SetFrequency(float freq);

/*
 * Включить/выключить ШИМ (звук).
 */
void BEEP_On(void);
void BEEP_Off(void);

/* Текущая длительность одной ноты, сек (float). */
float BEEP_GetDuration(void);

/* Изменение длительности на dSec (может быть + или -). */
void BEEP_ChangeDuration(float dSec);

/*
 * Проиграть одиночную ноту [0..6] (0=до,1=ре,...6=си)
 * в текущей октаве, на текущую длительность.
 * Предыдущая нота сбрасывается.
 */
void BEEP_PlayNoteOnce(uint8_t noteIndex);

/*
 * Запустить воспроизведение "всех" нот 0..6
 * (до..си) подряд без пауз.
 */
void BEEP_PlayAllNotesInOctave(void);

/*
 * Функции, чтобы получить частоту ноты [0..6] в текущей октаве,
 * и строку названия ("C","D","E","F","G","A","B").
 */
float BEEP_GetNoteFrequency(uint8_t noteIndex);
const char* BEEP_GetNoteName(uint8_t noteIndex);

#endif // __BEEP_H

