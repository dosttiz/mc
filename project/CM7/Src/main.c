/**
  ******************************************************************************
  * @file    Templates/BootCM4_CM7/CM7/Src/main.c
  * @author  MCD Application Team
  * @brief   Main program body for Cortex-M7.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>

/** @addtogroup STM32H7xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* SPI VARIABLES DEClARATION ###########################################*/

SPI_HandleTypeDef SpiHandle;

uint8_t arrow_right_low[8] = { // For LSB first format
  0b10000000,
  0b01000000,
  0b00100000,
  0b00010000,
  0b00010000,
  0b00100000,
  0b01000000,
  0b10000000
};

uint8_t arrow_right_high[8] = { // For LSB first format
  0b10010000,
  0b01001000,
  0b00100100,
  0b00010010,
  0b00010010,
  0b00100100,
  0b01001000,
  0b10010000
};

uint8_t initial_position[8] = { // For LSB first format
  0b00000000,
  0b00000000,
  0b00000000,
  0b00011000,
  0b00011000,
  0b00000000,
  0b00000000,
  0b00000000
};

uint8_t arrow_left_low[8] = { // For LSB first format
  0b00000001,
  0b00000010,
  0b00000100,
  0b00001000,
  0b00001000,
  0b00000100,
  0b00000010,
  0b00000001
};

uint8_t arrow_left_high[8] = { // For LSB first format
  0b00001001,
  0b00010010,
  0b00100100,
  0b01001000,
  0b01001000,
  0b00100100,
  0b00010010,
  0b00001001
};

/* Функция циклического сдвига влево */
int rol(uint8_t a, int n)
{
  uint8_t t1, t2;
  n = n % (sizeof(a)*8);  // нормализуем n
  t1 = a << n;   // двигаем а влево на n бит, теряя старшие биты
  t2 = a >> (sizeof(a)*8 - n); // перегоняем старшие биты в младшие
  return t1 | t2;  // объединяем старшие и младшие биты
}

/* Функция циклического сдвига вправо */
int ror(uint8_t a, int n)
{
  uint8_t t1, t2;
  n = n % (sizeof(a)*8);  // нормализуем n
  t1 = a >> n;   // двигаем а влево на n бит, теряя младшие биты
  t2 = a << (sizeof(a)*8 - n); // перегоняем младшие биты в старшие
  return t1 | t2;  // объединяем старшие и младшие биты
}



/* ADC handler declaration */
ADC_HandleTypeDef AdcHandle;
ADC_ChannelConfTypeDef sConfig_x;
ADC_ChannelConfTypeDef sConfig_y;

/* Variable used to get converted value */
__IO uint16_t uhADCxConvertedValue = 0;

float fADCxConvertedValue;

/* Private function prototypes -----------------------------------------------*/
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);
static void SystemClock_Config(void);
static void Error_Handler(void);


/* Private functions ---------------------------------------------------------*/
int __io_putchar(int ch)
{
  // Write character to ITM ch.0
  ITM_SendChar(ch);
  return(ch);
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  int32_t timeout;

  /* System Init, System clock, voltage scaling and L1-Cache configuration are done by CPU1 (Cortex-M7)
     in the meantime Domain D2 is put in STOP mode(Cortex-M4 in deep-sleep)
  */

  /* Configure the MPU attributes */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
    Error_Handler();
  }

 /* STM32H7xx HAL library initialization:
       - Systick timer is configured by default as source of time base, but user
         can eventually implement his proper time base source (a general purpose
         timer for example or other time source), keeping in mind that Time base
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();

  /* When system initialization is finished, Cortex-M7 will release Cortex-M4  by means of
     HSEM notification */

  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();

  /*Take HSEM */
  HAL_HSEM_FastTake(HSEM_ID_0);
  /*Release HSEM in order to notify the CPU2(CM4)*/
  HAL_HSEM_Release(HSEM_ID_0,0);

  /* wait until CPU2 wakes up from stop mode */
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
    Error_Handler();
  }

  /* Add Cortex-M7 user application code here */

  /*##-1- Configure the ADC peripheral #######################################*/
   AdcHandle.Instance = ADCx;

   if (HAL_ADC_DeInit(&AdcHandle) != HAL_OK)
   {
     /* ADC de-initialization Error */
     Error_Handler();
   }

   AdcHandle.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV2;          /* Asynchronous clock mode, input ADC clock divided by 2*/
   AdcHandle.Init.Resolution               = ADC_RESOLUTION_16B;            /* 16-bit resolution for converted data */
   AdcHandle.Init.ScanConvMode             = ENABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
   AdcHandle.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;           /* EOC flag picked-up to indicate conversion end */
   AdcHandle.Init.LowPowerAutoWait         = DISABLE;                       /* Auto-delayed conversion feature disabled */
   AdcHandle.Init.ContinuousConvMode       = DISABLE;                       /* Continuous mode disabled to have only 1 conversion at each conversion trig */
   AdcHandle.Init.NbrOfConversion          = 2;                             /* Parameter discarded because sequencer is disabled */
   AdcHandle.Init.DiscontinuousConvMode    = ENABLE;                       /* Parameter discarded because sequencer is disabled */
   AdcHandle.Init.NbrOfDiscConversion      = 1;                             /* Parameter discarded because sequencer is disabled */
   AdcHandle.Init.ExternalTrigConv         = ADC_SOFTWARE_START;            /* Software start to trig the 1st conversion manually, without external event */
   AdcHandle.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE; /* Parameter discarded because software trigger chosen */
   AdcHandle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;         /* Regular Conversion data stored in DR register only */
   AdcHandle.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;      /* DR register is overwritten with the last conversion result in case of overrun */
   AdcHandle.Init.OversamplingMode         = DISABLE;                       /* No oversampling */

   if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
   {
     /* ADC initialization Error */
     Error_Handler();
   }

   /*##-2- Configure ADC regular channel ######################################*/
   sConfig_x.Channel      = ADCx_CHANNEL_X;                /* Sampled channel number */
   sConfig_x.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCx_CHANNEL */
   sConfig_x.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;    /* Sampling time (number of clock cycles unit) */
   sConfig_x.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
   sConfig_x.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */
   sConfig_x.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */

   sConfig_y.Channel      = ADCx_CHANNEL_Y;                /* Sampled channel number */
   sConfig_y.Rank         = ADC_REGULAR_RANK_2;          /* Rank of sampled channel number ADCx_CHANNEL */
   sConfig_y.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;    /* Sampling time (number of clock cycles unit) */
   sConfig_y.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
     sConfig_y.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */
     sConfig_y.Offset = 0;

   if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig_x) != HAL_OK)
   {
     /* Channel Configuration Error */
     Error_Handler();
   }

   if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig_y) != HAL_OK)
     {
       /* Channel Configuration Error */
       Error_Handler();
     }

   /* Run the ADC calibration in single-ended mode */
   if (HAL_ADCEx_Calibration_Start(&AdcHandle, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK)
   {
     /* Calibration Error */
     Error_Handler();
   }

  /* Configure the SPI peripherals */
  /* OUTL pin configuration */
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  MTX_OUTL_GPIO_CLK_ENABLE();

  GPIO_InitStruct.Pin = MTX_OUTL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(MTX_OUTL_GPIO_Port, &GPIO_InitStruct);

  /*##-1- Configure the SPI peripheral #######################################*/
  /* Set the SPI parameters */
  SpiHandle.Instance               = SPIx;
  SpiHandle.Init.Mode              = SPI_MODE_MASTER;
  SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES_TXONLY;
  SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
  SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
  SpiHandle.Init.DataSize          = SPI_DATASIZE_16BIT;
  SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_LSB;
  SpiHandle.Init.NSS               = SPI_NSS_SOFT;

  if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /*#######################################################################*/

   struct {
    uint8_t row;
    uint8_t col;
  } spi_buf; // LSB First !

  uint8_t *pixels = arrow_left_low;

  // uint32_t tickstart = HAL_GetTick();

  /*#333333333333333333333333333333333333333333333####*/


  int32_t value_x = 0;
  int32_t value_y = 0;

  int32_t ADCxConvertedValue_x;
  int32_t ADCxConvertedValue_y;

  /* Infinite loop */
  while (1)
  {
    /*##-3- Start the conversion process #######################################*/
    if (HAL_ADC_Start(&AdcHandle) != HAL_OK)
    {
      /* Start Conversation Error */
      Error_Handler();
    }

    /*##-4- Wait for the end of conversion #####################################*/
    /*  For simplicity reasons, this example is just waiting till the end of the
        conversion, but application may perform other tasks while conversion
        operation is ongoing. */
    if (HAL_ADC_PollForConversion(&AdcHandle, 10) != HAL_OK)
    {
      /* End Of Conversion flag not set on time */
      Error_Handler();
    }
    else
    {
    	value_x = HAL_ADC_GetValue(&AdcHandle);
      /* ADC conversion completed */
      /*##-5- Get the converted value of regular channel  ########################*/
     ADCxConvertedValue_x = (value_x - 32768) * 100 / 32768;
    }


    if (HAL_ADC_Start(&AdcHandle) != HAL_OK)
        {
          /* Start Conversation Error */
          Error_Handler();
        }

    if (HAL_ADC_PollForConversion(&AdcHandle, 10) != HAL_OK)
        {
          /* End Of Conversion flag not set on time */
          Error_Handler();
        }
        else
        {
        	value_y = HAL_ADC_GetValue(&AdcHandle);
          /* ADC conversion completed */
          /*##-5- Get the converted value of regular channel  ########################*/
         ADCxConvertedValue_y = (value_y - 32768) * 100 / 32768;
        }

    // printf("x: %d, y: %d\r\n", ADCxConvertedValue_x, ADCxConvertedValue_y);
    if (-25 < ADCxConvertedValue_x && 25 >  ADCxConvertedValue_x)
        pixels = &initial_position;
    else if (ADCxConvertedValue_x > 50)
        pixels = &arrow_left_high;
    else if (ADCxConvertedValue_x < -50)
        pixels = &arrow_right_high;
    else if (ADCxConvertedValue_x > 25)
        pixels = &arrow_left_low;
    else if (ADCxConvertedValue_x < -25)
        pixels = &arrow_right_low;

    for (int col = 0; col < 8; col++)
    {
      /* Light off all LEDs on entire column */
      spi_buf.row = ~(0x00);
      spi_buf.col = ~(0x00);

      HAL_GPIO_WritePin(MTX_OUTL_GPIO_Port, MTX_OUTL_Pin, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&SpiHandle, (uint8_t*)&spi_buf, 1, 5000);
      HAL_GPIO_WritePin(MTX_OUTL_GPIO_Port, MTX_OUTL_Pin, GPIO_PIN_SET);

      /* Light on required LEDs on entire column */
      /* Select column */
      uint8_t col_mask = 0b10000000 >> col; // ">>" for LSB
      spi_buf.col = ~col_mask; // "~" because "0" is ON
      /* Select rows corresponding to given column */
      uint8_t rows = 0x00;
      for (int row = 0; row < 8; row++) // get rows state (8-bit variable) corresponding to given column from pixels matrix
        rows |= (pixels[row] & col_mask) << col >> row;
      spi_buf.row = ~rows;

      HAL_GPIO_WritePin(MTX_OUTL_GPIO_Port, MTX_OUTL_Pin, GPIO_PIN_RESET);
      HAL_SPI_Transmit(&SpiHandle, (uint8_t*)&spi_buf, 1, 5000);
      HAL_GPIO_WritePin(MTX_OUTL_GPIO_Port, MTX_OUTL_Pin, GPIO_PIN_SET);

    HAL_Delay(1);
    }
  }
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE BYPASS)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (Cortex-M4 CPU, Bus matrix Clocks)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 4
  *            PLL_N                          = 400
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;

  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }

  /*
  Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
          (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
          The I/O Compensation Cell activation  procedure requires :
        - The activation of the CSI clock
        - The activation of the SYSCFG clock
        - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR

          To do this please uncomment the following code
  */

  /*
  __HAL_RCC_CSI_ENABLE() ;

  __HAL_RCC_SYSCFG_CLK_ENABLE() ;

  HAL_EnableCompensationCell();
  */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

/**
  * @brief  Configure the MPU attributes
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU as Strongly ordered for not defined regions */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */
