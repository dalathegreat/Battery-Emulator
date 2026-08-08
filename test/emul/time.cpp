#include <stdint.h>

uint64_t current_time = 0;

uint32_t millis() {
  // Match the ESP32: millis() is 32 bits wide and wraps at 2^32 ms (49.7 days).
  // Production time variables are uint32_t, so the host runs exactly the
  // arithmetic the target runs, wrap included.
  return static_cast<uint32_t>(current_time);
}

uint64_t get_timestamp(unsigned long millis) {
  return 0;
}

uint64_t millis64(void) {
  return current_time;
}

void set_millis64(uint64_t time) {
  current_time = time;
}
