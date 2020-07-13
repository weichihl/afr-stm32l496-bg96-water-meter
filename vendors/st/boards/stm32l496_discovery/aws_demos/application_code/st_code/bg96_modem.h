/*
 * Copyright (C) 2018, Verizon, Inc. All rights reserved.
 */

/**
 * @file aws_BG96_modem.h
 * @brief BG96 modem interface.
 */

#ifndef _BG96_MODEM_H_
#define _BG96_MODEM_H_

typedef enum {
	network_is_up = 1,
	network_is_down = 0
} BG96_network_status_t;

/**
 * @brief Initialize the BG96 HAL.
 *
 * This function initializes the low level hardware drivers and must be called
 * before calling any other modem API
 *
 */
void BG96_HAL_Init(void);

#endif /* _BG96_MODEM_H_ */
