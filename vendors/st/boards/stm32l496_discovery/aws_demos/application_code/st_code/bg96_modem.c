/*
 * Copyright (C) 2018, Verizon, Inc. All rights reserved.
 */


#include "cmsis_os.h"
#include "bg96_modem.h"
#include "dc_common.h"
#include "cellular_service.h"
#include "cellular_service_task.h"
#include "cellular_mngt.h"
#include "FreeRTOS.h"
#include "task.h"

BG96_network_status_t BG96_network_status = network_is_down;

/* Message queue which signals modem status */
osMessageQId BG96_modem_queue;

/* Function prototypes -------------------------------------------------------*/
static void BG96_net_up_cb ( dc_com_event_id_t dc_event_id, void* private_gui_data );

/**
 * @brief Initialize the BG96 HAL.
 *
 * This function initializes the low level hardware drivers and must be called
 * before calling any other modem API
 *
 */
void BG96_HAL_Init(void)
{
	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	//MX_LPUART1_UART_Init();
}

/**
 * @brief Initialize the BG96 drivers.
 *
 * This function initializes the low level drivers and must be called
 * before calling BG96_Modem_Start
 *
 */
void BG96_Modem_Init(void)
{
	cellular_init();

	dc_com_register_gen_event_cb(&dc_com_db, BG96_net_up_cb, (void*) NULL);

	osMessageQDef(BG96_modem_queue, 1, uint32_t);
	BG96_modem_queue = osMessageCreate(osMessageQ(BG96_modem_queue), NULL);

	if(BG96_modem_queue == NULL)
	{
		configPRINTF(("Can't create Queue\r\n"));
	}
}

/**
 * @brief Callback function.
 *
 * This function is called whenever there is an event on the network interface
 * manager. Only interested when the modem is up.
 *
 */
static void BG96_net_up_cb ( dc_com_event_id_t dc_event_id, void* private_gui_data )
{
	if( dc_event_id == DC_COM_NIFMAN_INFO)
	{
		dc_nifman_info_t  dc_nifman_info;
		dc_com_read( &dc_com_db, DC_COM_NIFMAN_INFO, (void *)&dc_nifman_info, sizeof(dc_nifman_info));
		if(dc_nifman_info.rt_state  ==  DC_SERVICE_ON)
		{
			osMessagePut(BG96_modem_queue, dc_event_id, 0);
			BG96_network_status = network_is_up;
			configPRINTF(("Network is up\r\n"));
		}
		else
		{
			configPRINTF(("Network is down\r\n"));
		}
	}
}

/**
 * @brief Starts the BG96 drivers.
 *
 * This function starts the low level  drivers and must be called last in the sequence
 * e.g. after BG96_HAL_Init and BG96_Modem_Init
 *
 */
void BG96_Modem_Start ()
{
	cellular_start();
}
