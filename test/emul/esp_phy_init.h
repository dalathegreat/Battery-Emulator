#pragma once

// Host stand-in for the ESP-IDF PHY calibration API. Only erase_phy_cal_data()
// uses it, to wipe the stored Wi-Fi calibration; there is no calibration data
// on the host, so the stub reports success.

#include <stdint.h>

typedef int esp_err_t;
#define ESP_OK 0

inline esp_err_t esp_phy_erase_cal_data_in_nvs(void) {
  return ESP_OK;
}
