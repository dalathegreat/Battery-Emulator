#include "energy_counter.h"

#include <algorithm>
#include <limits>
#include "../battery/BATTERIES.h"
#include "../communication/nvm/comm_nvm.h"
#include "datalayer.h"

namespace {
constexpr uint32_t CHECKPOINT_MS = 24UL * 60 * 60 * 1000;
constexpr uint64_t WH_DIVISOR = 360000000ULL;  // dV * dA * ms per Wh
constexpr uint64_t DAH_DIVISOR = 3600000ULL;   // dA * ms per 0.1 Ah
constexpr int32_t CALCULATED_COUNTER_CURRENT_DEADBAND_dA = 5;

struct CounterState {
  uint64_t charged_Wh_remainder = 0;
  uint64_t discharged_Wh_remainder = 0;
  uint64_t charged_dAh_remainder = 0;
  uint64_t discharged_dAh_remainder = 0;
  uint32_t previous_ms = 0;
  uint32_t checkpoint_ms = 0;
  bool initialized = false;
} counters;

template <typename T>
void accumulate(T& total, uint64_t& remainder, uint64_t increment, uint64_t divisor) {
  remainder += increment;
  const uint64_t whole = remainder / divisor;
  const uint64_t room = static_cast<uint64_t>(std::numeric_limits<T>::max()) - total;
  if (whole >= room) {
    total = std::numeric_limits<T>::max();
    remainder = 0;
  } else {
    total += static_cast<T>(whole);
    remainder %= divisor;
  }
}
}  // namespace

void init_energy_counters(uint32_t now_ms) {
  counters = {};
  counters.previous_ms = now_ms;
  counters.checkpoint_ms = now_ms;
  counters.initialized = true;
  if (!battery || (battery->supports_charged_energy() && battery->supports_directional_capacity())) {
    return;
  }
  BatteryEmulatorSettingsStore settings(true);
  auto& status = datalayer.battery.status;
  if (!battery->supports_charged_energy()) {
    status.total_charged_battery_Wh = std::max<int32_t>(0, settings.getInt("ENERGY_CHG_WH", 0));
    status.total_discharged_battery_Wh = std::max<int32_t>(0, settings.getInt("ENERGY_DIS_WH", 0));
  }
  if (!battery->supports_directional_capacity()) {
    status.total_charged_battery_dAh = settings.getUInt("CAP_CHG_DAH", 0);
    status.total_discharged_battery_dAh = settings.getUInt("CAP_DIS_DAH", 0);
  }
}

void update_energy_counters(uint32_t now_ms) {
  const uint32_t elapsed_ms = now_ms - counters.previous_ms;
  counters.previous_ms = now_ms;  // Never accumulate time spent behind an inactive gate.
  const auto& system = datalayer.system.status;
  auto& status = datalayer.battery.status;
  if (!counters.initialized || !battery || !status.CAN_battery_still_alive || system.system_status != ACTIVE ||
      !system.battery_allows_contactor_closing || system.contactors_engaged != 1 ||
      status.real_bms_status == BMS_FAULT || status.voltage_dV == 0) {
    return;
  }

  // Promote before negation, including INT16_MIN. All products fit in uint64_t
  // even for a full uint32_t elapsed interval and maximum voltage/current.
  const int32_t current_dA = status.reported_current_dA;
  if (current_dA >= -CALCULATED_COUNTER_CURRENT_DEADBAND_dA && current_dA <= CALCULATED_COUNTER_CURRENT_DEADBAND_dA) {
    return;
  }
  const uint64_t current_ms = static_cast<uint64_t>(current_dA < 0 ? -current_dA : current_dA) * elapsed_ms;
  if (!battery->supports_charged_energy()) {
    auto& total = current_dA >= 0 ? status.total_charged_battery_Wh : status.total_discharged_battery_Wh;
    auto& remainder = current_dA >= 0 ? counters.charged_Wh_remainder : counters.discharged_Wh_remainder;
    accumulate(total, remainder, current_ms * status.voltage_dV, WH_DIVISOR);
  }
  if (!battery->supports_directional_capacity()) {
    auto& total = current_dA >= 0 ? status.total_charged_battery_dAh : status.total_discharged_battery_dAh;
    auto& remainder = current_dA >= 0 ? counters.charged_dAh_remainder : counters.discharged_dAh_remainder;
    accumulate(total, remainder, current_ms, DAH_DIVISOR);
  }
}

void store_energy_counters(uint32_t now_ms) {
  if (!counters.initialized || !battery || now_ms - counters.checkpoint_ms < CHECKPOINT_MS) {
    return;
  }
  counters.checkpoint_ms = now_ms;
  if (battery->supports_charged_energy() && battery->supports_directional_capacity()) {
    return;
  }
  // Use the existing NVS wear levelling and changed-value-only settings writes.
  // Fractions and the checkpoint timer are intentionally volatile. A reboot
  // restores the last checkpoint and starts a new 24-hour runtime interval.
  BatteryEmulatorSettingsStore settings;
  const auto& status = datalayer.battery.status;
  if (!battery->supports_charged_energy()) {
    settings.saveInt("ENERGY_CHG_WH", status.total_charged_battery_Wh);
    settings.saveInt("ENERGY_DIS_WH", status.total_discharged_battery_Wh);
  }
  if (!battery->supports_directional_capacity()) {
    settings.saveUInt("CAP_CHG_DAH", status.total_charged_battery_dAh);
    settings.saveUInt("CAP_DIS_DAH", status.total_discharged_battery_dAh);
  }
}
