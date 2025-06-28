#ifndef CHEVYVOLT_CHARGER_H
#define CHEVYVOLT_CHARGER_H
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../include.h"
#include "CanCharger.h"

#ifdef CHEVYVOLT_CHARGER
#define SELECTED_CHARGER_CLASS ChevyVoltCharger
#endif

class ChevyVoltCharger : public CanCharger {
 public:
  ChevyVoltCharger() : CanCharger(ChargerType::ChevyVolt) {}

  const char* name() { return Name; }
  static constexpr char* Name = "Chevy Volt Gen1 Charger";

  void map_can_frame_to_variable(CAN_frame rx_frame);
  void transmit_can(unsigned long currentMillis);

  float outputPowerDC() {
    return static_cast<float>(datalayer.charger.charger_stat_HVcur * datalayer.charger.charger_stat_HVvol);
  }
  bool efficiencySupported() { return true; }
  float efficiency() {
    float chgPwrAC = static_cast<float>(datalayer.charger.charger_stat_ACcur * datalayer.charger.charger_stat_ACvol);
    return outputPowerDC() / chgPwrAC * 100;
  }

 private:
  /* Charger hardware limits
 *
 * Relative to runtime settings, expectations are:
 * hw minimum <= setting minimum <= setting maximum <= hw max
 */
  const float CHEVYVOLT_MAX_HVDC = 420.0;
  const float CHEVYVOLT_MIN_HVDC = 200.0;
  const float CHEVYVOLT_MAX_AMP = 11.5;
  const float CHEVYVOLT_MAX_POWER = 3300;

  /* CAN cycles and timers */
  unsigned long previousMillis30ms = 0;    // 30ms cycle for keepalive frames
  unsigned long previousMillis200ms = 0;   // 200ms cycle for commanding I/V targets
  unsigned long previousMillis5000ms = 0;  // 5s status printout to serial

  enum CHARGER_MODES : uint8_t { MODE_DISABLED = 0, MODE_LV, MODE_HV, MODE_HVLV };

  //Actual content messages
  CAN_frame charger_keepalive_frame = {.FD = false,
                                       .ext_ID = false,
                                       .DLC = 1,
                                       .ID = 0x30E,  //one byte only, indicating enabled or disabled
                                       .data = {MODE_DISABLED}};

  CAN_frame charger_set_targets = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 4,
                                   .ID = 0x304,
                                   .data = {0x40, 0x00, 0x00, 0x00}};  // data[0] is a value, meaning unknown
};

#endif
