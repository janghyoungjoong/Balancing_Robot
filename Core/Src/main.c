/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include "mpu6050.h"
#include "pid.h"
#include "math.h"
#include "state_machine.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
 void MX_GPIO_Init(void);
 void MX_I2C1_Init(void);
 void MX_USART2_UART_Init(void);
 void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
 int __io_putchar(int ch)
 {
   HAL_UART_Transmit(&huart2,(uint8_t*)&ch,1,HAL_MAX_DELAY);
   return ch;
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
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* Initialize IMU (Gyroscope and Accelerometer) */
  mpu6050_init();
  gyro_data gyro;
  accel_data accel;

  /* Set up a variable to keep track of motor direction switches */
  int toggle = 0;

  /* Upright Position */
  const float set_pitch = 0.0f;

  /* Set up PID Controller and gain constants for tuning. */
  PIDController controller;
  controller.kp = 47.25f;
  controller.ki = 26.5f;
  controller.kd = 0.2f;
  controller.sampling_time = 500;
  PIDController_Init(&controller);

  /* Turn on Channel 3 motor at 100% duty cycle */
  HAL_GPIO_WritePin(GPIOA, PA9_D8_OUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PC7_D9_OUT_GPIO_Port, PC7_D9_OUT_Pin, GPIO_PIN_RESET);

  /* Turn on Channel 4 motor at 100% duty cycle */
  HAL_GPIO_WritePin(GPIOA, PA8_D7_OUT_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(PB10_D6_OUT_GPIO_Port, PB10_D6_OUT_Pin, GPIO_PIN_RESET);

  /* Start Timer and its channels for the motor driver */
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_Base_Start(&htim3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);	//stdby

  /* Configure default robot state */
  Robot_State state = ROBOT_IDLE;

  /* Begin Control loop */
  while (1)
  {
	/* Capture gyroscope and accelerometer measurements */
	mpu6050_read_accel(&accel);
	mpu6050_read_gyro(&gyro);

	/* Update motor output based on the captured pitch angle */
	PIDController_Update(&controller, set_pitch, accel.pitch_angle);

	/* State Machine Implementation */
	/* This state machine is to ensure that the robot moves in the correct
	   direction as it receives new PID outputs. */
	switch(state)
	{
	  case ROBOT_IDLE:
		state = ROBOT_START;
		break;
	  case ROBOT_START:
		// Determine which state to enter next
		state = robot_start_state(&accel);
		break;
	  case ROBOT_FORWARD:
		// Move the robot backward
		state = robot_forward_state(&accel, &toggle);
		break;
	  case ROBOT_BACKWARD:
		// Move the robot forward
		state = robot_backward_state(&accel, &toggle);
		break;
	  case ROBOT_STOP:
		// Stop all motors
		state = robot_stopped_state(&htim3);
		break;
	  case ROBOT_BALANCED:
		break;
	  default:
		break;
	}

	/* Change the motor duty cycle based on PID outputs */
	__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_3, controller.motor_output);
	__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_4, controller.motor_output);


	printf("PITCH ANGLE: %.2f MOTOR OUTPUT: %.2f TOGGLE: %d\n\r ", accel.pitch_angle, controller.motor_output, toggle);
	printf("Proportional %.2f Derivative %.2f Integral: %.2f\n\r ", controller.proportional, controller.derivative, controller.integral);
	HAL_Delay(100);
  }
  /* USER CODE END 3 */
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_TIM34;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.Tim34ClockSelection = RCC_TIM34CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
