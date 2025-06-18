#include "../include.h"

CanCharger* charger = nullptr;

ChargerType user_selected_charger_type = ChargerType::None;

std::vector<ChargerType> supported_charger_types() {
  std::vector<ChargerType> types;

  for (int i = 0; i < (int)ChargerType::Highest; i++) {
    types.push_back((ChargerType)i);
  }

  return types;
}

extern const char* name_for_charger_type(ChargerType type) {
  switch (type) {
    case ChargerType::ChevyVolt:
      return ChevyVoltCharger::Name;
    case ChargerType::NissanLeaf:
      return NissanLeafCharger::Name;
    case ChargerType::None:
      return "None";
  }

  return nullptr;
}

void setup_charger() {
#ifdef COMMON_IMAGE
  switch (user_selected_charger_type) {
    case ChargerType::ChevyVolt:
      charger = new ChevyVoltCharger();
    case ChargerType::NissanLeaf:
      charger = new NissanLeafCharger();
  }
#else
#ifdef SELECTED_CHARGER_CLASS
  charger = new SELECTED_CHARGER_CLASS();
#endif
#endif
}
