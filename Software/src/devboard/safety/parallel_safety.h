#ifndef PARALLEL_SAFETY_H
#define PARALLEL_SAFETY_H

#include <stdint.h>

/**
 * @brief Safety checks for parallel-connected secondary batteries.
 *
 * Called once per second. Verifies that the specified secondary battery
 * is safe to remain connected in parallel with the primary battery.
 * Currently checks voltage synchronization — if voltages drift apart
 * by more than 1.5V for longer than 10 seconds, the secondary battery
 * is disconnected.
 *
 * @param[in] batteryNumber The battery to check (2 or 3)
 */
void check_parallel_battery_safety(uint8_t batteryNumber);

/**
 * @brief Warn when a battery on the DC link stops carrying current while the others take over its
 * share: a tripped breaker, a blown fuse or a loose HV connection, which its BMS cannot report.
 *
 * Called once per second, after the packs have published their currents. Raises
 * EVENT_BATTERY_NO_CURRENT for that pack, a warning only, and clears it once the pack carries
 * current again. Does nothing with fewer than two batteries configured.
 */
void check_parallel_current_sharing(void);

#ifdef UNIT_TEST
// Test seam only: the check remembers the last 15 seconds of currents, and every test has to start
// without them.
void reset_parallel_current_sharing(void);
#endif

#endif
