#include <Arduino.h>
#include "esp_timer.h"

uint64_t ARDUINO_ISR_ATTR millis64() {
  // ESP32's esp_timer_get_time() returns time in microseconds, we convert to
  // milliseconds by dividing by 1000.

  // This is almost identical to the existing Arduino millis() function, except
  // we return a 64-bit value which won't roll over for 600k years.

  return esp_timer_get_time() / 1000ULL;
}
