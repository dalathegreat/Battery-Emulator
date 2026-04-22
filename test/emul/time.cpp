#include <stdint.h>
#include <time.h>

uint64_t current_time = 0;
uint64_t start_time = 0;

uint64_t millis64(void) {
  if (current_time == UINT64_MAX) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - start_time;
  }

  return current_time;
}

unsigned long millis() {
  return static_cast<unsigned long>(millis64());
}

void set_millis64(uint64_t time) {
  current_time = time;
  if (time == UINT64_MAX) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    start_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  }
}
