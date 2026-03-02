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
  static const int MAX_PACK_VOLTAGE_DV = 3940;
  static const int MIN_PACK_VOLTAGE_DV = 2880;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4100;
  static const int MIN_CELL_VOLTAGE_MV = 3000;

  unsigned long previousMillis200 = 0;  // will store last time a 100ms CAN Message was sent

  CAN_frame PCU_310 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 3,
                       .ID = 0x310,
                       .data = {0x00, 0x00, 0x06}};  //Charger Status & TIM Status = TRUE
  CAN_frame PCU_311 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x311, .data = {0x00, 0x00}};

  uint16_t min_cellvoltage = 3700;
  uint16_t max_cellvoltage = 3700;
  uint16_t sys_voltage = 3600;
  uint16_t sys_dod = 0;
  uint16_t sys_voltageMinDischarge = MIN_PACK_VOLTAGE_DV;
  uint16_t sys_currentMaxDischarge = 0;
  uint16_t sys_currentMaxCharge = 0;
  uint16_t sys_voltageMaxCharge = MAX_PACK_VOLTAGE_DV;
  uint16_t BatterySOC = 500;
  int16_t sys_tempMean = 0;
  int16_t sys_current = 0;
  uint8_t Battery_Type = 0;
  uint8_t sys_ZebraTempError = 0;
  uint8_t sys_numberFailedCells = 0;
  int8_t min_pack_temperature = 0;
  int8_t max_pack_temperature = 0;
  bool sys_errGeneral = false;
  bool sys_isolationError = false;
};

#endif
