/**
  ******************************************************************************
  * @file    board_buttons.c
  * @author  MCD Application Team
  * @brief   Implements functions for user buttons actions
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
#include "plf_config.h"

#if (USE_BOARD_BUTTONS == 1)

#include "cmsis_os_misrac2012.h"
#include "board_buttons.h"
#include "dc_common.h"
#include "error_handler.h"

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
#define BOARD_BUTTONS_DEBOUNCE_TIMEOUT  (200U) /* in millisec */
#define BOARD_BUTTONS_MSG_QUEUE_SIZE (uint32_t) (8)

/* Private variables ---------------------------------------------------------*/
static osMessageQId board_buttons_msg_queue;
static board_buttons_t board_buttons_up;
static board_buttons_t board_buttons_dn;
static board_buttons_t board_buttons_right;
static board_buttons_t board_buttons_left;
static board_buttons_t board_buttons_sel;

static osTimerId DebounceTimerHandle;
static __IO uint8_t debounce_ongoing = 0U;

/* Global variables ----------------------------------------------------------*/
dc_com_res_id_t    DC_BOARD_BUTTONS_UP      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_BOARD_BUTTONS_DN      = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_BOARD_BUTTONS_RIGHT   = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_BOARD_BUTTONS_LEFT    = DC_COM_INVALID_ENTRY;
dc_com_res_id_t    DC_BOARD_BUTTONS_SEL     = DC_COM_INVALID_ENTRY;

/* Private function prototypes -----------------------------------------------*/
static void board_buttons_post_event_debounce(dc_com_event_id_t event_id);
static void board_buttons_bebounce_timer_callback(void const *argument);
static void board_buttons_thread(void const *argument);

/* Functions Definition ------------------------------------------------------*/

/**
  * @brief  board_button thread
  * @param  argument (UNUSED)
  * @retval none
  */
void board_buttons_thread(void const *argument)
{
  osEvent event;
  dc_com_event_id_t event_id;

  for (;;)
  {
    event = osMessageGet(board_buttons_msg_queue, RTOS_WAIT_FOREVER);
    if (event.status == osEventMessage)
    {
      event_id = (dc_com_event_id_t)event.value.v;
      (void)dc_com_write_event(&dc_com_db, event_id) ;
    }
  }
}

/**
  * @brief  debounce timer management
  * @param  event_id            event id
  * @retval board_buttons_status_t    return status
  */
static void board_buttons_bebounce_timer_callback(void const *argument)
{
  UNUSED(argument);
  debounce_ongoing = 0U;
}

/**
  * @brief  debounce event management
  * @param  event_id            event id
  * @retval board_buttons_status_t    return status
  */
static void board_buttons_post_event_debounce(dc_com_event_id_t event_id)
{
  /* post event to the queue only if no ongoing debounce
   * limitation: all events with debounce are sharing same timer
   */
  if (debounce_ongoing == 0U)
  {
    if (osMessagePut(board_buttons_msg_queue, (uint32_t)event_id, 0U) != osOK)
    {
      ERROR_Handler(DBG_CHAN_DATA_CACHE, 7, ERROR_WARNING);
    }
    else
    {
      (void)osTimerStart(DebounceTimerHandle, BOARD_BUTTONS_DEBOUNCE_TIMEOUT);
      debounce_ongoing = 1U;
    }
  }
}


void board_button_joy_up_press(void)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  board_buttons_post_event_debounce(DC_BOARD_BUTTONS_UP);
}

void board_button_joy_down_press(void)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  board_buttons_post_event_debounce(DC_BOARD_BUTTONS_DN);
}

void board_button_joy_right_press(void)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  board_buttons_post_event_debounce(DC_BOARD_BUTTONS_RIGHT);
}

void board_button_joy_left_press(void)
{
  /* USER can implement button action here
  * WARNING ! this function is called under an IT, treatment has to be short !
  * (like posting an event into a queue for example)
  */
  board_buttons_post_event_debounce(DC_BOARD_BUTTONS_LEFT);
}


/**
  * @brief  component initialisation
  * @param  -
  * @retval board_buttons_status_t     return status
  */
board_buttons_status_t board_buttons_init(void)
{
  /* definition and creation of DebounceTimer */
  osTimerDef(DebounceTimer, board_buttons_bebounce_timer_callback);
  DebounceTimerHandle = osTimerCreate(osTimer(DebounceTimer), osTimerOnce, NULL);

  /* queue creation */
  osMessageQDef(BOARD_BUTTONS_MSG_QUEUE, BOARD_BUTTONS_MSG_QUEUE_SIZE, uint32_t);
  board_buttons_msg_queue = osMessageCreate(osMessageQ(BOARD_BUTTONS_MSG_QUEUE), NULL);

  DC_BOARD_BUTTONS_UP      = dc_com_register_serv(&dc_com_db, (void *)&board_buttons_up,
                                                  (uint16_t)sizeof(board_buttons_t));
  DC_BOARD_BUTTONS_DN      = dc_com_register_serv(&dc_com_db, (void *)&board_buttons_dn,
                                                  (uint16_t)sizeof(board_buttons_t));
  DC_BOARD_BUTTONS_RIGHT   = dc_com_register_serv(&dc_com_db, (void *)&board_buttons_right,
                                                  (uint16_t)sizeof(board_buttons_t));
  DC_BOARD_BUTTONS_LEFT    = dc_com_register_serv(&dc_com_db, (void *)&board_buttons_left,
                                                  (uint16_t)sizeof(board_buttons_t));
  DC_BOARD_BUTTONS_SEL     = dc_com_register_serv(&dc_com_db, (void *)&board_buttons_sel,
                                                  (uint16_t)sizeof(board_buttons_t));

  return BOARD_BUTTONS_OK;
}


/**
  * @brief  component start
  * @param  -
  * @retval board_buttons_status_t     return status
  */
board_buttons_status_t board_buttons_start(void)
{
  static osThreadId board_buttons_thread_id;

  /* definition and creation of dc_CtrlEventTask */
  osThreadDef(dc_CtrlTask, board_buttons_thread, BOARD_BUTTONS_THREAD_PRIO, 0, BOARD_BUTTONS_THREAD_STACK_SIZE);
  board_buttons_thread_id = osThreadCreate(osThread(dc_CtrlTask), NULL);
  if (board_buttons_thread_id == NULL)
  {
    ERROR_Handler(DBG_CHAN_DATA_CACHE, 7, ERROR_WARNING);
  }
  else
  {
#if (USE_STACK_ANALYSIS == 1)
    (void)stackAnalysis_addStackSizeByHandle(board_buttons_thread_id, USED_BOARD_BUTTONS_THREAD_STACK_SIZE);
#endif /* USE_STACK_ANALYSIS == 1 */
  }

  if (DC_BOARD_BUTTONS_UP != DC_COM_INVALID_ENTRY)
  {
    board_buttons_up.rt_state    = DC_SERVICE_ON;
  }
  if (DC_BOARD_BUTTONS_DN != DC_COM_INVALID_ENTRY)
  {
    board_buttons_dn.rt_state    = DC_SERVICE_ON;
  }
  if (DC_BOARD_BUTTONS_RIGHT != DC_COM_INVALID_ENTRY)
  {
    board_buttons_right.rt_state = DC_SERVICE_ON;
  }
  if (DC_BOARD_BUTTONS_LEFT != DC_COM_INVALID_ENTRY)
  {
    board_buttons_left.rt_state = DC_SERVICE_ON;
  }
  if (DC_BOARD_BUTTONS_SEL != DC_COM_INVALID_ENTRY)
  {
    board_buttons_sel.rt_state   = DC_SERVICE_ON;
  }


  return BOARD_BUTTONS_OK;
}

#endif /* (USE_BOARD_BUTTONS == 1) */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
