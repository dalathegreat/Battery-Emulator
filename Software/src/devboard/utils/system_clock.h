#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <stdint.h>

/* The ESP32's system clock, in UTC, which its RTC keeps running. Battery Emulator has no network
   time, so only the inverter sets it, from the clock it sends once an hour (BYD HVS/HVM over Modbus
   and Fronius Generic Storage). Until the first time arrives the clock holds no real time, and
   clock_valid is false. */
extern bool clock_valid;

// Sets the clock to utc_s, in Unix seconds, and marks it valid. A time before 2025 is not a real
// date but an inverter clock that was never set, so it is ignored. Returns whether the clock was set
bool clock_set_utc(uint64_t utc_s);

// The clock in Unix seconds, or 0 while it is not valid
uint64_t clock_utc_s();

#endif
