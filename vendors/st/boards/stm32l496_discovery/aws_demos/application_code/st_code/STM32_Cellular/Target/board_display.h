/**
  ******************************************************************************
  * @file    board_display.h
  * @author  MCD Application Team
  * @brief   Header for board_display.c module
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOARD_DISPLAY_H
#define BOARD_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* include BSP for L496 */
/* MISRAC messages linked to BSP include are ignored */
/*cstat -MISRAC2012-* */
#include "stm32l496g_discovery_lcd.h"
/*cstat +MISRAC2012-* */

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  BD_DISPLAY_OK = 0,
  BD_DISPLAY_ERROR
} BD_DISPLAY_Status_TypDef;
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
uint8_t BD_DISPLAY_Init(void);
uint8_t BD_DISPLAY_DrawRawFrameBuffer(uint8_t *buffer);
void    BD_DISPLAY_Clear(uint16_t color);
#ifdef __cplusplus
}
#endif

#endif /* BOARD_DISPLAY_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
