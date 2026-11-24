#ifndef RESTART_TRACE_H
#define RESTART_TRACE_H

/* Diagnostic for RESET_WDT after restarts, ESP32-S3 only (no-ops elsewhere). Makes esp_restart() finish
   through a copy of IDF 5.5's esp_restart_noos() that notes each step it reaches in memory a watchdog
   reset leaves alone, and counts the app starts after it, so the next boot can tell where it stopped and,
   when it got through, how long the ROM and the bootloader took. The RTC watchdog gets ten seconds there,
   not one.

   register_restart_trace() from setup() before register_restart_watchdog_feed(), so the feed still runs
   first; restart_trace_report() once logging works, as it also resets the trace for the next restart. */
void register_restart_trace();
void restart_trace_report();

#endif
