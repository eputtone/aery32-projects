#include "dht.h"
#include <aery32/all.h>
#include "board.h"
#include <display/all.h>
#include <display/fatfs/ff.h>
#include <math.h>

using namespace aery;

#define HISTORY_SIZE 220
#define TICKS_PER_HOUR 2

const char* REFRESH_BUTTON_TEXT = "REFRESH";
const char* PUMP_BUTTON_TEXT = "PUMP";
const char* NEXT_BUMP_TEXT = "Next pump\nafter: ";
const char* EMPTY_TANK_TEXT = "Empty tank: ";
const char* FULL_TANK_TEXT = "Tank has water";
const char* HOUR_TEXT = "h";
const char* CELCIUS_TEXT = "C";
const char* PERCENTAGE_TEXT = "P";
const char* PLUS_TEXT = "+";
const char* MINUS_TEXT = "-";
const char* TEMPERATURE_HIGH_TEXT = "+50C";
const char* TEMPERATURE_LOW_TEXT = "0C";
const char* HUMIDITY_HIGH_TEXT = "90P";
const char* HUMIDITY_LOW_TEXT = "20P";

const uint32_t TICK = 5*115000/2;

int temperatureHistory[HISTORY_SIZE];
int humidityHistory[HISTORY_SIZE];
int current_hour;
uint32_t ticks_between_pumps = 24 * TICKS_PER_HOUR; 
uint32_t current_tick = 0;
uint32_t next_pump_ticks_left = ticks_between_pumps;
uint32_t empty_tank_since_ticks = 0;

void draw_background() 
{
	char str[20] = "";
	char tmp[10] = "";
	display::fill_rectangle(0, 0, 400, 240, WHITE);

	// draw actions buttons
	display::draw_rectangle(10, 127, 150, 45, BLACK);
	display::print_text(20, 140, BLACK, 3, REFRESH_BUTTON_TEXT);

	display::draw_rectangle(10, 177, 150, 45, BLACK);
	display::print_text(40, 190, BLACK, 3, PUMP_BUTTON_TEXT);

	strcat(str, NEXT_BUMP_TEXT);
	dtoa(((double)next_pump_ticks_left)/TICKS_PER_HOUR, 2, tmp);
	strcat(str, tmp);
	strcat(str, HOUR_TEXT);
	display::print_text(170, 140, BLACK, 2, str);
	display::draw_rectangle(355, 125, 35, 35, BLACK);
	display::print_text(365, 135, BLACK, 3, PLUS_TEXT);
	display::draw_rectangle(355, 160, 35, 35, BLACK);
	display::print_text(365, 167, BLACK, 3, MINUS_TEXT);

	display::print_text(5, 7, GREEN, 1, TEMPERATURE_HIGH_TEXT);
	display::draw_line(30, 10, 250, 10, GREEN);
	display::print_text(5, 58, RED, 1, TEMPERATURE_LOW_TEXT);
	display::draw_line(30, 60, 250, 60, RED);

	display::print_text(5, 78, GREEN, 1, HUMIDITY_HIGH_TEXT);
	display::draw_line(30, 80, 250, 80, GREEN);
	display::print_text(5, 113, RED, 1, HUMIDITY_LOW_TEXT);
	display::draw_line(30, 115, 250, 115, RED);

	for (int i=current_hour-1, j=0; j<HISTORY_SIZE; j++, i--)
	{
		int value = temperatureHistory[i % HISTORY_SIZE];
		if (value>=0 && value<=50)
		{
			display::set_pixel(30+j, 58-value, BLACK);
		}
	}
	for (int i=current_hour-1, j=0; j<HISTORY_SIZE; j++, i--)
	{
		int value = humidityHistory[i % HISTORY_SIZE];
		if (value>=0 && value<=35)
		{
			display::set_pixel(30+j, 115-value, BLACK);
		}
	}
}

void pump_water()
{
	if (!gpio_read_pin(WATER_FLOAT))
	{
		gpio_set_pin_high(PUMP);
		delay_ms(20000);
		gpio_set_pin_low(PUMP);
	}
}

void update_dht11_values(void)
{
	char tmp[10];
	display::fill_rectangle(270, 0, 129, 120, WHITE);
	dht::read11(DHT11);

	dtoa(dht::getTemperature(), 1, tmp);
	display::print_text(280, 30, BLACK, 3, strcat(tmp, CELCIUS_TEXT));

	dtoa(dht::getHumidity(), 1, tmp);
	display::print_text(280, 80, BLACK, 3, strcat(tmp, PERCENTAGE_TEXT));
}

void update_water_float_status(void)
{
	char str[20] = "";
	char tmp[10] = "";
	bool no_water = gpio_read_pin(WATER_FLOAT);
	display::fill_rectangle(165, 200, 234, 39, WHITE);
	if (no_water)
	{
		strcat(str, EMPTY_TANK_TEXT);
		dtoa(((double)empty_tank_since_ticks)/TICKS_PER_HOUR, 2, tmp);
		strcat(str, tmp);
		strcat(str, HOUR_TEXT);
		display::print_text(170, 205, BLACK, 2, str);
	}
	else
	{
		empty_tank_since_ticks = 0;
		display::print_text(170, 205, BLACK, 2, FULL_TANK_TEXT);		
	}
}

void save_history(void)
{
	temperatureHistory[current_hour % HISTORY_SIZE] = (int)round(dht::getTemperature());
	humidityHistory[current_hour % HISTORY_SIZE] = (int)(round(dht::getHumidity()/2.0))-10;
	current_hour++;
}

void isrhandler_rtc(void)
{	
	draw_background();
	update_dht11_values();
	update_water_float_status();
	current_tick++;
	if (current_tick % TICKS_PER_HOUR == 0)
	{
		save_history();
	}
	if (next_pump_ticks_left == 0)
	{
		next_pump_ticks_left = ticks_between_pumps;
		pump_water();
	}
	else
	{
		next_pump_ticks_left--;
	}
	//rtc_clear_interrupt();
	//rtc_set_value(0);
}

bool is_pump_touch_event(display::touch_t event) 
{
	return event.x > 10 && event.x < 160 && event.y > 177 && event.y < 222;
}

bool is_refresh_touch_event(display::touch_t event) 
{
	return event.x > 10 && event.x < 160 && event.y > 127 && event.y < 172;
}

bool is_increase_pump_delay(display::touch_t event) 
{
	return event.x > 355 && event.x < 390 && event.y > 125 && event.y < 160;
}

bool is_decrease_pump_delay(display::touch_t event) 
{
	return event.x > 355 && event.x < 390 && event.y > 160 && event.y < 195;
}

int main(void)
{
	display::touch_t current, last;
	for (int i=0; i<HISTORY_SIZE; i++)
	{
		temperatureHistory[i] = -1;
		humidityHistory[i] = -1;
	}

	board::init();
	display::lcd_init();
	display::touch_init();

	rtc_init(RTC_SOURCE_RC, 0, 0, TICK);
	//intc_init();
	//intc_register_isrhandler(&isrhandler_rtc, 1, 0);
	//intc_enable_globally();
	rtc_enable(false);

	isrhandler_rtc();

	for(;;) 
	{
		display::touch_enable();
		//display::touch_wait_for_data();
		while (true)
		{
			if (AVR32_RTC.val == TICK)
			{
				isrhandler_rtc();
				rtc_init(RTC_SOURCE_RC, 0, 0, TICK);
				rtc_enable(false);
			}
			if (aery::gpio_read_pin(SDO))
			{
				break;
			}
		}
		current = display::touch_get_data();
		display::touch_disable();
		if (is_pump_touch_event(current))
		{
			pump_water();
		}
		else if (is_refresh_touch_event(current))
		{
			;
		}
		else if (is_increase_pump_delay(current)) 
		{
			next_pump_ticks_left += TICKS_PER_HOUR;
		}
		else if (is_decrease_pump_delay(current))
		{
			if (next_pump_ticks_left < TICKS_PER_HOUR)
			{
				next_pump_ticks_left = 0;
			}
			else
			{
				next_pump_ticks_left = next_pump_ticks_left - TICKS_PER_HOUR;
			}
		}
		draw_background();
		update_dht11_values();
		update_water_float_status();
	}
	return 0;
}
