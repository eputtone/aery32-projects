#ifndef dht_h
#define dht_h

#include <inttypes.h>
#include <aery32/delay.h>
#include <aery32/rtc.h>
#include <aery32/gpio.h>

#define DHTLIB_OK                0
#define DHTLIB_ERROR_CHECKSUM   -1
#define DHTLIB_ERROR_TIMEOUT    -2
#define DHTLIB_INVALID_VALUE    -999

#define DHTLIB_DHT11_WAKEUP     18
#define DHTLIB_DHT_WAKEUP       1

#define DHTLIB_TIMEOUT (60000)

namespace dht {
    // return values: DHTLIB_OK, DHTLIB_ERROR_CHECKSUM, DHTLIB_ERROR_TIMEOUT
    int read11(uint8_t pin);
    double getTemperature();
    double getHumidity();

    int _readSensor(uint8_t pin, uint8_t wakeupDelay);
}
#endif
