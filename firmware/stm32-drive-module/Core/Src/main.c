/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "wizchip_port.h"
#include "mb.h"
#include "mbproto.h"
#include "mbutils.h"
#include "stepper.h"
#include "time.h"
#include <stdio.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define REG_HOLDING_START 0x0001
#define REG_HOLDING_NREGS 10
#define MBTCP_PORT        502

#define REG_RESULT   0
#define REG_ACK_RETURN 1
#define REG_STATE 2
#define REG_SENSOR 3
#define REG_TASK   5
#define REG_ACK_ID 6

#define SUCCESS 1
#define FAULT 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

Stepper_t diverter;
Stepper_t conveyor;

/* Internal state */
uint16_t usRegHoldingBuf[REG_HOLDING_NREGS] = {0};

uint32_t lastUpdateMs = 0;
uint32_t now_ms = 0;

uint8_t connectionTimeout;
uint16_t timeForTimeout = 1000;

uint16_t lastAckID = 0xFFFF;
uint8_t diverterHomed = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* ===================== LOGIC FUNCTIONS (SAFE) ===================== */

uint8_t watchdog(void)
{
    now_ms = HAL_GetTick();
    connectionTimeout = ((now_ms - lastUpdateMs) > timeForTimeout);
    return connectionTimeout;
}

uint8_t taskCheck(uint16_t *task)
{
    uint16_t currentAckID = usRegHoldingBuf[REG_ACK_ID];

    if (currentAckID != lastAckID)
    {
        lastAckID = currentAckID;
        *task = usRegHoldingBuf[REG_TASK];
        return 1;
    }
    return 0;
}

void taskAck(uint16_t result)
{
    usRegHoldingBuf[REG_RESULT] = result;
    usRegHoldingBuf[REG_ACK_RETURN] = lastAckID;
}

void diverterHoming(void){
	  Stepper_GoToPos(&diverter, 0.5f, -170.0f / 360.0f);
	  while (Stepper_IsRunning(&diverter));
	  Stepper_SetCurrentPos(&diverter, 0.0f);
	  HAL_Delay(10);
	  Stepper_GoToPos(&diverter, 2.0f, 78.0f / 360.0f);
	  while (Stepper_IsRunning(&diverter));
	  HAL_Delay(10);
	  Stepper_SetCurrentPos(&diverter, 0.0f);
}

void homePosUpdate(void){
	diverterHomed = (HAL_GPIO_ReadPin(GPIO_PIN_10_GPIO_Port, GPIO_PIN_10_Pin) == GPIO_PIN_RESET) ? 1 : 0;
}

/* ===================== MODBUS CALLBACK ===================== */

eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                             USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ((usAddress >= REG_HOLDING_START) &&
        (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS))
    {
        iRegIndex = (int)(usAddress - REG_HOLDING_START);

        switch (eMode)
        {
            case MB_REG_READ:
                while (usNRegs--)
                {
                    *pucRegBuffer++ = (uint8_t)(usRegHoldingBuf[iRegIndex] >> 8);
                    *pucRegBuffer++ = (uint8_t)(usRegHoldingBuf[iRegIndex] & 0xFF);
                    iRegIndex++;
                }
                break;

            case MB_REG_WRITE:
                while (usNRegs--)
                {
                    usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
                    usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                    iRegIndex++;
                }
                lastUpdateMs = HAL_GetTick();
                break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress,
                           USHORT usNCoils, eMBRegisterMode eMode)
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNDiscrete)
{
    return MB_ENOREG;
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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */


  if (W5500_Init() != 0) Error_Handler();
  eMBTCPInit(MBTCP_PORT);
  eMBEnable();

  Stepper_GPIO_Timer_Init(&conveyor,
		  	  	  	  	  GPIO_PIN_14_GPIO_Port, GPIO_PIN_14_Pin,
						  GPIO_PIN_15_GPIO_Port, GPIO_PIN_15_Pin,
                          &htim1,
                          TIM_CHANNEL_1);

  Stepper_GPIO_Timer_Init(&diverter,
						  GPIO_PIN_4_GPIO_Port, GPIO_PIN_4_Pin,
						  GPIO_PIN_5_GPIO_Port, GPIO_PIN_5_Pin,
                          &htim2,
                          TIM_CHANNEL_2);

  Stepper_Init(&conveyor, 10.0f, 1.0f, 8, 5);
  Stepper_Init(&diverter, 10.0f, 6.0f, 8, 5);


  printf("Modbus system started\r\n");

  diverterHoming();

  float outSpeed = 1.0f;
  float runSpeed = 0.5f;
  float fineSpeed = 0.2f;
  float pos1 = 55.0f / 360.0f;
  float homePos = 0.0f;
  float pos3 = -55.0f / 360.0f;
  float diverterSpeed = 3.0f;
  uint8_t forward = 1;
  uint8_t backward = 0;
  uint16_t timeForDiverting = 500;
  uint16_t timeForDiverterHoming = 500;



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while(1){
	modbus_tcps(0, MBTCP_PORT);
	eMBPoll();
	homePosUpdate();

//	if(watchdog()){
//		Stepper_StopHard(&conveyor);
//		Stepper_StopHard(&diverter);
//	}


	uint16_t task;
	if (taskCheck(&task))
	{
		switch (task)
		{
			  case 0x0001:  // Automatic conveyor run
				  Stepper_Stop(&conveyor);
				  Stepper_RunSpeed(&conveyor, runSpeed, forward);
				  taskAck(SUCCESS);
				  break;

			  case 0x0002: // Conveyor stop
				  Stepper_Stop(&conveyor);
				  taskAck(SUCCESS);
				  break;

			  case 0x0003: // Emergency stop
				  Stepper_StopHard(&conveyor);
				  Stepper_StopHard(&diverter);
				  taskAck(SUCCESS);
				  break;

			  case 0x000A: // Automatic divert to bin 1
				  Stepper_GoToPos(&diverter, diverterSpeed, pos1);
				  while (Stepper_IsRunning(&diverter));
				  homePosUpdate();
				  if(diverterHomed){ taskAck(FAULT); break; }
				  Stepper_RunSpeed(&conveyor, outSpeed, forward);
				  HAL_Delay(timeForDiverting);
				  Stepper_RunSpeed(&conveyor, runSpeed, forward);
				  HAL_Delay(timeForDiverterHoming);
				  Stepper_GoToPos(&diverter, diverterSpeed, homePos);
				  while (Stepper_IsRunning(&diverter));
				  homePosUpdate();
				  if(!diverterHomed){ taskAck(FAULT); break; }
				  taskAck(SUCCESS);
				  break;

			  case 0x000B: // Automatic divert to bin 2
				  Stepper_GoToPos(&diverter, diverterSpeed, homePos);
				  while (Stepper_IsRunning(&diverter));
				  homePosUpdate();
				  if(!diverterHomed){ taskAck(FAULT); break; }
				  Stepper_RunSpeed(&conveyor, outSpeed, forward);
				  HAL_Delay(timeForDiverting);
				  Stepper_RunSpeed(&conveyor, runSpeed, forward);
				  HAL_Delay(timeForDiverterHoming);
				  taskAck(SUCCESS);
				  break;

			  case 0x000C: // Automatic divert to bin 3
				  Stepper_GoToPos(&diverter, diverterSpeed, pos3);
				  while (Stepper_IsRunning(&diverter));
				  homePosUpdate();
				  if(diverterHomed){ taskAck(FAULT); break; }
				  Stepper_RunSpeed(&conveyor, outSpeed, forward);
				  HAL_Delay(timeForDiverting);
				  Stepper_RunSpeed(&conveyor, runSpeed, forward);
				  HAL_Delay(timeForDiverterHoming);
				  Stepper_GoToPos(&diverter, diverterSpeed, homePos);
				  while (Stepper_IsRunning(&diverter));
				  homePosUpdate();
				  if(!diverterHomed){ taskAck(FAULT); break; }
				  taskAck(SUCCESS);
				  break;

			  case 0x0010: // Manual conveyor run
				  Stepper_Stop(&conveyor);
				  Stepper_RunSpeed(&conveyor, fineSpeed, forward);
				  taskAck(SUCCESS);
				  break;

			  case 0x0020: // Manual conveyor reverse
				  Stepper_Stop(&conveyor);
				  Stepper_RunSpeed(&conveyor, fineSpeed, backward);
				  taskAck(SUCCESS);
				  break;

			  case 0xF000: // Diverter homing
				  Stepper_Stop(&conveyor);
				  diverterHoming();
				  taskAck(SUCCESS);
				  break;

			  case 0x00A0: // Manual divert to bin 1
				  Stepper_GoToPos(&diverter, diverterSpeed, pos1);
				  while (Stepper_IsRunning(&diverter));
				  taskAck(SUCCESS);
				  break;

			  case 0x00B0: // Manual divert to bin 2
				  Stepper_GoToPos(&diverter, diverterSpeed, homePos);
				  while (Stepper_IsRunning(&diverter));
				  taskAck(SUCCESS);
				  break;

			  case 0x00C0: // Manual divert to bin 3
				  Stepper_GoToPos(&diverter, diverterSpeed, pos3);
				  while (Stepper_IsRunning(&diverter));
				  taskAck(SUCCESS);
				  break;

			default:
				// Unknown task
				break;
		}
	}
	usRegHoldingBuf[REG_STATE] = !Stepper_IsRunning(&conveyor) ? 0 : Stepper_GetDir(&conveyor) == forward ? 1 : Stepper_GetDir(&conveyor) == backward ? 2 : -1;
	usRegHoldingBuf[REG_SENSOR] = diverterHomed;
  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 95;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 95;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 5;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, RESET_Pin|CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14_Pin|GPIO_PIN_15_Pin|GPIO_PIN_4_Pin|GPIO_PIN_5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : RESET_Pin CS_Pin */
  GPIO_InitStruct.Pin = RESET_Pin|CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = GPIO_PIN_10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIO_PIN_10_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PBPin PBPin PBPin PBPin */
  GPIO_InitStruct.Pin = GPIO_PIN_14_Pin|GPIO_PIN_15_Pin|GPIO_PIN_4_Pin|GPIO_PIN_5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    Stepper_TimerCallback(&conveyor, htim);
    Stepper_TimerCallback(&diverter, htim);
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
