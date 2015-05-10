/**   
 * \file bc.h
 * \date 2013-4-27
 * \brief Brightness controller
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

#ifndef BC_H_
#define BC_H_

namespace display {

/**
 *	\brief Decrease background LED brightness
 *	There are 31 brightness steps. 31 being Full brightness
 *	and 0 being LED off. Decreasing 1 step from 0 will jump 
 *	brightness back to 31.
 *	\param steps 
 */

void set_brightness( int val );

}

#endif /* BC_H_ */