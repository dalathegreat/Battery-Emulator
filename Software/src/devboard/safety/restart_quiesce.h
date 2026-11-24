#ifndef RESTART_QUIESCE_H
#define RESTART_QUIESCE_H

/* Makes esp_restart() park the other CPU in internal RAM, with its interrupts off, right before
   esp_restart_noos() resets it, so it has no PSRAM access in flight when the reset hits it.

   Call it from setup() before anything initialises Wi-Fi. esp_restart() runs its shutdown handlers
   newest first, so registered ahead of the one Wi-Fi adds (esp_wifi_stop()), this one runs after it:
   stopping Wi-Fi still needs both CPUs. Registers only when PSRAM is in use, the setup the hang has
   been seen on. */
void register_restart_quiesce();

#endif
