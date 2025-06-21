#ifndef NISSANLEAF_CHARGER_H
#define NISSANLEAF_CHARGER_H
#include <Arduino.h>
#include "../include.h"

#include "CanCharger.h"

#ifdef NISSANLEAF_CHARGER
#define SELECTED_CHARGER_CLASS NissanLeafCharger
#endif

class NissanLeafCharger : public CanCharger {
 public:
  NissanLeafCharger() : CanCharger(ChargerType::NissanLeaf) {}

  const char* name() { return Name; }
  static constexpr char* Name = "Nissan LEAF 2013-2024 PDM charger";

  void map_can_frame_to_variable(CAN_frame rx_frame);
  void transmit_can(unsigned long currentMillis);

  float outputPowerDC() { return static_cast<float>(datalayer.charger.charger_stat_HVcur * 100); }

  float HVDC_output_current() {
    // P/U=I
    if (datalayer.battery.status.voltage_dV > 0) {
      return outputPowerDC() / (datalayer.battery.status.voltage_dV / 10);
    }
    return 0;
  }

  float HVDC_output_voltage() { return static_cast<float>(datalayer.battery.status.voltage_dV / 10); }

 private:
  /* CAN cycles and timers */
  unsigned long previousMillis10ms = 0;
  unsigned long previousMillis100ms = 0;

  /* LEAF charger/battery parameters */
  enum OBC_MODES : uint8_t {
    IDLE_OR_QC = 1,
    FINISHED = 2,
    CHARGING_OR_INTERRUPTED = 4,
    IDLE1 = 8,
    IDLE2 = 9,
    PLUGGED_IN_WAITING_ON_TIMER
  };
  enum OBC_VOLTAGES : uint8_t { NO_SIGNAL = 0, AC110 = 1, AC230 = 2, ABNORMAL_WAVE = 3 };
  uint16_t OBC_Charge_Power = 0;  // Actual charger output
  uint8_t mprun100 = 0;           // Counter 0-3
  uint8_t mprun10 = 0;            // Counter 0-3
  uint8_t OBC_Charge_Status = IDLE_OR_QC;
  uint8_t OBC_Status_AC_Voltage = 0;  //1=110V, 2=230V
  uint8_t OBCpowerSetpoint = 0;
  uint8_t OBCpower = 0;
  bool PPStatus = false;
  bool OBCwakeup = false;

  //Actual content messages
  CAN_frame LEAF_1DB = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1DB,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_1DC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1DC,
                        .data = {0x6E, 0x0A, 0x05, 0xD5, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_1F2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x1F2,
                        .data = {0x30, 0x00, 0x20, 0xAC, 0x00, 0x3C, 0x00, 0x8F}};
  CAN_frame LEAF_50B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 7,
                        .ID = 0x50B,
                        .data = {0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x00}};
  CAN_frame LEAF_55B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x55B,
                        .data = {0xA4, 0x40, 0xAA, 0x00, 0xDF, 0xC0, 0x10, 0x00}};
  CAN_frame LEAF_5BC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x5BC,
                        .data = {0x3D, 0x80, 0xF0, 0x64, 0xB0, 0x01, 0x00, 0x32}};

  CAN_frame LEAF_59E = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x59E,
                        .data = {0x00, 0x00, 0x0C, 0x76, 0x18, 0x00, 0x00, 0x00}};
};

#endif
