#ifndef ORION_BMS_H
#define ORION_BMS_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef ORION_BMS
#define SELECTED_BATTERY_CLASS OrionBms
#endif

class OrionBms : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "DIY battery with Orion BMS (Victron setting)";

 private:
  /* Change the following to suit your battery */
  static const int NUMBER_OF_CELLS = 96;
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_DEVIATION_MV = 150;

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
