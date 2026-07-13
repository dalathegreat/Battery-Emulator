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
const char* get_charging_status_text(int32_t current_dA, bool inverter_limits_charge, bool inverter_limits_discharge,
                                      bool user_settings_limit_charge, bool user_settings_limit_discharge) {
  if (current_dA == 0) {
    return "Battery idle";
  }
  if (current_dA < 0) {
    if (inverter_limits_discharge) {
      return "Battery discharging! (Inverter limiting)";
    }
    if (user_settings_limit_discharge) {
      return "Battery discharging! (Settings limiting)";
    }
    return "Battery discharging! (Battery limiting)";
  }
  if (inverter_limits_charge) {
    return "Battery charging! (Inverter limiting)";
  }
  if (user_settings_limit_charge) {
    return "Battery charging! (Settings limiting)";
  }
  return "Battery charging! (Battery limiting)";
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
