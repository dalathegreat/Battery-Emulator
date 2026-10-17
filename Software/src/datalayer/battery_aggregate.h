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

#ifndef SMALL_FLASH_DEVICE
/**
 * @brief Whether a current sensor fitted in place of the batteries' own (the QNHCK2-16) has a
 * reading to stand in with.
 */
static inline bool shunt_replaces_battery_current() {
  return datalayer.shunt.replaces_battery_current && datalayer.shunt.available;
}

/**
 * @brief What that sensor measured, in deciAmpere: rounded half away from zero, and held within
 * int16_t.
 */
static inline int16_t shunt_current_dA() {
  const int32_t mA = datalayer.shunt.measured_amperage_mA;
  const int32_t dA = (mA >= 0) ? (mA + 50) / 100 : (mA - 50) / 100;
  return (int16_t)(dA > INT16_MAX ? INT16_MAX : (dA < INT16_MIN ? INT16_MIN : dA));
}

/**
 * @brief Whether that sensor stands in for pack 1's current: it has a reading, and pack 1 is the
 * whole installation.
 */
static inline bool shunt_measures_battery1() {
  return datalayer.system.info.configured_batteries < 2 && shunt_replaces_battery_current();
}
#endif  // SMALL_FLASH_DEVICE

/**
 * @brief A pack's current, as everything but its own driver uses and shows it.
 *
 * The datalayer keeps what each battery reports. A current sensor fitted in place of their own
 * measures the whole installation, so while it has a reading it stands in for pack 1 when pack 1
 * is the installation. With several packs each keeps its own, since the sensor only measures
 * their sum: datalayer.aggregate and reported_current_dA take that instead.
 *
 * @param[in] status datalayer.battery.status, .battery2.status or .battery3.status
 * @return The current in deciAmpere, positive while charging
 */
#ifndef SMALL_FLASH_DEVICE
int16_t pack_current_dA(const DATALAYER_BATTERY_STATUS_TYPE& status);  // Never inline: see battery_aggregate.cpp
#else
static inline int16_t pack_current_dA(const DATALAYER_BATTERY_STATUS_TYPE& status) {
  return status.current_dA;
}
#endif  // SMALL_FLASH_DEVICE

/**
 * @brief A pack's power, the same way: from the sensor's current where pack_current_dA() takes
 * that.
 *
 * @param[in] status datalayer.battery.status, .battery2.status or .battery3.status
 * @return The power in Watts, positive while charging
 */
#ifndef SMALL_FLASH_DEVICE
int32_t pack_power_W(const DATALAYER_BATTERY_STATUS_TYPE& status);  // Never inline: see battery_aggregate.cpp
#else
static inline int32_t pack_power_W(const DATALAYER_BATTERY_STATUS_TYPE& status) {
  return status.active_power_W;
}
#endif  // SMALL_FLASH_DEVICE

/**
 * @brief The current of the whole installation: the sensor's while it has a reading, else the
 * sum of what the packs report (0 for the ones not used).
 *
 * @return The current in deciAmpere, positive while charging
 */
static inline int16_t installation_current_dA() {
#ifndef SMALL_FLASH_DEVICE
  if (shunt_replaces_battery_current()) {
    return shunt_current_dA();
  }
#endif  // SMALL_FLASH_DEVICE
  return datalayer.battery.status.current_dA + datalayer.battery2.status.current_dA +
         datalayer.battery3.status.current_dA;
}

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
