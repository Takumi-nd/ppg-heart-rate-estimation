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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "algorithm.h"
#include "MAX30102.h"
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
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static uint32_t ir1[BUFFER_SIZE];
static uint32_t ir2[BUFFER_SIZE];
static float ir1filt[BUFFER_SIZE] = {0.0f};
static float ir2filt[BUFFER_SIZE] = {0.0f};

static float ir1_dtr[BUFFER_SIZE];
static float ir2_dtr[BUFFER_SIZE];

static float prev_ir1[5] = {0.0f};
static float prev_ir1filt[5] = {0.0f};
static float prev_ir2[5] = {0.0f};
static float prev_ir2filt[5] = {0.0f};

static uint16_t peak_list1[20];
static uint16_t peak_list2[20];
static uint8_t num_peaks1 = 0, num_peaks2 = 0;
static uint8_t seg_count = 0;
static uint8_t hr = 0, vo2 = 0;
//static uint8_t residual_ppi1[10], residual_ppi2[10];
//static uint8_t residual_ppi_len1 = 0, residual_ppi_len2 = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
static void max30102_setup(void);
static void max30102_pre_read(void);
static void max30102_main_loop(void);

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

PUTCHAR_PROTOTYPE {
	HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 100);
	return ch;
}

void max30102_setup() {
	uint8_t uch_dummy;
	maxim_max30102_reset();
	maxim_max30102_read_reg(hi2c1, REG_INTR_STATUS_1, &uch_dummy); // reads/clears the interrupt status register
	maxim_max30102_read_reg(hi2c2, REG_INTR_STATUS_1, &uch_dummy);
	maxim_max30102_init();
}

void max30102_pre_read() {
	uint8_t count = 0;
	uint8_t i;
	uint32_t dummy_value;
	while (count < 5) {

		printf(".\n");

		for (i = 0; i < BUFFER_SIZE; i++) {
			while (HAL_GPIO_ReadPin(GPIOB, PB12_IT_Pin) == GPIO_PIN_SET || HAL_GPIO_ReadPin(GPIOB, PB13_IT_Pin) == GPIO_PIN_SET);	// wait until the interrupt pin asserts
			maxim_max30102_read_fifo(hi2c1, (ir1 + i), &dummy_value); // ppg1
			maxim_max30102_read_fifo(hi2c2, (ir2 + i), &dummy_value); // ppg2
		}
		filter(ir1, ir1filt, BUFFER_SIZE, prev_ir1, prev_ir1filt);
		filter(ir2, ir2filt, BUFFER_SIZE, prev_ir2, prev_ir2filt);

		count++;
	}
}

// Takes samples from MAX30102.  Heart rate are calculated every ST seconds
void max30102_main_loop() {
	uint8_t i;
	uint32_t dummy_value;

	if (seg_count < 5) {
		for (i = 0; i < BUFFER_SIZE; i++) {
			while (HAL_GPIO_ReadPin(GPIOB, PB12_IT_Pin) == GPIO_PIN_SET || HAL_GPIO_ReadPin(GPIOB, PB13_IT_Pin) == GPIO_PIN_SET);	// wait until the interrupt pin asserts
			maxim_max30102_read_fifo(hi2c1, (ir1 + i), &dummy_value); // ppg1
			maxim_max30102_read_fifo(hi2c2, (ir2 + i), &dummy_value); // ppg2
		}
		filter(ir1, ir1filt, BUFFER_SIZE, prev_ir1, prev_ir1filt);
		filter(ir2, ir2filt, BUFFER_SIZE, prev_ir2, prev_ir2filt);

		moving_avg_and_dtr(ir1filt, ir1_dtr, BUFFER_SIZE, (WINDOW_SIZE * 2) + 1, 1);
		moving_avg_and_dtr(ir2filt, ir2_dtr, BUFFER_SIZE, (WINDOW_SIZE * 2) + 1, 1);

		peak_detection(ir1_dtr, BUFFER_SIZE, peak_list1, &num_peaks1, seg_count, W1_PPG1, W2_PPG1, BETA_PPG1);
		peak_detection(ir2_dtr, BUFFER_SIZE, peak_list2, &num_peaks2, seg_count, W1_PPG2, W2_PPG2, BETA_PPG2);

//		printf("============================================\n");
//		for(uint8_t j = 0; j < BUFFER_SIZE; j++) {
//			printf("%d,%d,%d,%d\n", ir1[j], ir2[j], (int32_t)ir1filt[j], (int32_t)ir2filt[j]);
//		}
//
//		for (uint8_t k = 0; k < 20; k++){
//			printf("%d,%d\n", peak_list1[k], peak_list2[k]);
//		}
//		printf("=============================================\n");

		seg_count += 1;
	} else {
		seg_count = 0;
		hr_vo2_cal(peak_list1, num_peaks1, &hr, &vo2);
		printf("hr1: %d, vo2_1: %d, ", hr, vo2);
		hr_vo2_cal(peak_list2, num_peaks2, &hr, &vo2);
		printf("hr2: %d, vo2_2: %d\n", hr, vo2);
		num_peaks1 = 0;
		num_peaks2 = 0;
	}
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
	MX_I2C2_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */
	max30102_setup();
	max30102_pre_read();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		max30102_main_loop();
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C2_Init(void) {

	/* USER CODE BEGIN I2C2_Init 0 */

	/* USER CODE END I2C2_Init 0 */

	/* USER CODE BEGIN I2C2_Init 1 */

	/* USER CODE END I2C2_Init 1 */
	hi2c2.Instance = I2C2;
	hi2c2.Init.ClockSpeed = 400000;
	hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c2.Init.OwnAddress1 = 0;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C2_Init 2 */

	/* USER CODE END I2C2_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pins : PB12_IT_Pin PB13_IT_Pin */
	GPIO_InitStruct.Pin = PB12_IT_Pin | PB13_IT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
