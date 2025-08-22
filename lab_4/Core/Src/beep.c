/*
 * beep.c

 */
#include "beep.h"
#include "tim.h"     // чтобы пользоваться htim3
#include <math.h>    // для powf

// -----------------------------
// Глобальные настройки
// -----------------------------
static uint8_t currentOctave = 1;      // [1..5], минимум=1 (субконтроктава), максимум=5 (5-я)
static float   noteDuration = 1.0f;    // длительность (с), от 0.1 до 5.0

// -----------------------------
// Для "одноразового" воспроизведения ноты:
// мы будем хранить endTimeMs = HAL_GetTick() + durationMs
// когда now >= endTimeMs, выключаем
// -----------------------------
static uint8_t beepActive = 0;
static uint32_t beepEndTimeMs = 0;

// -----------------------------
// Для "последовательного" воспроизведения Enter:
// хранить флаг и индекс текущей ноты [0..6],
// когда нота закончилась -> переходим к следующей
// -----------------------------
static uint8_t playSequence = 0;
static uint8_t sequenceIndex = 0;
static const uint8_t SEQUENCE_LEN = 7; // до..си

// -----------------------------
// Имена нот (7 штук)
// -----------------------------
static const char* noteNames[7] = {
    "Do", "Re", "Mi", "Fa", "Sol", "La", "Si"
};

// Частоты первой октавы (по заданию 5-я по порядку), Гц
// (Соответствие: до=261.63, ре=293.67, ми=329.63, фа=349.23, соль=392.0, ля=440.0, си=493.88)
static const float baseFreq[7] = {
    261.63f, 293.67f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f
};

/*
 * Вычислить частоту ноты noteIndex (0..6)
 * в текущей октаве [1..5].
 * Формула:
 * freq = baseFreq[noteIndex] * 2^(octave - 5)
 * т.к. "первая октава" здесь соответствует octave=5 в классической нотации.
 */
static float calcNoteFreq(uint8_t noteIndex, uint8_t octave)
{
    // octave=5 => множитель 2^0 = 1
    // octave=4 => 2^-1 = 0.5
    // octave=1 => 2^-4 = 1/16
    float mult = powf(2.0f, (float)(octave - 5));
    return baseFreq[noteIndex] * mult;
}

// ========== Функции управления PWM (TIM3_CH1) ==========

static void startPWM(void)
{
    // Запускаем PWM на канале 1
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

static void stopPWM(void)
{
    // Останавливаем
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

/*
 * Установка частоты PWM (freq Гц).
 * На входе freq может быть от ~16 Гц до нескольких кГц (зависит от задачи).
 * Период таймера = (PSC+1)*(ARR+1)/TimerCLK
 *
 * При тактировании APB1=60 MHz => если TIM3 на APB1, то TimerCLK=120 MHz
 * (для F4 в случае x2 multiplier).
 *
 * Для удобства:
 *   - оставим PSC = 0
 *   - ARR = TimerCLK / freq - 1
 * DutyCycle=50% => CCR1 = ARR/2
 */
void BEEP_SetFrequency(float freq)
{
    if (freq < 1.0f) freq = 1.0f;

    // Тактирование таймера (TIM3) = 120 MHz (при APB1 x2)
    float timerClock = 120e6f;
    uint32_t period = (uint32_t)((timerClock / freq) + 0.5f) - 1;
    if (period < 100) period = 100;     // ограничимся, чтобы не было слишком высокого звука
    if (period > 0xFFFF) period = 0xFFFF; // нельзя превысить 16 бит

    // Обновляем регистры
    htim1.Instance->PSC = 0;
    htim1.Instance->ARR = period;
    // 50% duty:
    htim1.Instance->CCR1 = period/2;
    htim1.Instance->EGR = TIM_EGR_UG; // принудительно обновим
}

// ========== Реализация интерфейса из beep.h ==========

void BEEP_Init(void)
{
    // Предполагаем, что TIM3 уже проинициализирован в MX_TIM_Init().
    // Сразу можем выключить, на всякий случай
    stopPWM();
    beepActive = 0;
    playSequence = 0;
    currentOctave = 1;
    noteDuration = 1.0f;
}

/*
 * Вызываем из main() постоянно.
 * Проверяем, не пора ли остановить текущую ноту,
 * либо перейти к следующей (если playSequence=1).
 */
void BEEP_Update(void)
{
    uint32_t now = HAL_GetTick();
    if (beepActive)
    {
        if (now >= beepEndTimeMs)
        {
            // Конец ноты
            BEEP_Off();
            beepActive = 0;

            // Если была последовательность, переходим к следующей
            if (playSequence)
            {
                sequenceIndex++;
                if (sequenceIndex < SEQUENCE_LEN)
                {
                    // играем следующую
                    BEEP_PlayNoteOnce(sequenceIndex);
                }
                else
                {
                    // Всё
                    playSequence = 0;
                    sequenceIndex = 0;
                }
            }
        }
    }
}

// Включить PWM
void BEEP_On(void)
{
    startPWM();
}

// Выключить PWM
void BEEP_Off(void)
{
    stopPWM();
}

/* Изменение длительности */
void BEEP_ChangeDuration(float dSec)
{
    float newDur = noteDuration + dSec;
    if (newDur < 0.1f) newDur = 0.1f;
    if (newDur > 5.0f) newDur = 5.0f;
    noteDuration = newDur;
}

float BEEP_GetDuration(void)
{
    return noteDuration;
}

void BEEP_IncreaseOctave(void)
{
    if (currentOctave < 5) currentOctave++;
}

void BEEP_DecreaseOctave(void)
{
    if (currentOctave > 1) currentOctave--;
}

uint8_t BEEP_GetOctave(void)
{
    return currentOctave;
}

float BEEP_GetNoteFrequency(uint8_t noteIndex)
{
    if (noteIndex > 6) noteIndex = 6;
    return calcNoteFreq(noteIndex, currentOctave);
}

const char* BEEP_GetNoteName(uint8_t noteIndex)
{
    if (noteIndex > 6) noteIndex = 6;
    return noteNames[noteIndex];
}

/*
 * Проиграть одиночную ноту (noteIndex) - без блокировок.
 * Запускаем PWM на нужной freq, запоминаем время окончания.
 */
void BEEP_PlayNoteOnce(uint8_t noteIndex)
{
    // Остановим, если что-то уже звучит
    BEEP_Off();
    beepActive = 1;

    float freq = calcNoteFreq(noteIndex, currentOctave);
    BEEP_SetFrequency(freq);
    BEEP_On();

    uint32_t durMs = (uint32_t)(noteDuration * 1000.0f + 0.5f);
    beepEndTimeMs = HAL_GetTick() + durMs;
}

/*
 * Запускает воспроизведение всех нот 0..6 (до..си) подряд
 * текущей октавы, каждая noteDuration.
 * Запускаем первую ноту, а дальше BEEP_Update() само переключит.
 */
void BEEP_PlayAllNotesInOctave(void)
{
    // Сбросим, если уже что-то играется
    beepActive = 0;
    playSequence = 1;
    sequenceIndex = 0;

    // Запускаем первую
    BEEP_PlayNoteOnce(0);
}

void BEEP_HandleKey(uint8_t key)
{
    switch (key)
    {
    case 1: // "до"
        BEEP_PlayNoteOnce(0);
        break;
    case 2: // "ре"
        BEEP_PlayNoteOnce(1);
        break;
    case 3: // "ми"
        BEEP_PlayNoteOnce(2);
        break;
    case 4: // "фа"
        BEEP_PlayNoteOnce(3);
        break;
    case 5: // "соль"
        BEEP_PlayNoteOnce(4);
        break;
    case 6: // "ля"
        BEEP_PlayNoteOnce(5);
        break;
    case 7: // "си"
        BEEP_PlayNoteOnce(6);
        break;
    case 8: // octave++
        BEEP_IncreaseOctave();
        break;
    case 9: // octave--
        BEEP_DecreaseOctave();
        break;
    case 10: // duration++
        BEEP_ChangeDuration(+0.1f);
        break;
    case 11: // duration--
        BEEP_ChangeDuration(-0.1f);
        break;
    case 12: // playAll
        BEEP_PlayAllNotesInOctave();
        break;
    default:
        // ignore
        break;
    }
}
