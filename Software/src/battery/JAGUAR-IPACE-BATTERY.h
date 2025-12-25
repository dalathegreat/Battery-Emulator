#ifndef JAGUAR_IPACE_BATTERY_H
#define JAGUAR_IPACE_BATTERY_H

#include "CanBattery.h"

extern bool user_selected_use_estimated_SOC;

class JaguarIpaceBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Jaguar I-PACE";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4546;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3370;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  /* Do not change code below unless you are sure what you are doing */
  unsigned long previousMillisKeepAlive = 0;

  uint8_t HVBattAvgSOC = 0;
  uint8_t HVBattFastChgCounter = 0;
  uint8_t HVBattTempColdCellID = 0;
  uint8_t HVBatTempHotCellID = 0;
  uint8_t HVBattVoltMaxCellID = 0;
  uint8_t HVBattVoltMinCellID = 0;
  uint8_t HVBattPwerGPCS = 0;
  uint8_t HVBattPwrGpCounter = 0;
  int8_t HVBattCurrentTR = 0;
  uint16_t HVBattCellVoltageMaxMv = 3700;
  uint16_t HVBattCellVoltageMinMv = 3700;
  uint16_t HVBattEnergyAvailable = 0;
  uint16_t HVBattEnergyUsableMax = 0;
  uint16_t HVBattTotalCapacityWhenNew = 0;
  uint16_t HVBattDischargeContiniousPowerLimit = 0;
  uint16_t HVBattDischargePowerLimitExt = 0;
  uint16_t HVBattDischargeVoltageLimit = 0;
  uint16_t HVBattVoltageExt = 0;
  uint16_t HVBatteryVoltageOC = 0;
  uint16_t HVBatteryChgCurrentLimit = 0;
  uint16_t HVBattChargeContiniousPowerLimit = 0;
  int16_t HVBattAverageTemperature = 0;
  int16_t HVBattCellTempAverage = 0;
  int16_t HVBattCellTempColdest = 0;
  int16_t HVBattCellTempHottest = 0;
  int16_t HVBattInletCoolantTemp = 0;
  bool HVBatteryContactorStatus = false;
  bool HVBatteryContactorStatusT = false;
  bool HVBattHVILError = false;
  bool HVILBattIsolationError = false;
  bool HVIsolationTestStatus = false;

  /* KeepAlive: PMZ_CAN_GWM_OSEK_NM_Pdu every 200ms.
 */
  CAN_frame ipace_keep_alive = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x51e,
                                .data = {0x22, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  /* KeepAlive: PMZ_CAN_NodeGWM_NM every 1s.
 * TODO: This may be needed for >2021 models.
 */
  /*CAN_frame ipace_keep_alive = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x59e,
                              .data = {0x9E, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};*/
};

#endif
