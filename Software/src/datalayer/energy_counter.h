#ifndef ENERGY_COUNTER_H
#define ENERGY_COUNTER_H

#include <stdint.h>

// After setup_battery(), before starting the core task. Native counters remain driver-owned.
void init_energy_counters(uint32_t now_ms);

// Central 10 ms path: integer integration only, no flash access.
void update_energy_counters(uint32_t now_ms);

// Central 1 s path: opt-in checkpoint every 24 hours of uptime.
// ENERGYPERSIST defaults off and is read only by init_energy_counters().
void store_energy_counters(uint32_t now_ms);

#endif
