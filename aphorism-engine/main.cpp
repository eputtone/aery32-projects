/** 
 * Turns your aery32 display cloud into an aphorism engine that
 * displays aphorisms from a text file.
 *
 * Prepare SD card by:
 * 1) Fetch file that contains one-line aphorisms e.g. from: http://silgro.com/fortunes.txt
 *    and save it in root folder as APHO.TXT
 * 2) Create 64 x 48 BMP file in root folder as BULP.BMP (supported 8bit (uncompressed),
 *    16bit (x-5-5-5 and 5-6-5), 24bit)
 */

#include <aery32/all.h>
#include "board.h"
#include <display/all.h>
#include <display/fatfs/ff.h>

using namespace aery;

const char* APHORISM_FN = "APHO.TXT";
const char* BULP_FN = "BULP.BMP";
TCHAR* FILE_READ_ERROR = "  Error reading file!";
const uint MAX_APHORISM_SIZE_CHARS = 90;
const uint LINE_LENGTH = 20;

// draw background stripes from top black to bottom gray 
// without overwriting the right lower corner picture
void draw_background() 
{
	unsigned int r = 0, g = 0, b = 0, color = 0, x_end = 399;
	for (int i = 0; i < 240; i++) 
	{
		color = ((r % 16) << 11) | ((g % 32) << 5) | (b % 16);
		if (i == 192) 
		{
			x_end = 336;
		}
		display::draw_line(0, i, x_end, i, color);
		if (i % 16 == 0) 
		{
			r++;
			g = g + 2;
			b++;
		}
	}
}

bool should_break_line(TCHAR* output_buffer, uint obi, uint pbi) 
{
	if (pbi >= LINE_LENGTH / 2) 
	{
		// break the line after half of the screen width is exceeded, unless
		// the next word fits the remaining space
  		for (uint i = obi + 1; (i < MAX_APHORISM_SIZE_CHARS) && ((i - obi) < (LINE_LENGTH - pbi)); i++) 
  		{
  	  		if (output_buffer[i] == ' ' || output_buffer[i] == '\n') 
  	  		{
  	  			return false;
  	  		}
  	  	}
  	  	return true;
  	}
  	return pbi > LINE_LENGTH - 2;
}

int main(void)
{
	board::init();
	display::lcd_init();

	FATFS fs;
	FIL file;
	
	TCHAR* output_buffer;
	TCHAR temp_buffer[MAX_APHORISM_SIZE_CHARS];
	TCHAR print_buffer[LINE_LENGTH+1];
	
	// draw 64 x 48 light bulp to lower right corner
	display::show_image(336, 192, 64, 48, BULP_FN);

	f_mount(CARD_DRIVE, &fs);
	f_open(&file, APHORISM_FN, FA_OPEN_EXISTING | FA_READ);
	int file_row, pbi;
	for(;;) 
	{
		file_row = 0;
		pbi = 0;
		output_buffer = f_gets(temp_buffer, MAX_APHORISM_SIZE_CHARS, &file);
		if (f_error(&file)) 
			output_buffer = FILE_READ_ERROR;
		else if (f_eof(&file)) 
		{
			// start from the beginning when run out of aphorisms
			f_close(&file);
			f_open(&file, APHORISM_FN, FA_OPEN_EXISTING | FA_READ);
			continue;
		}		
		draw_background();
		// skip first 2 chars of aphorism as they contain the type of aphorism
		for (uint i = 2; i < (MAX_APHORISM_SIZE_CHARS-1) && output_buffer[i] != '\0'; i++) 
		{
			print_buffer[pbi++] = output_buffer[i];
			if (should_break_line(output_buffer, i, pbi) || (output_buffer[i+1] == '\0'))
			{
				display::print_text(20, 20 + (file_row++) * 30, WHITE, 3, print_buffer);
				for (int j = 0; j < LINE_LENGTH; j++)
				{
					print_buffer[j] = '\0';
				}
				if (output_buffer[i+1] == ' ')
				{
					// do not start new row by space
					i++;
				}
				pbi = 0;
			}
		}
		delay_ms(10000);
	}
	return 0;
}
