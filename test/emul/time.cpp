#include <stdint.h>

uint64_t current_time = 0;

unsigned long millis() {
  return static_cast<unsigned long>(current_time);
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
