/**
  ******************************************************************************
  * @file    board_display.c
  * @author  MCD Application Team
  * @brief   Implements functions for display actions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "plf_config.h"
#include "board_display.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Functions Definition ------------------------------------------------------*/


/**
  * @brief  Initialize a DISPLAY
  * @param
  * @retval -
  */

uint8_t BD_DISPLAY_Init(void)
{
  return BSP_LCD_Init();
}

uint8_t BD_DISPLAY_DrawRawFrameBuffer(uint8_t *buffer)
{
  return BSP_LCD_DrawRawFrameBuffer(buffer);
}

void BD_DISPLAY_Clear(uint16_t color)
{
  BSP_LCD_Clear(color);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
