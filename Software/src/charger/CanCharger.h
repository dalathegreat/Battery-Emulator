#ifndef CAN_CHARGER_H
#define CAN_CHARGER_H

#include "src/devboard/utils/types.h"

#include "../datalayer/datalayer.h"

enum class ChargerType { NissanLeaf, ChevyVolt };

// Generic base class for all chargers
class Charger {
 public:
  ChargerType type() { return m_type; }

  virtual const char* name() = 0;

  virtual float outputPowerDC() = 0;

  virtual float HVDC_output_voltage() { return datalayer.charger.charger_stat_HVvol; }
  virtual float HVDC_output_current() { return datalayer.charger.charger_stat_HVcur; }

  virtual float LVDC_output_voltage() { return datalayer.charger.charger_stat_LVvol; }
  virtual float LVDC_output_current() { return datalayer.charger.charger_stat_LVcur; }

  virtual float AC_input_voltage() { return datalayer.charger.charger_stat_ACvol; }
  virtual float AC_input_current() { return datalayer.charger.charger_stat_ACcur; }

  virtual bool efficiencySupported() { return false; }
  virtual float efficiency() { return 0; }

 protected:
  Charger(ChargerType type) : m_type(type) {}

 private:
  ChargerType m_type;
};

// Base class for chargers on a CAN bus
class CanCharger : public Charger {
 public:
  virtual void map_can_frame_to_variable(CAN_frame rx_frame) = 0;
  virtual void transmit_can(unsigned long currentMillis) = 0;

 protected:
  CanCharger(ChargerType type) : Charger(type) {}
};

#endif
