#ifndef RESTART_WATCHDOG_H
#define RESTART_WATCHDOG_H

/* Makes esp_restart() hand the RTC watchdog over with its counter at zero, so the ROM and the bootloader
   get the whole second esp_restart_noos() means to give them. Call once from setup(). */
void register_restart_watchdog_feed();

#endif
