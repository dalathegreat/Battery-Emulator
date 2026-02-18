#ifndef ENNOID_BMS_H
#define ENNOID_BMS_H

#include "../system_settings.h"
#include "CanBattery.h"

class EnnoidBms : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "ENNOID BMS via VESC, DIY battery";

 private:
  static const int MAX_CHARGE_POWER_WHEN_TOPBALANCING_W = 500;
  static const int RAMPDOWN_SOC =
      9000;  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%

  uint32_t packVoltage = 3300;
  int32_t packCurrent1 = 0;
  int32_t packCurrent2 = 0;
  uint16_t cellVoltageLow = 3300;
  uint16_t cellVoltageHigh = 3300;
  uint16_t SOC = 5000;
  uint16_t SOH = 9900;
  uint8_t BitF = 0;
  uint8_t NoOfCells = 0;
  int8_t tBattHi = 0;
};

#endif
