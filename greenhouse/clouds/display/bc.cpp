/**   
 * \file bc.cpp
 * \date 2013-4-27
 * \brief Brightness controller
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

#include <aery32/gpio.h>
#include <aery32/delay.h>
#include "lcd.h"
#include "bc.h"

namespace display {}


void display::set_brightness(int val)
{
 		aery::gpio_set_pin_low( DISPLAY_LED );
		aery::delay_ms(1);		

  if(val <= 0) return;
  if(val > 31) val = 31;
  for(int i = 31; i >= val; i--)
  {
    			aery::gpio_set_pin_low( DISPLAY_LED );
				__asm__("nop");
				aery::gpio_set_pin_high( DISPLAY_LED );
				__asm__("nop");
  }
}
