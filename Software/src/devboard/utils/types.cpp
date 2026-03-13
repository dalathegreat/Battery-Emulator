#include "types.h"

// Function to get string representation of bms_status_enum
std::string getBMSStatus(bms_status_enum status) {
  switch (status) {
    case STANDBY:
      return "STANDBY";
    case INACTIVE:
      return "INACTIVE";
    case DARKSTART:
      return "DARKSTART";
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
#ifdef HW_LILYGO2CAN
GPIOOPT1 user_selected_gpioopt1 = GPIOOPT1::DEFAULT_OPT;
#endif
GPIOOPT2 user_selected_gpioopt2 = GPIOOPT2::DEFAULT_OPT_BMS_POWER_18;
GPIOOPT3 user_selected_gpioopt3 = GPIOOPT3::DEFAULT_SMA_ENABLE_05;
GPIOOPT4 user_selected_gpioopt4 = GPIOOPT4::DEFAULT_SD_CARD;
