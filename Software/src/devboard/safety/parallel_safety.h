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

#ifdef UNIT_TEST
/**
 * @brief Put the module back into its power-on state. Test-only.
 *
 * The sentinel latch and the out-of-sync second counters persist for the
 * lifetime of the process, which is correct in firmware and untestable in a
 * test binary: without this, every case after the first inherits whatever
 * latch state its predecessors left behind, and no case can exercise the
 * startup grace at all.
 */
void reset_parallel_safety_state();
#endif

#endif
