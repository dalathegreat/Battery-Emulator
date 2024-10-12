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
