#include <stdint.h>
#include <cstdio>
#ifdef UNIT_TEST
#include "WString.h"
#else
#include <Arduino.h>
#endif
#include "../i18n/tr.h"
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

/* The unit words come from the catalog rather than a format string. Upstream
   collected the two hand-rolled copies of this arithmetic into one helper,
   which happens to make this the single place the uptime text needs
   translating - so the conversion follows the helper instead of fighting it at
   each call site. format_ms_stamp() above stays untouched: it emits unit
   LETTERS (1d02h03m04s), which are not words and are not translated. */
String format_ms_string(uint64_t ms) {
  TimeBreakdown t = ms_to_parts(ms);
  return String(t.days) + " " + TR(TrKey::UI_DAYS) + ", " + String(t.hours) + " " + TR(TrKey::UI_HOURS) + ", " +
         String(t.minutes) + " " + TR(TrKey::UI_MINUTES) + ", " + String(t.seconds) + " seconds";
}
