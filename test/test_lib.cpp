#include "test_lib.h"

#include <cstdint>

#include "config.h"

MySerial Serial;

unsigned long testlib_millis = 0;

uint8_t bms_status = ACTIVE;
