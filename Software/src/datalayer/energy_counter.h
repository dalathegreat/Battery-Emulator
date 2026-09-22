#ifndef ENERGY_COUNTER_H
#define ENERGY_COUNTER_H

#include <stdint.h>

// Call after setup_battery(), before starting the main loop. Only calculated
// counters are restored; native lifetime totals remain owned by the battery.
void init_energy_counters(uint32_t now_ms);

// Central 10 ms path: integer integration only, no flash access.
void update_energy_counters(uint32_t now_ms);

// Central 1 s path: checkpoint calculated totals every 24 hours of uptime.
void store_energy_counters(uint32_t now_ms);

#endif
