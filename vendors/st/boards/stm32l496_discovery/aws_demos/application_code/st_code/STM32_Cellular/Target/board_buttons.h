/**
  ******************************************************************************
  * @file    board_buttons.h
  * @author  MCD Application Team
  * @brief   Header for board_buttons.c module
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
#ifndef BOARD_BUTTONS_H
#define BOARD_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "plf_config.h"
#if (USE_BOARD_BUTTONS == 1)
#include "dc_common.h"

/* Exported types ------------------------------------------------------------*/
/* Data Cache structure for button entries */
typedef struct
{
  dc_service_rt_header_t header;
  dc_service_rt_state_t  rt_state;
} board_buttons_t;

typedef enum
{
  BOARD_BUTTONS_OK = 0x00,
  BOARD_BUTTONS_ERROR
} board_buttons_status_t;

/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Button event Data Cache entries */
/** @brief Control Button UP Data Cache entries         */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_UP;

/** @brief Control Button DOWN Data Cache entries       */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_DN;

/** @brief Control Button RIGHT Data Cache entries      */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_RIGHT;

/** @brief Control Button LEFT Data Cache entries       */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_LEFT;

/** @brief Control Button SELECTION Data Cache entries  */
extern dc_com_res_id_t    DC_BOARD_BUTTONS_SEL;

/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */

void board_button_joy_up_press(void);
void board_button_joy_down_press(void);
void board_button_joy_right_press(void);
void board_button_joy_left_press(void);

board_buttons_status_t board_buttons_init(void);
board_buttons_status_t board_buttons_start(void);

#ifdef __cplusplus
}
#endif

#endif /* (USE_BOARD_BUTTONS == 1) */

#endif /* BOARD_BUTTONS_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
