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
 * Call once per second, after every pack has been scaled. Fills everything except the limit
 * fields, which combine_pack_limits() and update_aggregate_limits() own.
 */
void update_aggregate_values();

/**
 * @brief The charge current ceiling the user has asked for: the MQTT SET_LIMITS value while one
 * is active, otherwise the manual limit from the settings page.
 *
 * A remote limit replaces the manual one for as long as it lasts rather than combining with it,
 * so it may sit above it. Either way it only ever lowers what the batteries allow.
 *
 * @return The ceiling in deciAmpere
 */
static inline uint16_t effective_charge_cap_dA() {
  return datalayer.battery_settings.remote_settings_limit_charge ? datalayer.battery_settings.max_remote_set_charge_dA
                                                                 : datalayer.battery_settings.max_user_set_charge_dA;
}

/**
 * @brief The discharge counterpart of effective_charge_cap_dA().
 *
 * @return The ceiling in deciAmpere
 */
static inline uint16_t effective_discharge_cap_dA() {
  return datalayer.battery_settings.remote_settings_limit_discharge
             ? datalayer.battery_settings.max_remote_set_discharge_dA
             : datalayer.battery_settings.max_user_set_discharge_dA;
}

/**
 * @brief Convert a current ceiling to a power ceiling, rounding down.
 *
 * Rounding down keeps the power at or under the ceiling, and power_W_to_current_dA() rounds to
 * nearest on the way back, so the current the user typed comes out of the round trip unchanged.
 *
 * @param[in] current_dA The ceiling in deciAmpere
 * @param[in] voltage_dV Pack voltage in deciVolt
 * @return The equivalent power in Watts
 */
static inline uint32_t current_cap_to_power_W(uint16_t current_dA, uint16_t voltage_dV) {
  return ((uint32_t)current_dA * voltage_dV) / 100;
}

/**
 * @brief Passes - seconds - the inverter is given to follow a change of the parallel charge limit
 * before the packs' powers are measured against it again. See combine_pack_limits().
 */
static constexpr uint8_t PARALLEL_SETTLE_PASSES = 10;

/**
 * @brief Passes - seconds - a pack's charge limit has to stay up before a parallel raise counts on
 * the higher value. A lower one counts at once. See combine_pack_limits().
 */
static constexpr uint8_t PARALLEL_LIMIT_HOLD_PASSES = 60;

/**
 * @brief Passes - seconds - the installation may stop charging before the parallel raise starts
 * over from the lowest pack limit. A load the panels cannot cover for a while - a kettle, a cloud -
 * does not undo it. See combine_pack_limits().
 */
static constexpr uint16_t PARALLEL_STOP_PASSES = 300;

/**
 * @brief Whether parallel headroom tracking is in use: two or three batteries configured, and the
 * "Parallel headroom tracking" setting on.
 *
 * Without it the limits go the way they did before the tracking existed: the SOC taper and the low
 * pass filter shape pack 1's, and update_aggregate_limits() folds the joined pack 2 and 3 in after
 * them, as the lowest limit of any joined pack.
 */
static inline bool parallel_headroom_tracking_active() {
  return datalayer.system.info.configured_batteries > 1 && datalayer.battery_settings.parallel_headroom_tracking;
}

/**
 * @brief Fold the joined packs' power limits into datalayer.aggregate, while parallel headroom
 * tracking is in use.
 *
 * Call once per second, after the safety layer and before the SOC taper and the low pass filter,
 * which then shape the combined limits instead of pack 1's. Discharge gets the lowest joined
 * pack's limit. Charge starts there and is raised in steps, while the installation charges at it,
 * until the tightest pack takes its own limit. Does nothing with a single battery, or with the
 * tracking off: see parallel_headroom_tracking_active().
 */
void combine_pack_limits();

/**
 * @brief Fill the charge/discharge limits in datalayer.aggregate.
 *
 * Call last in the update chain, after the safety layer, combine_pack_limits(), the SOC taper and
 * the low pass filter, and before the inverter is updated. Both the power and the current limits
 * come out capped to effective_charge_cap_dA() / effective_discharge_cap_dA(), so a protocol
 * reading either one honours it. While forced recovery charge is on, its small charge current
 * replaces the charge limits, unless charging and discharging are paused.
 */
void update_aggregate_limits();

#endif  // BATTERY_AGGREGATE_H
