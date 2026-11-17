#include "system_clock.h"

bool clock_valid = false;

static constexpr uint64_t EARLIEST_UTC_S = 1735689600;  // 2025-01-01 00:00:00 UTC

#ifdef UNIT_TEST
#include <Arduino.h>  // millis(), from test/emul

/* The host's clock is not the tests' to set. They get one of their own instead, which runs on
   millis() like the rest of the emulated time. */
static uint64_t test_clock_s = 0;
static uint32_t test_clock_set_ms = 0;

static bool write_clock(uint64_t utc_s) {
  test_clock_s = utc_s;
  test_clock_set_ms = millis();
  return true;
}

static uint64_t read_clock() {
  return test_clock_s + (uint32_t)(millis() - test_clock_set_ms) / 1000;
}
#else
#include <sys/time.h>  // settimeofday()
#include <time.h>      // time()

static bool write_clock(uint64_t utc_s) {
  struct timeval now = {};
  now.tv_sec = (time_t)utc_s;
  return settimeofday(&now, nullptr) == 0;
}

static uint64_t read_clock() {
  return (uint64_t)time(nullptr);
}
#endif

bool clock_set_utc(uint64_t utc_s) {
  if (utc_s < EARLIEST_UTC_S || !write_clock(utc_s)) {
    return false;
  }
  clock_valid = true;
  return true;
}

uint64_t clock_utc_s() {
  return clock_valid ? read_clock() : 0;
}
