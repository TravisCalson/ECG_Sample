/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ads1292.h"
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t rxBuffer[MAX_BUF_LEN];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
    uint8_t sum;
    uint8_t txBuffer[MAX_BUF_LEN] = {0xAA, 0xAA, 0xF1, 0x08};
    uint8_t Mode_Info[MAX_INFO_LEN];
    uint32_t channels[2];
    int32_t p_Temp[2];
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */
    HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);
    ADS1292_PowerOnInit();

    while (Set_ADS1292_Collect(2)) {
        sprintf((char *)Mode_Info, "1292 register setting failed\r\n");
        HAL_UART_Transmit(&huart1, Mode_Info, strlen((char *) Mode_Info), 100);
    }
    sprintf((char *) Mode_Info, "1292 register setting success\r\n");
    HAL_UART_Transmit(&huart1, Mode_Info, strlen((char *) Mode_Info), 100);
    HAL_Delay(1000);

    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
//    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
//    HAL_UARTEx_ReceiveToIdle_IT(&huart1, rxBuffer, MAX_BUF_LEN);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (ads1292_receive_flag) {
          channels[0]= ads1292_Cache[3] << 16 | ads1292_Cache[4] << 8 | ads1292_Cache[5];
          channels[1]= ads1292_Cache[6] << 16 | ads1292_Cache[7] << 8 | ads1292_Cache[8];

          p_Temp[0] = Get_Voltage(channels[0]);
          p_Temp[1] = Get_Voltage(channels[1]);

          channels[0] = p_Temp[0];
          channels[1] = p_Temp[1];

          txBuffer[4] = channels[0] >> 24;
          txBuffer[5] = channels[0] >> 16;
          txBuffer[6] = channels[0] >> 8;
          txBuffer[7] = channels[0];

          txBuffer[8] = channels[1] >> 24;
          txBuffer[9] = channels[1] >> 16;
          txBuffer[10] = channels[1] >> 8;
          txBuffer[11] = channels[1];

          for (int i = 0; i < 12; i++)
              sum += txBuffer[i];
          txBuffer[12] = sum;
          HAL_UART_Transmit_DMA(&huart1, txBuffer, 13);
          HAL_UART_Transmit_DMA(&huart1, "\r\n", 2);
          ads1292_receive_flag = 0;
          sum = 0;
      }
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == DRDY_Pin) {
        ADS1292_ReadData(ads1292_Cache);
        ads1292_receive_flag = 1;
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == &huart1) {
        if (!strncmp((char *) rxBuffer, "start", 5)) {

        }
        else if (!strncmp((char *) rxBuffer, "stop", 4)) {

        }
        else if (!strncmp((char *) rxBuffer, "reset", 5)) {
            ADS1292_PowerOnInit();
        }
    }
    memset(rxBuffer, 0, MAX_BUF_LEN);
}
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
