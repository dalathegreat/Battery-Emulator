#include "../battery/Battery.h"
#include "../devboard/hal/hal.h"
#include "../inverter/INVERTERS.h"
#include "BMW-SBOX.h"
#include "QNHCK2-16.h"
#include "Shunt.h"

Shunt* shunt = nullptr;
ShuntType user_selected_shunt_type = ShuntType::None;

#ifndef SMALL_FLASH_DEVICE
// Shunts read through an ADC pin are only offered on boards whose HAL routes one
static bool board_has_shunt_adc_pin() {
  return esp32hal && esp32hal->SHUNT_ADC_PIN() != GPIO_NUM_NC;
}
#endif  // SMALL_FLASH_DEVICE

void setup_shunt() {
  if (shunt) {
    return;
  }

  switch (user_selected_shunt_type) {
    case ShuntType::None:
      shunt = nullptr;
      return;
    case ShuntType::BmwSbox:
      shunt = new BmwSbox();
      shunt->setup();
      return;
    case ShuntType::Inverter:
      if (inverter && inverter->provides_shunt()) {
        inverter->enable_shunt();
      }
      return;
#ifndef SMALL_FLASH_DEVICE
    case ShuntType::CustomClamp:
      shunt = nullptr;
      return;
    case ShuntType::Qnhck2_16:
      shunt = new Qnhck2_16Shunt();
      shunt->setup();
      return;
#endif  // SMALL_FLASH_DEVICE
    default:
      return;
  }
}

bool shunt_type_supported_by_battery(ShuntType type, BatteryType battery_type) {
#ifndef SMALL_FLASH_DEVICE
  if (type == ShuntType::CustomClamp) {
    return battery_type == BatteryType::Chademo;
  }
#endif  // SMALL_FLASH_DEVICE
  return true;
}

bool shunt_type_supported_by_inverter(ShuntType type, InverterProtocolType inverter_type) {
  if (type == ShuntType::Inverter) {
    return inverter_type_provides_shunt(inverter_type);
  }
  return true;
}

extern const char* name_for_shunt_type(ShuntType type) {
  switch (type) {
    case ShuntType::None:
      return "None";
    case ShuntType::BmwSbox:
      return BmwSbox::Name;
    case ShuntType::Inverter:
      return "Using inverter values";
#ifndef SMALL_FLASH_DEVICE
    case ShuntType::CustomClamp:
      return "Custom Clamp";
    case ShuntType::Qnhck2_16:
      return board_has_shunt_adc_pin() ? Qnhck2_16Shunt::Name : nullptr;
#endif  // SMALL_FLASH_DEVICE
    default:
      return nullptr;
  }
}
