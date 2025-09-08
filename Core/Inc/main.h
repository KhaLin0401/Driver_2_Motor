/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
// External declarations for handles
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;
extern ADC_HandleTypeDef hadc1;

// External declarations for global variables
extern uint16_t g_holdingRegisters[];
extern uint16_t g_inputRegisters[];
extern uint8_t g_coils[];
extern uint8_t g_discreteInputs[];
extern uint32_t g_taskCounter;
extern uint32_t g_modbusCounter;
extern uint8_t rxBuffer[];
extern uint8_t rxIndex;
extern uint8_t frameReceived;
extern uint32_t g_lastUARTActivity;
extern uint32_t g_totalReceived;
extern uint32_t g_corruptionCount;
extern uint8_t g_receivedIndex;

// Function prototypes for USER CODE BEGIN 4 functions
uint16_t calcCRC(uint8_t *buf, int len);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void resetUARTCommunication(void);
void processModbusFrame(void);

// FreeRTOS task function prototypes
void StartUartTask(void *argument);

// Private function prototypes
static void MX_I2C1_Init(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED4_Pin GPIO_PIN_13
#define LED4_GPIO_Port GPIOC
#define LED3_Pin GPIO_PIN_14
#define LED3_GPIO_Port GPIOC
#define LED2_Pin GPIO_PIN_15
#define LED2_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_1
#define LED1_GPIO_Port GPIOA
#define DIR_1_Pin GPIO_PIN_4
#define DIR_1_GPIO_Port GPIOA
#define IN1_Pin GPIO_PIN_5
#define IN1_GPIO_Port GPIOA
#define DIR_2_Pin GPIO_PIN_1
#define DIR_2_GPIO_Port GPIOB
#define IN4_Pin GPIO_PIN_10
#define IN4_GPIO_Port GPIOB
#define DIR_4_Pin GPIO_PIN_12
#define DIR_4_GPIO_Port GPIOB
#define IN2_Pin GPIO_PIN_13
#define IN2_GPIO_Port GPIOB
#define IN3_Pin GPIO_PIN_14
#define IN3_GPIO_Port GPIOB
#define DIR_3_Pin GPIO_PIN_9
#define DIR_3_GPIO_Port GPIOA
#define OUT2_Pin GPIO_PIN_3
#define OUT2_GPIO_Port GPIOB
#define OUT1_Pin GPIO_PIN_4
#define OUT1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
