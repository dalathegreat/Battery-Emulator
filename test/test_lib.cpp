#include "test_lib.h"

#include <cstdint>

MySerial Serial;

unsigned long testlib_millis = 0;

uint8_t bms_status = ACTIVE;

uint8_t LEDcolor = GREEN;
