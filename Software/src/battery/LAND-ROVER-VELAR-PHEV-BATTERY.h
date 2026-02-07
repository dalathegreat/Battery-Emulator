#ifndef LAND_ROVER_VELAR_PHEV_BATTERY_H
#define LAND_ROVER_VELAR_PHEV_BATTERY_H

#include "CanBattery.h"

class LandRoverVelarPhevBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Range Rover Velar 17kWh PHEV battery (L560)";

 private:
  /* Change the following to suit your battery */
  static const int MAX_PACK_VOLTAGE_DV = 4710;
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_DEVIATION_MV = 150;

  unsigned long previousMillis50ms = 0;  // will store last time a 50ms CAN Message was sent

  uint16_t HVBattStateofHealth = 1000;
  uint16_t HVBattSOCAverage = 5000;
  uint16_t HVBattVoltageExt = 370;
  uint16_t HVBattCellVoltageMin = 3700;
  uint16_t HVBattCellVoltageMax = 3700;
  int16_t HVBattCellTempHottest = 0;
  int16_t HVBattCellTempColdest = 0;
  uint8_t HVBattStatusCritical = 1;  //1=OK, 2 = FAULT
  uint8_t voltage_group = 0;
  uint8_t module_id = 0;
  uint8_t base_index = 0;
  bool HVBattHVILStatus = false;     //0=OK, 1=Not OK
  bool HVBattAuxiliaryFuse = false;  //0=OK, 1=Not OK
  bool HVBattTractionFuseF = false;  //0=OK, 1=Not OK
  bool HVBattTractionFuseR = false;  //0=OK, 1=Not OK

  //CAN messages needed by battery (LOG needed!)
  CAN_frame VELAR_18B = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x18B,  //CONTENT??? TODO
                         .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
