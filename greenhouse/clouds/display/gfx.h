/**   
 * \file gfx.h
 * \date 2013-4-27
 * \brief Graphics
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

#ifndef GFX_H_
#define GFX_H_

// Define buffer size used by show_image():
#define GFX_BUFFER_SIZE 12000 // in Bytes

// color values:
#define RED		0xF800
#define GREEN	0x07E0
#define BLUE	0x001F
#define YELLOW	0xFFE0
#define BLACK	0x0000
#define WHITE	0xFFFF

// Uncommend following line, you dont use FATFS:
// #define NO_FATFS

namespace display {

	typedef union // 16bit color
	{
		unsigned int u;

		struct {
			unsigned int : 16;
			unsigned int r : 5;
			unsigned int g : 6;
			unsigned int b : 5;
		} rgb;
	} color16_t;

	/**
	 *	\brief Display colored stripes on the screen
	 */
	void test_image(void);
	/**
	 *	\brief Changes the color of the specified pixel
	 */
	void set_pixel(unsigned int x,
			unsigned int y,
			unsigned int color
			);
	/**
	 * \brief Read pixel from screen
     * \param x
     * \param y
     * \return 
     */
	unsigned short read_pixel(unsigned int x, unsigned int y);
	/**
	 *	\brief Prints text on the screen
	 *	Works almost like printf() in C
	 */
	void print_text(int x,
			int y,
			unsigned int color,
			int size,
			const char* txt,
			...
			);

	/**
	 *	\brief Draw a line on the screen
	 *   line is drawn from (x1,y1) to (x2,y2)
	 */
	void draw_line(unsigned int x1, // start point x
			unsigned int y1, // start point y
			unsigned int x2, // end point x
			unsigned int y2, // end point y
			unsigned int color
			);

	/**
	 *	\brief Draw a circle on the screen
	 *   circle center point is (x,y) 
	 */
	void draw_circle(unsigned int x, // center point x
			unsigned int y, // center point y
			int radius, //  circle radius
			unsigned int color
			);

	/**
	 *	\brief Fill a circle on the screen
	 *   circle center point is (x,y) 
	 */
	void fill_circle(unsigned int x, // center point x
			unsigned int y, // center point y
			int radius, //  circle radius
			unsigned int color
			);

	/**
	 *	\brief Draw a rectangle with the specified color
	 */
	void draw_rectangle(unsigned int x,
			unsigned int y,
			unsigned int width,
			unsigned int height,
			unsigned int color
			);
	/**
	 *	\brief Draw and fill a rectangle with the specified color
	 */
	void fill_rectangle(unsigned int x,
			unsigned int y,
			unsigned int width,
			unsigned int height,
			unsigned int color
			);
#ifndef NO_FATFS
	/**
	 *	\brief Displays an Windows BMP file on the screen.
	 *	Supported formats:	8bit (uncompressed)
	 *						16bit (x-5-5-5 and 5-6-5)
	 *						24bit
	 *	Currently no scaling is supported, so height and width have to match the
	 *	files height and with. 
	 */
	void show_image(unsigned int x,
			unsigned int y,
			unsigned int width,
			unsigned int height,
			const char *path
			);
#endif

} /* end of namespace display */

#endif /* GFX_H_ */