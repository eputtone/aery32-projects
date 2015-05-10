/**   
 * \file lcd.cpp
 * \date 2013-4-27
 * \brief lcd
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
#include "gfx.h"

/* 
	The display driver chip has a bug that does not allow updating the 
	memory access registers with the same values already in them.
	Therefor the current values are stored and checked on each call of
	the area_set() method.
*/
unsigned int write_area_x_begin = 0,
write_area_y_begin = 0,
write_area_x_end = 0,
write_area_y_end = 0;

unsigned char screen_orientation = 0x02;

int display::lcd_read_reg( unsigned short reg )
{
	unsigned short temp;
	aery::gpio_set_pin_low( CS );
	aery::gpio_set_pin_low( RS ); // write to register
	send_data_lcd( reg );
	aery::gpio_set_pin_high( RS ); //done
	
	aery::gpio_set_pin_low( RD ); // read register
	aery::gpio_init_pins(aery::portb, DATAMASK, GPIO_INPUT);
	temp = AVR32_GPIO.port[ 1 ].pvr & DATAMASK;
	aery::gpio_set_pin_high( RD ); // done
	aery::gpio_set_pin_high( CS );
	aery::gpio_init_pins(aery::portb, DATAMASK, GPIO_OUTPUT);
	
	return temp;
}

void display::lcd_set_reg( unsigned char add, unsigned short val )
{	
	aery::gpio_set_pin_low( CS );
	aery::gpio_set_pin_low( RS ); // write to register
	send_data_lcd( add );
	aery::gpio_set_pin_high( RS ); // write to data
	send_data_lcd( val );
	aery::gpio_set_pin_high( CS );
}

void display::lcd_init( void )
{	
	aery::delay_ms(10);
	aery::gpio_init_pins( aery::portb, 0xffffffff, GPIO_OUTPUT );
	aery::gpio_init_pin( FMARK, GPIO_INPUT );
	aery::gpio_set_pin_high( RD ); // set, nothing to be read
	aery::gpio_set_pin_high( RS ); // set
	aery::gpio_set_pin_high( CS ); // set
	aery::gpio_set_pin_high( RST );
	aery::delay_ms(10);
	
	//PANEL INTERFACE SETTINGS
	lcd_set_reg( 0x17, 0x01 ); // clocks per line to min
	lcd_set_reg( 0x2E, 0x00 );
	lcd_set_reg( 0x2F, 0x00 );
	lcd_set_reg( 0x30, 0x00 );
	lcd_set_reg( 0x31, 0x00 );
	lcd_set_reg( 0x32, 0x00 );
	
	//set_reg_lcd( 0x23, 0x01 ); // Fmark (TE) On

	// POWER SETTINGS:	
	lcd_set_reg( 0x1A, 0x57 ); // pc2 bt-setting no 1
	lcd_set_reg( 0x1B, 0x01 ); // pc3 1 x VCILVL
	lcd_set_reg( 0x1C, 0x1E ); // pc4 PON? VREGOUT1 x 1.32
	lcd_set_reg( 0x1D, 0x0F ); // pc5 VCIR x 1.6,
	lcd_set_reg( 0x8A, 0x07 ); // Power driving reg
	lcd_set_reg( 0x85, 0x07 );
	//set_reg( 0x58, 0x80 );
	
	lcd_set_reg( 0x1F, 0x75 ); // 75 vcom
	lcd_set_reg( 0x19, 0x12 ); // PC1 PSON and PON
	aery::delay_ms(50);
	lcd_set_reg( 0x1E, 0x36 ); // pc6
	aery::delay_ms(150);

	//DISPLAY SETTINGS
	lcd_set_reg( 0x55, 0x0F ); //RGB and Frame inversion (Panel Control register)
	lcd_set_reg( 0x3A, 0xC0 );
	lcd_set_reg( 0x3C, 0xFF ); // nsap
	lcd_set_reg( 0x3D, 0xFF ); // isap
	lcd_set_reg( 0x24, 0x3C ); // display enable
	lcd_set_reg( 0x91, 0x01 );
	
	//RAM SETTINGS:
	lcd_set_orientation( 2 );
	
	// LED on
	aery::gpio_set_pin_high( DISPLAY_LED );
	
	// Show test image to verify settings:
	test_image();
}

void display::lcd_area_reset()
{
	if( (screen_orientation&0x20) ) lcd_area_set( 0, 0, 399, 239 ); // landscape
	else lcd_area_set( 0, 0, 239, 399 ); // portrait
}

void display::lcd_area_set( unsigned int xb, unsigned int yb, unsigned int xe, unsigned int ye )
{
	// move ram pointer if needed
	if( write_area_x_begin != xb )
	{
		lcd_set_reg( 0x02, (xb>>8) );
		lcd_set_reg( 0x03, xb );
		write_area_x_begin = xb;
	}
	
	if ( write_area_y_begin != yb )
	{
		lcd_set_reg( 0x06, (yb>>8) );
		lcd_set_reg( 0x07, yb );
		write_area_y_begin = yb;
	}
	
	if( write_area_x_end != xe )
	{
		lcd_set_reg( 0x04, (xe>>8) );
		lcd_set_reg( 0x05, xe);
		write_area_x_end = xe;
	}
	
	if( write_area_y_end != ye )
	{
		lcd_set_reg( 0x08, (ye)>>8 );
		lcd_set_reg( 0x09, (ye) );
		write_area_y_end = ye;
	}
}

void display::lcd_set_orientation( unsigned char mode )
{
	//Ram access settings:
	switch(mode) 
	{
		case 1:
			screen_orientation = 0xC0;
			break;
		case 2:
			screen_orientation = 0xA0;
			break;
		case 3:
			screen_orientation = 0x60;
			break;
		default:
			screen_orientation = 0x00;
			break;
	}
	lcd_set_reg(0x16, screen_orientation);
	lcd_area_reset();
}
