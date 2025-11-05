/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

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


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC2_SCK_Pin GPIO_PIN_2
#define ADC2_SCK_GPIO_Port GPIOE
#define ADC2_MISO_Pin GPIO_PIN_5
#define ADC2_MISO_GPIO_Port GPIOE
#define ADC2_MOSI_Pin GPIO_PIN_6
#define ADC2_MOSI_GPIO_Port GPIOE
#define LIN_TX_Pin GPIO_PIN_8
#define LIN_TX_GPIO_Port GPIOD
#define LIN_RX_Pin GPIO_PIN_9
#define LIN_RX_GPIO_Port GPIOD
#define ENC1_CH1_Pin GPIO_PIN_12
#define ENC1_CH1_GPIO_Port GPIOD
#define ENC1_CH2_Pin GPIO_PIN_13
#define ENC1_CH2_GPIO_Port GPIOD
#define ENC2_CH1_Pin GPIO_PIN_6
#define ENC2_CH1_GPIO_Port GPIOC
#define ENC2_CH2_Pin GPIO_PIN_7
#define ENC2_CH2_GPIO_Port GPIOC
#define ADC1_SCK_Pin GPIO_PIN_10
#define ADC1_SCK_GPIO_Port GPIOC
#define ADC1_MISO_Pin GPIO_PIN_11
#define ADC1_MISO_GPIO_Port GPIOC
#define ADC1_MOSI_Pin GPIO_PIN_12
#define ADC1_MOSI_GPIO_Port GPIOC
#define SMAC1_RX_Pin GPIO_PIN_0
#define SMAC1_RX_GPIO_Port GPIOD
#define SMAC1_TX_Pin GPIO_PIN_1
#define SMAC1_TX_GPIO_Port GPIOD
#define SMAC2_RX_Pin GPIO_PIN_5
#define SMAC2_RX_GPIO_Port GPIOB
#define SMAC2_TX_Pin GPIO_PIN_6
#define SMAC2_TX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
