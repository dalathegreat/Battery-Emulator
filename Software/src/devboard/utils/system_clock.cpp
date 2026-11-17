#include "system_clock.h"
#include <stddef.h>  // offsetof()
#include "logging.h"

bool clock_valid = false;

static constexpr uint64_t EARLIEST_UTC_S = 1735689600;  // 2025-01-01 00:00:00 UTC
static constexpr uint64_t US_PER_S = 1000000;

/* After a restart the time is worked out again only if the inverter sent it at most this long before.
   The RTC counter runs on an RC oscillator, which drifts with temperature, so the error grows with
   how long it has counted alone. ESP-IDF restores the system time by itself too, but from how long
   the RTC counter says the emulator has run since power-on, which after weeks can be hours off. */
static constexpr uint64_t RESTORE_WITHIN_US = 2 * 3600 * US_PER_S;

/* The time the clock was last set to, and the RTC counter at that moment. On the ESP32 this is in
   RTC memory, which a restart of the firmware leaves alone. After a power cycle it holds whatever
   the memory came up with, which the checksum tells apart. No default member values: a constructor
   would clear it at every start. */
struct ClockAnchor {
  uint64_t utc_us;
  uint64_t rtc_ticks;
  uint32_t magic;
  uint32_t checksum;
};
static constexpr uint32_t ANCHOR_MAGIC = 0x434C4B31;  // "CLK1"

#ifdef UNIT_TEST
#include "millis64.h"

/* The host's clock is not the tests' to set, so they get one of their own. The RTC counter is a
   1 kHz one on the emulated time: a test restarts the firmware by clearing clock_valid, and
   power-cycles it by also taking the emulated time back below where the clock was set. */
static ClockAnchor anchor;
static uint64_t test_clock_us = 0;
static uint64_t test_clock_set_ms = 0;

static uint64_t rtc_ticks() {
  return millis64();
}

static uint64_t rtc_ticks_to_us(uint64_t ticks) {
  return ticks * 1000;
}

static bool write_clock(uint64_t utc_us) {
  test_clock_us = utc_us;
  test_clock_set_ms = millis64();
  return true;
}

static uint64_t read_clock_s() {
  return (test_clock_us + (millis64() - test_clock_set_ms) * 1000) / US_PER_S;
}
#else
#include <esp_attr.h>  // RTC_NOINIT_ATTR
#include <soc/rtc.h>   // rtc_time_get(), rtc_clk_cal(), rtc_time_slowclk_to_us()
#include <sys/time.h>  // settimeofday()
#include <time.h>      // time()

static RTC_NOINIT_ATTR ClockAnchor anchor;

static uint64_t rtc_ticks() {
  return rtc_time_get();
}

// At the rate the RTC clock runs at now, measured afresh. If that fails, the result is too long to
// restore from
static uint64_t rtc_ticks_to_us(uint64_t ticks) {
  const uint32_t period = rtc_clk_cal(RTC_CAL_RTC_MUX, 1024);
  return period ? rtc_time_slowclk_to_us(ticks, period) : UINT64_MAX;
}

static bool write_clock(uint64_t utc_us) {
  struct timeval now = {};
  now.tv_sec = (time_t)(utc_us / US_PER_S);
  now.tv_usec = (suseconds_t)(utc_us % US_PER_S);
  return settimeofday(&now, nullptr) == 0;
}

static uint64_t read_clock_s() {
  return (uint64_t)time(nullptr);
}
#endif

static uint32_t anchor_checksum() {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&anchor);
  uint32_t hash = 2166136261u;  // FNV-1a
  for (size_t i = 0; i < offsetof(ClockAnchor, checksum); i++) {
    hash = (hash ^ bytes[i]) * 16777619u;
  }
  return hash;
}

bool clock_set_utc(uint64_t utc_s) {
  if (utc_s < EARLIEST_UTC_S || !write_clock(utc_s * US_PER_S)) {
    return false;
  }
  anchor.utc_us = utc_s * US_PER_S;
  anchor.rtc_ticks = rtc_ticks();
  anchor.magic = ANCHOR_MAGIC;
  anchor.checksum = anchor_checksum();
  clock_valid = true;
  return true;
}

uint64_t clock_utc_s() {
  return clock_valid ? read_clock_s() : 0;
}

void clock_restore() {
  if (anchor.magic != ANCHOR_MAGIC || anchor.checksum != anchor_checksum()) {
    return;  // Never set, or a power cycle
  }
  const uint64_t ticks = rtc_ticks();
  if (ticks < anchor.rtc_ticks) {
    return;  // The RTC counter started again from 0, so a power cycle after all
  }
  const uint64_t since_us = rtc_ticks_to_us(ticks - anchor.rtc_ticks);
  if (since_us > RESTORE_WITHIN_US || !write_clock(anchor.utc_us + since_us)) {
    return;
  }
  clock_valid = true;
  logging.printf("Clock kept through the restart, %u s after the inverter last sent its time\n",
                 (unsigned)(since_us / US_PER_S));
}
