#include "../include.h"

#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "RS485Battery.h"

// These functions adapt the old C-style global functions battery-API to the
// object-oriented battery API.

// The instantiated class is defined by the pre-compiler define
// to support battery class selection at compile-time
#ifdef SELECTED_BATTERY_CLASS

Battery* battery = nullptr;
Battery* battery2 = nullptr;

void setup_battery() {
  // Instantiate the battery only once just in case this function gets called multiple times.
  if (battery == nullptr) {
    battery = new SELECTED_BATTERY_CLASS();
  }
  battery->setup();

#ifdef DOUBLE_BATTERY
  if (battery2 == nullptr) {
#if defined(BMW_I3_BATTERY)
    battery2 =
        new SELECTED_BATTERY_CLASS(&datalayer.battery2, &datalayer.system.status.battery2_allowed_contactor_closing,
                                   can_config.battery_double, WUP_PIN2);
#elif defined(KIA_HYUNDAI_64_BATTERY)
    battery2 = new SELECTED_BATTERY_CLASS(&datalayer.battery2, &datalayer_extended.KiaHyundai64_2,
                                          &datalayer.system.status.battery2_allowed_contactor_closing,
                                          can_config.battery_double);
#elif defined(SANTA_FE_PHEV_BATTERY) || defined(TEST_FAKE_BATTERY)
    battery2 = new SELECTED_BATTERY_CLASS(&datalayer.battery2, can_config.battery_double);
#else
    battery2 = new SELECTED_BATTERY_CLASS(&datalayer.battery2, nullptr, can_config.battery_double);
#endif
  }
  battery2->setup();
#endif
}

#endif
