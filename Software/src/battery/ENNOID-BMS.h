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

  bool supports_charged_energy() { return true; }

  static constexpr const char* Name = "ENNOID BMS via VESC, DIY battery";

 private:
  static const int MAX_CHARGE_POWER_WHEN_TOPBALANCING_W = 500;
  static const int RAMPDOWN_SOC =
      9000;  // (90.00) SOC% to start ramping down from max charge power towards 0 at 100.00%

  float packVoltage = 0;  // V
  float packCurrent = 0;  // A
  float dischargeAh = 0;  // Ah (current discharge counter)
  float dischargeWh = 0;  // Wh
  float totalChargeAh = 0;
  float totalChargeWh = 0;
  float totalDischargeAh = 0;
  float totalDischargeWh = 0;

  uint8_t numberOfCells = 0;
  uint8_t numberOfTempSensors = 0;
  uint16_t cellVoltages_mV[192] = {0};
  int16_t temperatures_cC[50] = {0};  // centi-degC; signed so sub-zero works
  uint16_t SOC = 0;
  uint16_t SOH = 10000;
  uint16_t cellVoltageLow_mV = 0;
  uint16_t cellVoltageMax_mV = 0;
  int16_t tBms_cC = 0;
  int16_t unknown2C0 = 0;  // -5000, static
};

#endif
