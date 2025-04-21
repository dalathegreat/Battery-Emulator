#ifndef CHARGERS_H
#define CHARGERS_H
#include "../../USER_SETTINGS.h"

#include "../devboard/utils/types.h"

enum class ChargerType { None = 0, ChevyVolt = 1, NissanLeaf = 2, HighestCharger = 3 };

class Charger {
 public:
  virtual void map_can_frame_to_variable_charger(CAN_frame rx_frame) = 0;
  virtual void transmit_can() = 0;
  ChargerType type() { return m_type; }
  virtual const char* name() = 0;

  static const char* name_for_type(ChargerType type);

 protected:
  Charger(ChargerType type) : m_type(type) {}
  ChargerType m_type;
};

extern Charger* charger;

Charger* init_charger(ChargerType type);

std::vector<ChargerType> supported_charger_types();
extern ChargerType userSelectedChargerType;

#ifdef BUILD_EM_ALL
#define CHEVYVOLT_CHARGER
#define NISSANLEAF_CHARGER
#endif

#ifdef CHEVYVOLT_CHARGER
#include "CHEVY-VOLT-CHARGER.h"
#endif

#ifdef NISSANLEAF_CHARGER
#include "NISSAN-LEAF-CHARGER.h"
#endif

#endif
