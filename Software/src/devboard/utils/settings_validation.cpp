#include "settings_validation.h"
#include "../../datalayer/datalayer.h"

bool validate_soc_window(int32_t min_pptt, int32_t max_pptt) {
  if (min_pptt < SOC_WINDOW_MIN_FLOOR_PPTT) {
    return false;
  }
  if (max_pptt > SOC_WINDOW_MAX_CEIL_PPTT) {
    return false;
  }
  if (min_pptt + SOC_WINDOW_MIN_GAP_PPTT > max_pptt) {
    return false;
  }
  return true;
}

bool set_soc_window(int32_t min_pptt, int32_t max_pptt) {
  if (!validate_soc_window(min_pptt, max_pptt)) {
    return false;
  }
  datalayer.battery.settings.min_percentage = static_cast<int16_t>(min_pptt);
  datalayer.battery.settings.max_percentage = static_cast<uint16_t>(max_pptt);
  return true;
}
