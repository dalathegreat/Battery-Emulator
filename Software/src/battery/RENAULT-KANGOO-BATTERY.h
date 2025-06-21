#ifndef RENAULT_KANGOO_BATTERY_H
#define RENAULT_KANGOO_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef RENAULT_KANGOO_BATTERY
#define SELECTED_BATTERY_CLASS RenaultKangooBattery
#endif

class RenaultKangooBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Renault Kangoo";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4150;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2500;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CHARGE_POWER_W = 5000;   // Battery can be charged with this amount of power

  uint32_t LB_Battery_Voltage = 3700;
  uint32_t LB_Charge_Power_Limit_Watts = 0;
  int32_t LB_Current = 0;
  int16_t LB_MAX_TEMPERATURE = 0;
  int16_t LB_MIN_TEMPERATURE = 0;
  uint16_t LB_SOC = 0;
  uint16_t LB_SOH = 0;
  uint16_t LB_Discharge_Power_Limit = 0;
  uint16_t LB_Charge_Power_Limit = 0;
  uint16_t LB_kWh_Remaining = 0;
  uint16_t LB_Cell_Max_Voltage = 3700;
  uint16_t LB_Cell_Min_Voltage = 3700;
  uint16_t LB_MaxChargeAllowed_W = 0;
  uint8_t LB_Discharge_Power_Limit_Byte1 = 0;
  uint8_t GVI_Pollcounter = 0;
  uint8_t LB_EOCR = 0;
  uint8_t LB_HVBUV = 0;
  uint8_t LB_HVBIR = 0;
  uint8_t LB_CUV = 0;
  uint8_t LB_COV = 0;
  uint8_t LB_HVBOV = 0;
  uint8_t LB_HVBOT = 0;
  uint8_t LB_HVBOC = 0;
  uint8_t LB_MaxInput_kW = 0;
  uint8_t LB_MaxOutput_kW = 0;
  bool GVB_79B_Continue = false;

  CAN_frame KANGOO_423 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x423,
                          .data = {0x0B, 0x1D, 0x00, 0x02, 0xB2, 0x20, 0xB2, 0xD9}};  // Charging
  // Driving: 0x07  0x1D  0x00  0x02  0x5D  0x80  0x5D  0xD8
  // Charging: 0x0B   0x1D  0x00  0x02  0xB2  0x20  0xB2  0xD9
  // Fastcharging: 0x07   0x1E  0x00  0x01  0x5D  0x20  0xB2  0xC7
  // Old hardcoded message: .data = {0x33, 0x00, 0xFF, 0xFF, 0x00, 0xE0, 0x00, 0x00}};
  CAN_frame KANGOO_79B = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x79B,
                          .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0xE0, 0x00, 0x00}};
  CAN_frame KANGOO_79B_Continue = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 8,
                                   .ID = 0x79B,
                                   .data = {0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  unsigned long GVL_pause = 0;
};

#endif
