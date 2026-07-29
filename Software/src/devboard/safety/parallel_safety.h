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

#endif
