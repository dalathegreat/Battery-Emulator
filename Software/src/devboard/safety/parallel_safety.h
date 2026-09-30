#ifndef PARALLEL_SAFETY_H
#define PARALLEL_SAFETY_H

#include <stdint.h>

/**
 * @brief Safety checks for parallel-connected secondary batteries.
 *
 * Called once per second. Publishes whether the specified secondary battery's
 * latest voltage reading is within 1.5V of the primary battery's, in
 * datalayer.system.status.batteryN_voltage_matches (false while either has no
 * reading), and raises a warning event once they have been apart for more than
 * 3 seconds.
 *
 * @param[in] batteryNumber The battery to check (2 or 3)
 */
void check_parallel_battery_safety(uint8_t batteryNumber);

#endif
