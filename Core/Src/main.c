/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "hx711.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  float Tempo;
  float Factor_Calibration;
} Contexto;

typedef enum {
    EST_SETUP,
    EST_START,
    EST_TimerLedPeso,
    EST_CalcPrint_Peso,
} EstadoID;

typedef EstadoID Estadofunc(Contexto *ctx);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAMPLES 20
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
HX711_Handle_t hx;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t debounceDelay = 100;
uint32_t LastStartTime = 0;
uint32_t LastResetTime = 0;

bool StartTimer = false;

volatile uint8_t i = 0;

bool lastBtnStartState = true;
bool lastBtnResetState = true;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// -------** Habilita o uso do printf para enviar dados pelo UART **-------
// (Não estou considerando como requisito, mas sim para comunicar com o computador e utilizar o printf)
void _write(int file, char *ptr, int len) {
  // Transmit the data over UART
  HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
}

//----------**Declaração de funções**----------

//Função - Cronometro
float Cronometro(uint32_t startTime){
  static uint32_t NewTime = 0;
  static float FinalTime = 0.0f;
  
  if(startTime){
    if(NewTime == 0){
      NewTime = HAL_GetTick();
    }

    uint32_t currentTime = HAL_GetTick();
    FinalTime = (currentTime - NewTime) / 1000.0f;
  }else{
    NewTime = 0;
  }

   return FinalTime;
}

//Função - Debounce Btn
bool BtnDebounce(bool currentState, bool *stableState, uint32_t *lastChangeTime, uint32_t debounceDelay){
  uint32_t now = HAL_GetTick();

  if (currentState != *stableState) {
    if ((now - *lastChangeTime) >= debounceDelay) {
      *stableState = currentState;
      *lastChangeTime = now;

      if (currentState == false) {
        return true;
      }
    }
  }else{
    *lastChangeTime = now;
  }

  return false;
}

//Função - Leds

//----------**Máquina de estados**----------

EstadoID setup(Contexto *ctx){
  ctx->Tempo = 0.0f;
  ctx->Factor_Calibration = 447.6f;
  return EST_START;
}

EstadoID start(Contexto *ctx){
  bool btnPressed = (HAL_GPIO_ReadPin(Start_GPIO_Port, Start_Pin) != GPIO_PIN_RESET);
  
  //Start button
  if (BtnDebounce(btnPressed, &lastBtnStartState, &LastStartTime, debounceDelay)){
    return EST_TimerLedPeso;
  }

  HAL_GPIO_WritePin(LedProto_GPIO_Port, LedProto_Pin, GPIO_PIN_RESET);

  return EST_START;
}

EstadoID tlp(Contexto *ctx){
  static bool FistEntry = true;
  bool btnPressed = (HAL_GPIO_ReadPin(Reset_GPIO_Port, Reset_Pin) != GPIO_PIN_RESET);

  if(FistEntry){
    Cronometro(true);
    FistEntry = false;
    HX711_Tare(&hx, SAMPLES); //Tare Function
    printf("TARA REALIZADA - Iniciando contagem de tempo e leitura do peso.\r\n");
  }
  
  float weight = HX711_GetWeight(&hx, SAMPLES, ctx->Factor_Calibration);
  printf("Peso: %.2f g\r\n", weight);
  

  HAL_GPIO_WritePin(LedProto_GPIO_Port, LedProto_Pin, GPIO_PIN_SET);

  //Reset button
  if (BtnDebounce(btnPressed, &lastBtnResetState, &LastResetTime, debounceDelay)){
    printf("BOTÃO RESET PRESSIONADO - Resetando a máquina lógica.\r\n");
    printf("Tempo Final: %.2f s\r\n", Cronometro(true));
    Cronometro(false);

    FistEntry = true;
    
    return EST_START;
  }

  return EST_TimerLedPeso;
}

EstadoID cpp(Contexto *ctx){
  //bool btnPressed = (HAL_GPIO_ReadPin(Reset_GPIO_Port, Reset_Pin) != GPIO_PIN_RESET);

  //Reset button
  if( BtnDebounce(HAL_GPIO_ReadPin(Reset_GPIO_Port, Reset_Pin), &lastBtnResetState, &LastResetTime, debounceDelay)){
    printf("BOTÃO RESET PRESSIONADO - Resetando a máquina lógica.\r\n");
    return EST_START;
  }
  
  return EST_CalcPrint_Peso;
}

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  Contexto meuContexto = {0};
  EstadoID estadoAtual = EST_SETUP;

  Estadofunc *tabela_estados[] = {
    [EST_SETUP] = setup,
    [EST_START] = start,
    [EST_TimerLedPeso] = tlp,
    [EST_CalcPrint_Peso] = cpp
  };

  //Configurações do HX711  
  hx.dt_Port  = DT_GPIO_Port;
  hx.dt_Pin   = DT_Pin;
  hx.sck_Port = SCK_GPIO_Port;
  hx.sck_Pin  = SCK_Pin;
  hx.gain     = HX711_GAIN_128;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    estadoAtual = tabela_estados[estadoAtual](&meuContexto);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
#ifdef USE_FULL_ASSERT
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
