#include "../devboard/hal/hal.h"
#include "../inverter/INVERTERS.h"
#include "BMW-SBOX.h"
#include "QNHCK2-16.h"
#include "Shunt.h"

Shunt* shunt = nullptr;
ShuntType user_selected_shunt_type = ShuntType::None;

// Shunts read through an ADC pin are only offered on boards whose HAL routes one
static bool board_has_shunt_adc_pin() {
  return esp32hal && esp32hal->SHUNT_ADC_PIN() != GPIO_NUM_NC;
}

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
    case ShuntType::CustomClamp:
      shunt = nullptr;
      return;
    case ShuntType::Qnhck2_16:
      shunt = new Qnhck2_16Shunt();
      shunt->setup();
      return;
    default:
      return;
  }
}

extern std::vector<ShuntType> supported_shunt_types() {
  std::vector<ShuntType> types;
  types.push_back(ShuntType::None);
  types.push_back(ShuntType::BmwSbox);

  if (inverter && inverter->provides_shunt())
    types.push_back(ShuntType::Inverter);

  types.push_back(ShuntType::CustomClamp);

  if (board_has_shunt_adc_pin())
    types.push_back(ShuntType::Qnhck2_16);

  return types;
}

extern const char* name_for_shunt_type(ShuntType type) {
  switch (type) {
    case ShuntType::None:
      return "None";
    case ShuntType::BmwSbox:
      return BmwSbox::Name;
    case ShuntType::Inverter:
      return "Using inverter values";
    case ShuntType::CustomClamp:
      return "Custom Clamp";
    case ShuntType::Qnhck2_16:
      return board_has_shunt_adc_pin() ? Qnhck2_16Shunt::Name : nullptr;
    default:
      return nullptr;
  }
}
