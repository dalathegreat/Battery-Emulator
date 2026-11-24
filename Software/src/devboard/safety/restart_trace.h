#ifndef RESTART_TRACE_H
#define RESTART_TRACE_H

/* Diagnostic for RESET_WDT after restarts, ESP32-S3 only (no-ops elsewhere). Makes esp_restart() end in a
   core reset instead of esp_restart_noos()'s CPU reset, notes how far it got in memory a watchdog reset
   leaves alone, and counts the app starts after it, so the next boot can tell whether it got through
   and, when it did, how long the ROM and the bootloader took.

   register_restart_trace() from setup() before register_restart_watchdog_feed(), so the feed still runs
   first; restart_trace_report() once logging works, as it also resets the trace for the next restart. */
void register_restart_trace();
void restart_trace_report();

#endif
