#ifndef THINK_CITY_H
#define THINK_CITY_H

#include "../system_settings.h"
#include "CanBattery.h"

class ThinkBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Think City";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 3920;  //Later read via CAN (sys_voltageMinCharge)
  static const int MIN_PACK_VOLTAGE_DV = 2400;  //Later read via CAN (sys_voltageMinDischarge)
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4200;
  static const int MIN_CELL_VOLTAGE_MV = 3300;

  uint16_t sys_voltage = 0;
  uint16_t sys_dod = 0;
  uint16_t sys_voltageMinDischarge = 0;
  uint16_t sys_currentMaxDischarge = 0;
  uint16_t sys_currentMaxCharge = 0;
  uint16_t sys_voltageMaxCharge = 0;
  uint16_t BatterySOC = 500;
  int16_t sys_tempMean = 0;
  int16_t sys_current = 0;
  uint8_t Battery_Type = 0;
  uint8_t sys_ZebraTempError = 0;
  uint8_t sys_numberFailedCells = 0;
  bool sys_errGeneral = false;
  bool sys_isolationError = false;
};

#endif
