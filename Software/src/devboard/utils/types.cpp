#include "types.h"

// Function to get string representation of system_status_enum
std::string getBMSStatus(system_status_enum status) {
  switch (status) {
    case STANDBY:
      return "STANDBY";
    case INACTIVE:
      return "INACTIVE";
    case ACTIVE:
      return "ACTIVE";
    case FAULT:
      return "FAULT";
    case UPDATING:
      return "UPDATING";
    default:
      return "UNKNOWN";
  }
}
ChargingState get_charging_state(int32_t current_dA) {
  if (current_dA == 0) {
    return ChargingState::Idle;
  }
  return current_dA < 0 ? ChargingState::Discharging : ChargingState::Charging;
}

LimitingFactor get_limiting_factor(ChargingState state, bool inverter_limits_charge, bool inverter_limits_discharge,
                                   bool user_settings_limit_charge, bool user_settings_limit_discharge) {
  if (state == ChargingState::Discharging) {
    if (inverter_limits_discharge) {
      return LimitingFactor::Inverter;
    }
    if (user_settings_limit_discharge) {
      return LimitingFactor::UserSetting;
    }
    return LimitingFactor::Battery;
  }
  if (state == ChargingState::Charging) {
    if (inverter_limits_charge) {
      return LimitingFactor::Inverter;
    }
    if (user_settings_limit_charge) {
      return LimitingFactor::UserSetting;
    }
    return LimitingFactor::Battery;
  }
  return LimitingFactor::None;
}

const char* charging_state_to_text(ChargingState state) {
  switch (state) {
    case ChargingState::Charging:
      return "Charging";
    case ChargingState::Discharging:
      return "Discharging";
    default:
      return "Idle";
  }
}

const char* limiting_factor_to_text(LimitingFactor factor) {
  switch (factor) {
    case LimitingFactor::Inverter:
      return "Inverter";
    case LimitingFactor::UserSetting:
      return "UserSetting";
    case LimitingFactor::Battery:
      return "Battery";
    default:
      return "Idle";
  }
}

const char* get_charging_status_text(int32_t current_dA, bool inverter_limits_charge, bool inverter_limits_discharge,
                                     bool user_settings_limit_charge, bool user_settings_limit_discharge) {
  ChargingState state = get_charging_state(current_dA);
  if (state == ChargingState::Idle) {
    return "Battery idle";
  }
  LimitingFactor factor = get_limiting_factor(state, inverter_limits_charge, inverter_limits_discharge,
                                              user_settings_limit_charge, user_settings_limit_discharge);
  if (state == ChargingState::Discharging) {
    switch (factor) {
      case LimitingFactor::Inverter:
        return "Battery discharging! (Inverter limiting)";
      case LimitingFactor::UserSetting:
        return "Battery discharging! (Settings limiting)";
      default:
        return "Battery discharging! (Battery limiting)";
    }
  }
  switch (factor) {
    case LimitingFactor::Inverter:
      return "Battery charging! (Inverter limiting)";
    case LimitingFactor::UserSetting:
      return "Battery charging! (Settings limiting)";
    default:
      return "Battery charging! (Battery limiting)";
  }
}

#ifdef HW_LILYGO2CAN
GPIOOPT1 user_selected_gpioopt1 = GPIOOPT1::DEFAULT_OPT;
#endif
GPIOOPT2 user_selected_gpioopt2 = GPIOOPT2::DEFAULT_OPT_BMS_POWER_18;
GPIOOPT3 user_selected_gpioopt3 = GPIOOPT3::DEFAULT_SMA_ENABLE_05;
GPIOOPT4 user_selected_gpioopt4 = GPIOOPT4::DEFAULT_SD_CARD;
#ifdef HW_STARK
GPIOOPT5 user_selected_gpioopt5 = GPIOOPT5::DEFAULT_BMS_POWER_23;
#endif
#ifdef HW_WAVESHARE
GPIOOPT6 user_selected_gpioopt6 = GPIOOPT6::DEFAULT_STATUS_LED;
#endif
