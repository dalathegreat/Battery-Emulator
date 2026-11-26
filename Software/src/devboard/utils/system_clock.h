#ifndef SYSTEM_CLOCK_H
#define SYSTEM_CLOCK_H

#include <stdint.h>

/* The ESP32's system clock, in UTC. Battery Emulator has no network time, so only the inverter sets
   it, from the clock it sends once an hour (Fronius Generic Storage v1 and v2).
   Until the first time arrives the clock holds no real time, and clock_valid is false.

   A restart of the firmware, such as a reboot, a crash or the inverter's reboot command, keeps the
   clock if the inverter sent its time in the 2 hours before: the RTC counter runs on through the
   restart. A power cycle starts that counter again, so then the clock waits for the inverter. */
extern bool clock_valid;

// Sets the clock to utc_s, in Unix seconds, and marks it valid. A time before 2025 is not a real
// date but an inverter clock that was never set, so it is ignored. Returns whether the clock was set
bool clock_set_utc(uint64_t utc_s);

// The clock in Unix seconds, or 0 while it is not valid
uint64_t clock_utc_s();

// At startup: after a restart of the firmware, works the time out again from the RTC counter
void clock_restore();

#endif
