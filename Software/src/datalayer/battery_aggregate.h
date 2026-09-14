#ifndef BATTERY_AGGREGATE_H
#define BATTERY_AGGREGATE_H

#include "datalayer.h"

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
