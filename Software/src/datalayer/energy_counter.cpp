#include "energy_counter.h"

#include <algorithm>
#include <limits>
#include "../battery/BATTERIES.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../communication/nvm/comm_nvm.h"
#include "battery_aggregate.h"

namespace {
constexpr uint32_t CHECKPOINT_MS = 24UL * 60 * 60 * 1000;
constexpr uint64_t WH_DIVISOR = 360000000ULL;  // dV * dA * ms per Wh
constexpr uint64_t DAH_DIVISOR = 3600000ULL;   // dA * ms per 0.1 Ah
constexpr int32_t CALCULATED_COUNTER_CURRENT_DEADBAND_dA = 5;

// Pack 1 retains the v70 keys and types. Additional packs have independent totals
// in the same namespace; every key fits NVS's 15-character limit.
constexpr const char* KEYS[3][4] = {
    {"ENERGY_CHG_WH", "ENERGY_DIS_WH", "CAP_CHG_DAH", "CAP_DIS_DAH"},
    {"ENERGY_CHG_WH_2", "ENERGY_DIS_WH_2", "CAP_CHG_DAH_2", "CAP_DIS_DAH_2"},
    {"ENERGY_CHG_WH_3", "ENERGY_DIS_WH_3", "CAP_CHG_DAH_3", "CAP_DIS_DAH_3"},
};

struct CounterState {
  uint64_t charged_Wh_remainder = 0;
  uint64_t discharged_Wh_remainder = 0;
  uint64_t charged_dAh_remainder = 0;
  uint64_t discharged_dAh_remainder = 0;
} counters[3];
uint32_t previous_ms = 0;
uint32_t checkpoint_ms = 0;
bool initialized = false;
bool persistence_enabled = false;

Battery* pack_driver(unsigned index) {
  return index == 0 ? battery : (index == 1 ? battery2 : battery3);
}

DATALAYER_BATTERY_STATUS_TYPE& pack_status(unsigned index) {
  return index == 0 ? datalayer.battery.status : (index == 1 ? datalayer.battery2.status : datalayer.battery3.status);
}

bool needs_calculated_counters(Battery* driver) {
  return driver && (!driver->supports_charged_energy() || !driver->supports_directional_capacity());
}

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
  for (auto& counter : counters) {
    counter = {};
  }
  previous_ms = checkpoint_ms = now_ms;
  initialized = true;
  persistence_enabled = false;
  if (!needs_calculated_counters(battery) && !needs_calculated_counters(battery2) &&
      !needs_calculated_counters(battery3)) {
    return;
  }
  BatteryEmulatorSettingsStore settings(true);
  // Latch the setting at startup: a UI change requires a restart, so enabling
  // cannot overwrite an old checkpoint with this boot's RAM-only totals.
  persistence_enabled = settings.getBool("ENERGYPERSIST", false);
  if (!persistence_enabled) {
    return;
  }
  for (unsigned i = 0; i < 3; ++i) {
    auto* driver = pack_driver(i);
    if (!needs_calculated_counters(driver)) {
      continue;
    }
    auto& status = pack_status(i);
    if (!driver->supports_charged_energy()) {
      status.total_charged_battery_Wh = std::max<int32_t>(0, settings.getInt(KEYS[i][0], 0));
      status.total_discharged_battery_Wh = std::max<int32_t>(0, settings.getInt(KEYS[i][1], 0));
    }
    if (!driver->supports_directional_capacity()) {
      status.total_charged_battery_dAh = settings.getUInt(KEYS[i][2], 0);
      status.total_discharged_battery_dAh = settings.getUInt(KEYS[i][3], 0);
    }
  }
}

void update_energy_counters(uint32_t now_ms) {
  const uint32_t elapsed_ms = now_ms - previous_ms;
  previous_ms = now_ms;  // Never accumulate time spent behind an inactive gate.
  const auto& system = datalayer.system.status;
  if (!initialized || !battery || system.system_status != ACTIVE || !system.battery_allows_contactor_closing ||
      system.contactors_engaged != 1) {
    return;
  }
  for (unsigned i = 0; i < 3; ++i) {
    auto* driver = pack_driver(i);
    const bool joined = i == 0 || (i == 1 ? system.battery2_allowed_contactor_closing
                                        : system.battery3_allowed_contactor_closing);
    const bool contactor_engaged =
        i == 0 || (i == 1 ? !contactor_control_enabled_double_battery || system.contactors_battery2_engaged
                         : !contactor_control_enabled_triple_battery || system.contactors_battery3_engaged);
    auto& status = pack_status(i);
    if (!needs_calculated_counters(driver) || !joined || !contactor_engaged || !status.CAN_battery_still_alive ||
        status.real_bms_status == BMS_FAULT || status.voltage_dV == 0) {
      continue;
    }

    // Use measured pack voltage, never the aggregate's startup placeholder. In a
    // single-pack installation pack_current_dA() selects the replacement shunt.
    // With multiple packs it keeps each pack's own current: the installation
    // shunt cannot apportion its reading. reported_current_dA is already a sum.
    const int32_t current_dA = pack_current_dA(status);
    if (current_dA >= -CALCULATED_COUNTER_CURRENT_DEADBAND_dA && current_dA <= CALCULATED_COUNTER_CURRENT_DEADBAND_dA) {
      continue;
    }
    // Promote before negation, including INT16_MIN. Products plus remainders
    // fit uint64_t even for a full uint32_t elapsed interval.
    const uint64_t current_ms = static_cast<uint64_t>(current_dA < 0 ? -current_dA : current_dA) * elapsed_ms;
    auto& counter = counters[i];
    if (!driver->supports_charged_energy()) {
      auto& total = current_dA > 0 ? status.total_charged_battery_Wh : status.total_discharged_battery_Wh;
      auto& remainder = current_dA > 0 ? counter.charged_Wh_remainder : counter.discharged_Wh_remainder;
      accumulate(total, remainder, current_ms * status.voltage_dV, WH_DIVISOR);
    }
    if (!driver->supports_directional_capacity()) {
      auto& total = current_dA > 0 ? status.total_charged_battery_dAh : status.total_discharged_battery_dAh;
      auto& remainder = current_dA > 0 ? counter.charged_dAh_remainder : counter.discharged_dAh_remainder;
      accumulate(total, remainder, current_ms, DAH_DIVISOR);
    }
  }
}

void store_energy_counters(uint32_t now_ms) {
  if (!initialized || !persistence_enabled || now_ms - checkpoint_ms < CHECKPOINT_MS) {
    return;
  }
  checkpoint_ms = now_ms;
  for (unsigned i = 0; i < 3; ++i) {
    auto* driver = pack_driver(i);
    if (!needs_calculated_counters(driver)) {
      continue;
    }
    // Existing NVS wear levelling and changed-value-only writes. Fractions and
    // timers are volatile: reboot restores the checkpoint and starts a new day.
    BatteryEmulatorSettingsStore settings;
    const auto& status = pack_status(i);
    if (!driver->supports_charged_energy()) {
      settings.saveInt(KEYS[i][0], status.total_charged_battery_Wh);
      settings.saveInt(KEYS[i][1], status.total_discharged_battery_Wh);
    }
    if (!driver->supports_directional_capacity()) {
      settings.saveUInt(KEYS[i][2], status.total_charged_battery_dAh);
      settings.saveUInt(KEYS[i][3], status.total_discharged_battery_dAh);
    }
  }
}
