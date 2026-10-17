/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
/* Lueftersteuerung: siehe dshot.h (DShot600 auf PA15 / TIM2_CH1) */
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VSYNC_FREQ_Pin GPIO_PIN_4
#define VSYNC_FREQ_GPIO_Port GPIOB
#define TestPin_Pin GPIO_PIN_2
#define TestPin_GPIO_Port GPIOI
#define LCD_BL_CTRL_Pin GPIO_PIN_3
#define LCD_BL_CTRL_GPIO_Port GPIOK
#define LCD_DISP_Pin GPIO_PIN_12
#define LCD_DISP_GPIO_Port GPIOI
#define MS5525_CS_PIN_Pin GPIO_PIN_8
#define MS5525_CS_PIN_GPIO_Port GPIOA
#define FRAME_RATE_Pin GPIO_PIN_7
#define FRAME_RATE_GPIO_Port GPIOC
#define RENDER_TIME_Pin GPIO_PIN_6
#define RENDER_TIME_GPIO_Port GPIOC
#define MCU_ACTIVE_Pin GPIO_PIN_6
#define MCU_ACTIVE_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */
extern volatile float g_pressure_mbar;
extern volatile float g_pressure_mmh2o;
extern volatile float g_temp_degc;

/* PID-Regler Parameter (werden von der UI gesetzt) */
extern volatile float g_pid_kp;
extern volatile float g_pid_ki;
extern volatile float g_pid_kd;
extern volatile float g_pid_setpoint;   /* Sollwert in mmH2O */
extern volatile float g_pid_output;     /* Reglerausgang 0-100 % */
extern volatile float g_pid_dt;         /* Abtastzeit in Sekunden */
extern volatile uint8_t g_pid_enabled;
extern volatile uint8_t g_fan_run;      /* 1 = Luefter laeuft (PID aktiv), 0 = DShot MOTOR_STOP */
/* USER CODE END Private defines */

/*------Testpin (Oszilloskop-Trigger, PI2)---------*/
void TPWrite(void);
void TPClear(void);
void TPToggle(void);
void TPInit(void);



#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
