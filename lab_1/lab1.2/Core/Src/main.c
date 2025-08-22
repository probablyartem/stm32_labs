/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_RED = 0,
    STATE_GREEN,
    STATE_GREEN_BLINK,
    STATE_YELLOW
} TrafficState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define T_RED_FULL         4000 // мс (красный - полный)
#define T_RED_QUARTER      (T_RED_FULL / 4)
#define T_GREEN            1000 // мс (зелёный)
#define T_GREEN_BLINK      1000 // мс (время, за которое зелёный "моргает")
#define T_GREEN_BLINK_HALF 250  // мс (период переключения ON/OFF при мигании)
#define T_YELLOW           1000 // мс (жёлтый)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */


static TrafficState_t currentState = STATE_RED;
static uint32_t timeStateEntered   = 0; // Время входа в текущее состояние
static uint32_t currentRedDuration = T_RED_FULL;
static uint8_t  nextRedShortened   = 0; // Флаг "укоротить следующий красный"

static uint8_t  greenBlinkLedOn    = 0;
static uint32_t greenBlinkLastTgl  = 0; // последнее переключение для мигания
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void TrafficLight_Init(void)
{
    currentState = STATE_RED;
    timeStateEntered = HAL_GetTick();
    currentRedDuration = T_RED_FULL;
    nextRedShortened = 0;

    turnOffAllLights();
    turnOnLight(RED); // включим красный
    greenBlinkLedOn = 0;
    greenBlinkLastTgl = 0;
}

void TrafficLight_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - timeStateEntered;

    // Проверим, нет ли нового нажатия кнопки
    if (Button_WasPressed()) {
        // Если кнопка нажата в красном
        if (currentState == STATE_RED) {
            if (elapsed > T_RED_QUARTER) {
                // Если уже горим красным дольше 1/4
                // — сразу переходим к зелёному
                nextRedShortened = 0; // сбрасываем флаг на будущее
                turnOffAllLights();
                turnOnLight(GREEN);
                currentState = STATE_GREEN;
                timeStateEntered = now;
            } else {
                // Иначе помечаем, что следующий красный будет укорочен
                nextRedShortened = 1;
            }
        }
        // Если кнопка нажата во время зелёного / мигающего / жёлтого
        else {
            // Просто запоминаем флаг для будущего сокращения красного
            nextRedShortened = 1;
        }
    }

    switch (currentState) {
    case STATE_RED:
        if (elapsed >= currentRedDuration) {
            // Переход на зелёный
            turnOffAllLights();
            turnOnLight(GREEN);
            currentState = STATE_GREEN;
            timeStateEntered = now;
        }
        break;

    case STATE_GREEN:
        if (elapsed >= T_GREEN) {
            // Переход на мигающий зелёный
            turnOffAllLights();
            currentState = STATE_GREEN_BLINK;
            timeStateEntered = now;
            greenBlinkLedOn = 0;
            greenBlinkLastTgl = now;
        }
        break;

    case STATE_GREEN_BLINK:
        // Каждые T_GREEN_BLINK_HALF мс переключаем светодиод
        if ((now - greenBlinkLastTgl) >= T_GREEN_BLINK_HALF) {
            greenBlinkLastTgl = now;
            greenBlinkLedOn = !greenBlinkLedOn;
            setLight(GREEN, greenBlinkLedOn);
        }
        // Проверим, не завершилось ли мигание
        if (elapsed >= T_GREEN_BLINK) {
            // Переходим на жёлтый
            turnOffAllLights();
            turnOnLight(YELLOW);
            currentState = STATE_YELLOW;
            timeStateEntered = now;
        }
        break;

    case STATE_YELLOW:
        if (elapsed >= T_YELLOW) {
            // Возвращаемся в красный
            turnOffAllLights();
            currentState = STATE_RED;
            timeStateEntered = now;

            // Смотрим, надо ли укоротить красный
            if (nextRedShortened) {
                currentRedDuration = T_RED_QUARTER;
                nextRedShortened = 0;
            } else {
                currentRedDuration = T_RED_FULL;
            }
            turnOnLight(RED);
        }
        break;

    default:
        break;
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  TrafficLight_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  Button_Update();
	  TrafficLight_Update();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
