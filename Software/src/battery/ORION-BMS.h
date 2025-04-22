#ifndef ORION_BMS_H
#define ORION_BMS_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

class OrionBms : public CanBattery {
 public:
  OrionBms() : CanBattery(OrionBMS) {}
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "DIY battery with Orion BMS (Victron setting)";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);

  // No transmit needed for this type
  virtual void transmit_can() {}

  virtual uint16_t max_pack_voltage_dv() { return 5000; }
  virtual uint16_t min_pack_voltage_dv() { return 1500; }
  virtual uint16_t max_cell_deviation_mv() { return 150; }
  virtual uint16_t max_cell_voltage_mv() { return 4250; }
  virtual uint16_t min_cell_voltage_mv() { return 2700; }
  virtual uint8_t number_of_cells() { return 96; }

 private:
  uint16_t cellvoltages[MAX_AMOUNT_CELLS];  //array with all the cellvoltages
  uint16_t Maximum_Cell_Voltage = 3700;
  uint16_t Minimum_Cell_Voltage = 3700;
  uint16_t Pack_Health = 99;
  int16_t Pack_Current = 0;
  int16_t Average_Temperature = 0;
  uint16_t Pack_Summed_Voltage = 0;
  int16_t Average_Current = 0;
  uint16_t High_Temperature = 0;
  uint16_t Pack_SOC_ppt = 0;
  uint16_t Pack_CCL = 0;  //Charge current limit (A)
  uint16_t Pack_DCL = 0;  //Discharge current limit (A)
  uint16_t Maximum_Pack_Voltage = 0;
  uint16_t Minimum_Pack_Voltage = 0;
  uint16_t CellID = 0;
  uint16_t CellVoltage = 0;
  uint16_t CellResistance = 0;
  uint16_t CellOpenVoltage = 0;
  uint16_t Checksum = 0;
  uint16_t CellBalancing = 0;
  uint8_t amount_of_detected_cells = 0;
};

#endif
