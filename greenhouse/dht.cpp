#include "dht.h"

using namespace aery;

namespace dht {
    double humidity;
    double temperature;
    uint8_t bits[5];  // buffer to receive data    
}

double dht::getTemperature() {
    return temperature;
}

double dht::getHumidity() {
    return humidity;
}

// return values:
// DHTLIB_OK
// DHTLIB_ERROR_CHECKSUM
// DHTLIB_ERROR_TIMEOUT
int dht::read11(uint8_t pin)
{
    // READ VALUES
    int rv = _readSensor(pin, DHTLIB_DHT11_WAKEUP);
    if (rv != DHTLIB_OK)
    {
        humidity    = DHTLIB_INVALID_VALUE; // invalid value, or is NaN prefered?
        temperature = DHTLIB_INVALID_VALUE; // invalid value
        return rv;
    }

    // CONVERT AND STORE
    humidity    = dht::bits[0];  // bits[1] == 0;
    temperature = bits[2];  // bits[3] == 0;

    // TEST CHECKSUM
    // bits[1] && bits[3] both 0
    uint8_t sum = bits[0] + bits[2];
    if (bits[4] != sum) return DHTLIB_ERROR_CHECKSUM;

    return DHTLIB_OK;
}

/////////////////////////////////////////////////////
//
// PRIVATE
//

// return values:
// DHTLIB_OK
// DHTLIB_ERROR_TIMEOUT
int dht::_readSensor(uint8_t pin, uint8_t wakeupDelay)
{
    // INIT BUFFERVAR TO RECEIVE DATA
    uint8_t mask = 128;
    uint8_t idx = 0;

    // EMPTY BUFFER
    for (uint8_t i = 0; i < 5; i++) {
        bits[i] = 0;
    }

    // REQUEST SAMPLE
    gpio_init_pin(pin, GPIO_OUTPUT|GPIO_LOW);
    delay_ms(wakeupDelay);
    gpio_set_pin_high(pin);
    delay_us(40);
    gpio_init_pin(pin, GPIO_INPUT);

    // GET ACKNOWLEDGE or TIMEOUT
    uint16_t loopCntLOW = DHTLIB_TIMEOUT;
    while (!gpio_read_pin(pin))  // T-rel
    {
        if (--loopCntLOW == 0) return -1;
    }

    uint16_t loopCntHIGH = DHTLIB_TIMEOUT;
    while (gpio_read_pin(pin))  // T-reh
    {
        if (--loopCntHIGH == 0) return -2;
    }

    // READ THE OUTPUT - 40 BITS => 5 BYTES
    for (uint8_t i = 40; i != 0; i--)
    {
        loopCntLOW = DHTLIB_TIMEOUT;
        while (!gpio_read_pin(pin))
        {
            if (--loopCntLOW == 0) return -3;
        }

        delay_us(30);
        if (gpio_read_pin(pin))
        { 
            bits[idx] |= mask;
        }
        loopCntHIGH = DHTLIB_TIMEOUT;
        while (gpio_read_pin(pin))
        {
            if (--loopCntHIGH == 0) return -4;
        }

        mask >>= 1;
        if (mask == 0)   // next byte?
        {
            mask = 128;
            idx++;
        }
    }
    gpio_init_pin(pin, GPIO_OUTPUT|GPIO_HIGH);
    return DHTLIB_OK;
}
