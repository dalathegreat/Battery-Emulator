#include "hal.h"

#include "../../../USER_SETTINGS.h"

#include <Arduino.h>
#include "hw_3LB.h"
#include "hw_devkit.h"
#include "hw_lilygo.h"
#include "hw_stark.h"

Esp32Hal* esp32hal = nullptr;

void init_hal() {
#if defined(HW_LILYGO)
  esp32hal = new LilyGoHal();
#elif defined(HW_STARK)
  esp32hal = new StarkHal();
#elif defined(HW_3LB)
  esp32hal = new ThreeLBHal();
#elif defined(HW_DEVKIT)
  esp32hal = new DevKitHal();
#else
#error "No HW defined."
#endif
}

bool Esp32Hal::system_booted_up() {
  return milliseconds(millis()) > BOOTUP_TIME();
}
