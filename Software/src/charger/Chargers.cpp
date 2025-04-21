#include "CHARGERS.h"

ChargerType userSelectedChargerType = ChargerType::None;

Charger* init_charger(ChargerType type) {
  switch (type) {
#ifdef CHEVYVOLT_CHARGER
    case ChargerType::ChevyVolt:
      return new ChevyVoltCharger();
#endif
#ifdef NISSANLEAF_CHARGER
    case ChargerType::NissanLeaf:
      return new NissanLeafCharger();
#endif
    default:
      return nullptr;
  }
}

const char* Charger::name_for_type(ChargerType type) {
  switch (type) {
    case ChargerType::None:
      return "None";
#ifdef CHEVYVOLT_CHARGER
    case ChargerType::ChevyVolt:
      return ChevyVoltCharger::Name;
#endif
#ifdef NISSANLEAF_CHARGER
    case ChargerType::NissanLeaf:
      return NissanLeafCharger::Name;
#endif
    default:
      return nullptr;
  }
}

std::vector<ChargerType> supported_charger_types() {
  std::vector<ChargerType> types;

  // Also allow None = 0 to be selected
  for (int i = 0; i < static_cast<int>(ChargerType::HighestCharger); i++) {
    auto type = static_cast<ChargerType>(i);
    if (Charger::name_for_type(type) != nullptr) {
      types.push_back(type);
    }
  }

  return types;
}
