#ifndef PYLON_BATTERY_H
#define PYLON_BATTERY_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../include.h"

#define BATTERY_SELECTED

/* Change the following to suit your battery */

class PylonBattery : public CanBattery {
 public:
  PylonBattery(DATALAYER_BATTERY_TYPE* target, CAN_Interface can_interface) : CanBattery(Pylon) {
    m_target = target;
    m_can_interface = can_interface;
  }
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "Pylon compatible battery";
  virtual bool supportsDoubleBattery() { return true; };

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { return 5000; }
  virtual uint16_t min_pack_voltage_dv() { return 1500; }
  virtual uint16_t max_cell_deviation_mv() { return 150; }
  virtual uint16_t max_cell_voltage_mv() { return 4250; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }

 private:
  DATALAYER_BATTERY_TYPE* m_target;
  CAN_Interface m_can_interface;

  int16_t celltemperature_max_dC = 0;
  int16_t celltemperature_min_dC = 0;
  int16_t current_dA = 0;
  uint16_t voltage_dV = 0;
  uint16_t cellvoltage_max_mV = 3700;
  uint16_t cellvoltage_min_mV = 3700;
  uint16_t charge_cutoff_voltage = 0;
  uint16_t discharge_cutoff_voltage = 0;
  int16_t max_charge_current = 0;
  int16_t max_discharge_current = 0;
  uint8_t ensemble_info_ack = 0;
  uint8_t battery_module_quantity = 0;
  uint8_t battery_modules_in_series = 0;
  uint8_t cell_quantity_in_module = 0;
  uint8_t voltage_level = 0;
  uint8_t ah_number = 0;
  uint8_t SOC = 0;
  uint8_t SOH = 0;
  uint8_t charge_forbidden = 0;
  uint8_t discharge_forbidden = 0;
};

#endif
