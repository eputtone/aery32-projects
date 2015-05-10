#ifndef __BOARD_H
#define __BOARD_H

#include <aery32/all.h>

#define ADC_VREF 3.0
#define ADC_BITS 10

#define PUMP AVR32_PIN_PB29
#define DHT11 AVR32_PIN_PA04
#define WATER_FLOAT AVR32_PIN_PA08

namespace board {

	void init(void);

	inline double cnv_to_volt(unsigned int cnv)
	{
		return cnv * (ADC_VREF / (1UL << ADC_BITS));
	}

} /* end of namespace board */

#endif
