#ifndef BATTERY_AGGREGATE_H
#define BATTERY_AGGREGATE_H

#include "datalayer.h"

/**
 * @brief Remember the charge/discharge power this pack's BMS asked for.
 *
 * Call once per cycle, right after the pack's driver has run and before anything downstream
 * rewrites max_charge_power_W / max_discharge_power_W.
 *
 * @param[in,out] pack datalayer.battery, .battery2 or .battery3
 */
/**
 * @brief Convert a power limit to a current limit, rounding to nearest.
 *
 * Truncating here costs a whole deciAmpere on the way back out of an A -> W -> A round trip:
 * a 19.0 A ceiling becomes 6697 W at 352.5 V, and 6697 W truncates back to 18.9 A. Every
 * conversion in the limit chain goes through this so the number the user typed survives.
 *
 * @param[in] power_W    The power limit
 * @param[in] voltage_dV Pack voltage in deciVolt. Must be non-zero
 * @return The equivalent current in deciAmpere
 */
static inline uint32_t power_W_to_current_dA(uint32_t power_W, uint16_t voltage_dV) {
  return (((uint64_t)power_W * 100) + (voltage_dV / 2)) / voltage_dV;
}

void snapshot_bms_limits(DATALAYER_BATTERY_TYPE& pack);

/**
 * @brief Scale one pack's SOC and capacity into its own reported_ fields.
 *
 * Applies the system-wide SOC window (min_percentage / max_percentage) to the pack handed in,
 * so every configured pack carries its own scaled figures instead of pack 1's.
 *
 * @param[in,out] pack The pack to scale. datalayer.battery, .battery2 or .battery3
 */
void scale_pack_values(DATALAYER_BATTERY_TYPE& pack);

/**
 * @brief Roll every configured pack up into datalayer.aggregate.
 *
 * Call once per second, after every pack has been scaled. Fills everything except the four
 * limit fields, which update_aggregate_limits() owns.
 */
void update_aggregate_values();

/**
 * @brief Fill the charge/discharge limits in datalayer.aggregate.
 *
 * Call last in the update chain, after the safety layer, the SOC taper and the low pass filter
 * have shaped datalayer.battery, and before the inverter is updated.
 */
void update_aggregate_limits();

#endif  // BATTERY_AGGREGATE_H
