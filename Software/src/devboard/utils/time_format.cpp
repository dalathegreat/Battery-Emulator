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

// Shows only the significant units: "S seconds", "M minutes, S seconds",
// "H hours, M minutes" or "D days, H hours, M minutes".
String format_ms_string(uint64_t ms) {
  static const char* const unit[] = {"days", "hours", "minutes", "seconds"};
  TimeBreakdown t = ms_to_parts(ms);
  const unsigned long v[] = {t.days, t.hours, t.minutes, t.seconds};
  unsigned i = 0;
  while (i < 3 && v[i] == 0) {
    i++;
  }
  const unsigned last = (i < 2) ? 2 : 3;  // seconds are dropped once hours are shown
  char buf[48];                           // worst case "4294967295 days, 23 hours, 59 minutes, "
  int n = 0;
  for (; i <= last; i++) {
    n += snprintf(buf + n, sizeof(buf) - n, "%lu %s, ", v[i], unit[i]);
  }
  buf[n - 2] = '\0';  // drop the trailing separator
  return buf;
}
