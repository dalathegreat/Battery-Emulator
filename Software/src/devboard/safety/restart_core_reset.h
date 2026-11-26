#ifndef RESTART_CORE_RESET_H
#define RESTART_CORE_RESET_H

/* On the ESP32-S3, makes esp_restart() end in a core reset instead of the CPU reset esp_restart_noos()
   ends in; does nothing on other chips. Call once from setup(), before anything starts Wi-Fi: the
   shutdown handler it adds never returns, and esp_restart() runs its handlers newest first, so it has to
   be older than the one Wi-Fi adds. */
void register_restart_core_reset();

#endif
