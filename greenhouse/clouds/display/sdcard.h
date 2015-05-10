/**   
 * \file sdcard.h
 * \date 2013-4-27
 * \brief SD card
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

#ifndef SDCARD_H_
#define SDCARD_H_

#include "fatfs/diskio.h"
#include <avr32/io.h>			// avr32 platform headers

#define mmc_wait_pdca_finish() while(AVR32_PDCA.channel[0].sr)
#define CARD_DRIVE 0

/**
 * \brief Send command to card
 */
BYTE mmc_send_command ( BYTE cmd, DWORD arg );
/**
 *	\brief Send a data packet to the card
 */
int mmc_send_datablock ( const BYTE *buffer, BYTE token );
/**
 * \brief Read datablock from the card
 */
int mmc_read_datablock ( BYTE *buffer, UINT bytes );

#endif /* SDCARD_H_ */