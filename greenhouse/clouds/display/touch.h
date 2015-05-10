/**   
 * \file touch.h
 * \date 2013-4-27
 * \brief Touch controller
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

#ifndef TOUCH_H_
#define TOUCH_H_

namespace display {

typedef struct {
	unsigned short x;
	unsigned short y;
	bool	penstate;
} touch_t;
/**
 *	\brief Initialize the touch controller
 *
 *	This needs to be run before any other touch related 
 *	functions
 */
void touch_init(void);
/**
 *	\brief Enables touch recognition
 *
 *	Controller needs to be calibrated before this works correctly.
 */
void touch_enable(void);
/**
 * \brief Disables the touch controller
 */
void touch_disable(void);
/**
 *	\brief Calibrate the touch controller.
 *
 *	Displays the calibration screen.
 *	Tap on the crosses in the corned to calibrate.
 *	Calibration data is stored in the controller's EEPROM
 */
void touch_calibrate(void);
/**
 *	\brief Send data to the touch controller
 *
 *	\param arr[] Data array
 *	\param size  Data array size in bytes
 */
void touch_send_data( unsigned char arr[], int size );
/**
 *	\brief Set value of a touch controller register
 *
 *	\param reg Register number
 *	\param val Register value
 */
void touch_set_reg( int reg=0, int val=0 );
/**
 *	\brief waits until touch controller registers touch 
 */
void touch_wait_for_data(void);
/**
 *	\brief Reads values from controller
 *
 *	Run touch_wait_for_data() before this function;
 */
touch_t touch_get_data(void);

} /* end of namespace display */

#endif /* TOUCH_H_ */