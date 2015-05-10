/**   
 * \file gfx.cpp
 * \date 2013-4-27
 * \brief Graphics
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

#include <aery32/gpio.h>
#include <stdio.h>
#include <stdarg.h>
#include "font.h"
#include "gfx.h"
#include "lcd.h"
#include "sdcard.h"

BYTE ImageBuffer[GFX_BUFFER_SIZE];
extern unsigned char screen_orientation;

void display::test_image() {
	lcd_area_reset();
	aery::gpio_set_pin_low(CS);
	aery::gpio_set_pin_low(RS); // write to register
	send_data_lcd(0x22);
	aery::gpio_set_pin_high(RS); // write to data

	int i;
	for (i = 0; i < 12000; i++) {
		send_data_lcd(0);
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd(RED);
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd(YELLOW);
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd(GREEN);
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd((GREEN | BLUE));
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd(BLUE);
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd((RED | BLUE));
	}
	for (i = 0; i < 12000; i++) {
		send_data_lcd(WHITE);
	}

	aery::gpio_set_pin_high(CS);
}

void display::set_pixel(unsigned int x,
		unsigned int y,
		unsigned int color) {
	lcd_area_set(x, y, x, y);
	lcd_set_reg(0x22, color);
}

unsigned short display::read_pixel(unsigned int x, unsigned int y)
{
	lcd_area_set(x,y,x,y);
	lcd_read_reg(0x22); // dummy read
	return lcd_read_reg(0x22);
}

void display::draw_rectangle(unsigned int x,
		unsigned int y,
		unsigned int width,
		unsigned int height,
		unsigned int color) {
	display::fill_rectangle(x, y, 1, height, color);
	display::fill_rectangle(x, y, width, 1, color);
	display::fill_rectangle(x + width - 1, y, 1, height, color);
	display::fill_rectangle(x, y + height - 1, width, 1, color);

}

void display::fill_rectangle(unsigned int x,
		unsigned int y,
		unsigned int width,
		unsigned int height,
		unsigned int color) {
	lcd_area_set(x, y, x + width - 1, y + height - 1);
	enable_fast_transfer();
	aery::gpio_set_pin_low(CS);
	aery::gpio_set_pin_low(RS); // write to register
	send_data_lcd_fast(0x22);
	aery::gpio_set_pin_high(RS); // write to data

	unsigned int i, len = height * width;
	for (i = 0; i < len; i++) {
		send_data_lcd_fast(color);
	}
	disable_fast_transfer();
	lcd_area_reset();
}

void display::draw_line(unsigned int x1,
		unsigned int y1,
		unsigned int x2,
		unsigned int y2,
		unsigned int color) {
	int xofs,
			yofs,
			dx, //deltas
			dy,
			dx2, //scaled deltas
			dy2,
			ix, //increase rate on the x axis
			iy, //increase rate on the y axis
			err, //the error term
			i; //looping variable

	// difference between starting and ending points
	dx = x2 - x1;
	dy = y2 - y1;

	// calculate direction of the vector and store in ix and iy
	if (dx >= 0) ix = 1;
	if (dx < 0) {
		ix = -1;
		dx = -dx;
	}

	if (dy >= 0) iy = 1;
	if (dy < 0) {
		iy = -1;
		dy = -dy;
	}

	// scale deltas and store in dx2 and dy2
	dx2 = dx << 1;
	dy2 = dy << 1;
	xofs = 0;
	yofs = 0;
	if (dx > dy) { // dx is the major axis
		// initialize the error term
		err = dy2 - dx;

		for (i = 0; i <= dx; i++) {
			set_pixel(x1 + xofs, y1 + yofs, color);
			if (err >= 0) {
				err -= dx2;
				yofs += iy;
			}
			err += dy2;
			xofs += ix;
		}
	} else { // dy is the major axis
		// initialize the error term
		err = dx2 - dy;

		for (i = 0; i <= dy; i++) {
			set_pixel(x1 + xofs, y1 + yofs, color);
			if (err >= 0) {
				err -= dy2;
				xofs += ix;
			}
			err += dx2;
			yofs += iy;
		}
	}
}

// Draw circle function, courtesy of MIT 
// http://groups.csail.mit.edu/graphics/classes/6.837/F98/Lecture6/circle.html

void display::draw_circle(unsigned int x,
		unsigned int y,
		int radius,
		unsigned int color) {
	int offset = 0;
	int p = (5 - (radius << 2)) >> 2;

	display::set_pixel(x, y + radius, color);
	display::set_pixel(x, y - radius, color);
	display::set_pixel(x + radius, y, color);
	display::set_pixel(x - radius, y, color);

	while (offset < radius) {
		offset++;
		if (p < 0) p += (offset << 1) + 1;
		else {
			radius--;
			p += ((offset - radius) << 1) + 1;
		}
		if (offset == radius) {
			display::set_pixel(x + offset, y + radius, color);
			display::set_pixel(x - offset, y + radius, color);
			display::set_pixel(x + offset, y - radius, color);
			display::set_pixel(x - offset, y - radius, color);
		}
		if (offset < radius) {
			display::set_pixel(x + offset, y + radius, color);
			display::set_pixel(x - offset, y + radius, color);
			display::set_pixel(x + offset, y - radius, color);
			display::set_pixel(x - offset, y - radius, color);
			display::set_pixel(x + radius, y + offset, color);
			display::set_pixel(x - radius, y + offset, color);
			display::set_pixel(x + radius, y - offset, color);
			display::set_pixel(x - radius, y - offset, color);
		}
	}
}

void display::fill_circle(unsigned int x,
		unsigned int y,
		int radius,
		unsigned int color) {
	int offset = 0;
	int p = (5 - (radius << 2)) >> 2;

	display::set_pixel(x, y + radius, color);
	display::set_pixel(x, y - radius, color);
	display::set_pixel(x + radius, y, color);
	display::set_pixel(x - radius, y, color);

	while (offset < radius) {
		offset++;
		if (p < 0) p += (offset << 1) + 1;
		else {
			radius--;
			p += ((offset - radius) << 1) + 1;
		}
		if (offset < radius) {
			display::draw_line(x + offset, y + radius, x - offset, y + radius, color);
			display::draw_line(x + offset, y - radius, x - offset, y - radius, color);
			display::draw_line(x + radius, y + offset, x + radius, y - offset, color);
			display::draw_line(x - radius, y - offset, x - radius, y + offset, color);
		}
	}
	display::fill_rectangle(x - radius, y - offset, radius + radius + 1, offset + offset + 1, color);
}

void display::show_image(unsigned int x,
		unsigned int y,
		unsigned int width,
		unsigned int height,
		const char *path) {
	// buffer needs to be divisible by 3 and 2,
	// because pixels are 1, 2 or 3 byte long:
	unsigned int buffersize =
			((GFX_BUFFER_SIZE >> 1) - ((GFX_BUFFER_SIZE >> 1) % 3)) << 1;

	// file system:
	FATFS fs;
	FIL file;
	FRESULT rc;

	// image properties:
	int bitsPerPixel = 0,
			bytesPerPixel = 0,
			rowSize = 0,
			rowDataSize = 0,
			bmpWidth,
			bmpHeight,
			padding = 0,
			rowByteCounter = 0;
	unsigned short colorTable[256];

	// control:
	unsigned int bytesRead = 0,
			k = 0,
			offset = 0,
			pixel = 0,
			limit = 0;
	bool first = true,
			rgb1555 = false;

	// mount volume and open file:
	rc = f_mount(CARD_DRIVE, &fs);
	rc = f_open(&file, path, FA_READ);

	// check current screen orientation:
	if ((screen_orientation & 0x20)) // landscape
	{
		lcd_set_reg(0x16, (screen_orientation^0x40));
		lcd_area_set(x, 240 - y - height, x + width - 1, 240 - y - 1);
	} else // portrait
	{
		lcd_set_reg(0x16, (screen_orientation^0x80));
		lcd_area_set(x, 400 - y - height, x + width - 1, 400 - y - 1);
	}

	// begin data transfer:
	aery::gpio_set_pin_low(CS);
	aery::gpio_set_pin_low(RS); // write to register
	send_data_lcd(0x22);
	aery::gpio_set_pin_high(RS); // write to data

	while (!f_read(&file, ImageBuffer, buffersize, &bytesRead)) {
		if (bytesRead == 0) break; //eof
		if (first) // get the bmp-header information from the first block
		{
			first = false; // deactivate

			// extract offset:
			offset = ImageBuffer[0xD];
			offset <<= 8;
			offset |= ImageBuffer[0xC];
			offset <<= 8;
			offset |= ImageBuffer[0xB];
			offset <<= 8;
			offset |= ImageBuffer[0xA];

			// other image properties:
			bmpWidth = *(&ImageBuffer[0x12]);
			bmpHeight = *(&ImageBuffer[0x16]);
			bitsPerPixel = ImageBuffer[0x1C];
			bytesPerPixel = (bitsPerPixel >> 3);
			rowSize = ((bitsPerPixel * bmpWidth + 31) >> 5) << 2;
			rowDataSize = (bytesPerPixel * bmpWidth);
			padding = rowSize - rowDataSize;

			// check red color mask for 16bit images:
			rgb1555 = (ImageBuffer[0x37] != 0xF8);

			// get color table for 8bit images:
			if (bitsPerPixel == 8) {
				int colorTablePosition = 14 + ImageBuffer[0x0E];
				for (int i = 0; i < 256; i++) {
					pixel = (
							(ImageBuffer[ colorTablePosition + (i << 2) + 2 ] << 16) |
							(ImageBuffer[ colorTablePosition + (i << 2) + 1 ] << 8) |
							(ImageBuffer[ colorTablePosition + (i << 2) ])
							);
					colorTable[i] = (
							((pixel & 0xF80000) >> 8) |
							((pixel & 0x00FC00) >> 5) |
							((pixel & 0x0000FF) >> 3)
							);
				}
			}

		} else offset = 0; // for all other blocks
		// Transmit image data to the screen:
		k = offset;
		limit = (bytesRead - bytesPerPixel + 1);
		enable_fast_transfer();
		while (k < limit) {
			switch (bitsPerPixel) {
				case 8:
					send_data_lcd_fast(colorTable[ ImageBuffer[k] ]);
					break;
				case 16:
					pixel = (
							((ImageBuffer[ k + 1 ]) << 8) |
							(ImageBuffer[ k ])
							);

					if (rgb1555) pixel = (
							((pixel & 0x7C00) << 1) |
							((pixel & 0x3E0) << 1) |
							(pixel & 0x1F)
							);

					send_data_lcd_fast(pixel);
					break;
				case 24:
					pixel = (
							(ImageBuffer[ k + 2 ] << 16) |
							(ImageBuffer[ k + 1 ] << 8) |
							(ImageBuffer[ k ])
							);
					send_data_lcd_fast((
							((pixel & 0xF80000) >> 8) |
							((pixel & 0x00FC00) >> 5) |
							((pixel & 0x0000FF) >> 3)
							));
					break;
			}
			rowByteCounter += bytesPerPixel;
			if (rowByteCounter >= rowDataSize) // new row
			{
				rowByteCounter = 0;
				k += padding;
			}
			k += bytesPerPixel;
		}
		disable_fast_transfer();
	}

	rc = f_close(&file);
	lcd_area_reset();
	lcd_set_reg(0x16, screen_orientation);
}

void display::print_text(int x,
		int y,
		unsigned int color,
		int size,
		const char* txt,
		...) {
	char unsigned *cp, mask, col;
	char buffer[256];

	va_list args;
	va_start(args, txt);
	vsprintf(buffer, txt, args);
	va_end(args);

	int k, j, i, offset, index;
	for (k = 0, cp = (unsigned char *) buffer; *cp != '\0'; cp++, k++) {
		switch (*cp) { // character special handling
			case 'g':
			case 'p':
			case 'q':
			case 'y':
				offset = 1;
				break;

			case '\n': // new line
				y += (size << 4)-size;
				k = -1;
				break;

			default:
				offset = 0;
				break;
		}
		
		if(*cp < 32) continue; // skip non-printable characters
		
		for (i = 0; i < 5; i++) {
			index = *cp - 32;
			if (*cp > 0xA0) index -= 32; // the table lacks some characters
			col = Latin1[ index ][i];
			mask = 1;
			for (j = 0; j < 8; j++) {
				if (mask & col) {
					fill_rectangle(x + (i * size)+(k * 6 * size),
							y + (offset * size)+(j * size),
							size,
							size,
							color);
				}
				mask <<= 1;
			}
		}
	}
}

