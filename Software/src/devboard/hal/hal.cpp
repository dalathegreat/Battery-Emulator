#include "hal.h"

#include "../../../USER_SETTINGS.h"

#include <Arduino.h>

Esp32Hal* esp32hal = nullptr;

void init_hal() {
#if defined(HW_LILYGO)
#include "hw_lilygo.h"
  esp32hal = new LilyGoHal();
#elif defined(HW_LILYGO2CAN)
#include "hw_lilygo2can.h"
  esp32hal = new LilyGo2CANHal();
#elif defined(HW_STARK)
#include "hw_stark.h"
  esp32hal = new StarkHal();
#elif defined(HW_3LB)
#include "hw_3LB.h"
  esp32hal = new ThreeLBHal();
#elif defined(HW_DEVKIT)
#include "hw_devkit.h"
  esp32hal = new DevKitHal();
#elif defined(HW_C6)
#include "hw_c6.h"
  esp32hal = new C6Hal();
#else
#error "No HW defined."
#endif
}

bool Esp32Hal::system_booted_up() {
  return milliseconds(millis()) > BOOTUP_TIME();
}
