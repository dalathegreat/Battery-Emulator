#include <stdint.h>
#include <cstdio>
#ifdef UNIT_TEST
#include "WString.h"
#else
#include <Arduino.h>
#endif
#include "time_format.h"

struct TimeBreakdown {
  unsigned long days;
  unsigned long hours;
  unsigned long minutes;
  unsigned long seconds;
};

static TimeBreakdown ms_to_parts(uint64_t ms) {
  uint64_t s = ms / 1000;
  return {
      .days = (unsigned long)(s / 86400),
      .hours = (unsigned long)((s % 86400) / 3600),
      .minutes = (unsigned long)((s % 3600) / 60),
      .seconds = (unsigned long)(s % 60),
  };
}

String format_ms_stamp(uint64_t ms) {
  TimeBreakdown t = ms_to_parts(ms);
  char buf[24];
  snprintf(buf, sizeof(buf), "%lud%02luh%02lum%02lus", t.days, t.hours, t.minutes, t.seconds);
  return buf;
}

String format_ms_string(uint64_t ms) {
  TimeBreakdown t = ms_to_parts(ms);
  char buf[64];
  snprintf(buf, sizeof(buf), "%lu days, %lu hours, %lu minutes, %lu seconds", t.days, t.hours, t.minutes, t.seconds);
  return buf;
}
