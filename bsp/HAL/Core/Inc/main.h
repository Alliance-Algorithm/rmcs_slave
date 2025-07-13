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
#include "stm32h7xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void AppEntry(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Power_5V_EN_Pin GPIO_PIN_15
#define Power_5V_EN_GPIO_Port GPIOC
#define CS1_ACCEL_Pin GPIO_PIN_0
#define CS1_ACCEL_GPIO_Port GPIOC
#define BMI088_MOSI_Pin GPIO_PIN_1
#define BMI088_MOSI_GPIO_Port GPIOC
#define BMI088_MISO_Pin GPIO_PIN_2
#define BMI088_MISO_GPIO_Port GPIOC
#define CS1_GYRO_Pin GPIO_PIN_3
#define CS1_GYRO_GPIO_Port GPIOC
#define INT1_ACC_Pin GPIO_PIN_10
#define INT1_ACC_GPIO_Port GPIOE
#define INT1_ACC_EXTI_IRQn EXTI15_10_IRQn
#define INT1_GYRO_Pin GPIO_PIN_12
#define INT1_GYRO_GPIO_Port GPIOE
#define INT1_GYRO_EXTI_IRQn EXTI15_10_IRQn
#define BMI088_SCK_Pin GPIO_PIN_13
#define BMI088_SCK_GPIO_Port GPIOB
#define KEY_Pin GPIO_PIN_15
#define KEY_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
