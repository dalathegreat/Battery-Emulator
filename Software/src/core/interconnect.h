#ifndef INTERCONNECT_H
#define INTERCONNECT_H

#include <stdint.h>

/**
 * @brief Check if battery voltages are close enough to allow interconnection.
 *
 * Called once per second. Compares the voltage of the specified secondary
 * battery against the primary battery. If voltages drift apart by more than
 * 1.5V for longer than 10 seconds, the secondary battery is disconnected.
 *
 * @param[in] batteryNumber The battery to check (2 or 3)
 */
void check_interconnect_available(uint8_t batteryNumber);

#endif
