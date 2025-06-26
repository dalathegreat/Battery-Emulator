#ifndef SONO_BATTERY_H
#define SONO_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef SONO_BATTERY
#define SELECTED_BATTERY_CLASS SonoBattery
#endif

class SonoBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Sono Motors Sion 64kWh LFP ";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2500;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 3800;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was send

  uint8_t seconds = 0;
  uint8_t functionalsafetybitmask = 0;
  uint16_t batteryVoltage = 3700;
  uint16_t allowedDischargePower = 0;
  uint16_t allowedChargePower = 0;
  uint16_t CellVoltMax_mV = 0;
  uint16_t CellVoltMin_mV = 0;
  int16_t batteryAmps = 0;
  int16_t temperatureMin = 0;
  int16_t temperatureMax = 0;
  uint8_t batterySOH = 99;
  uint8_t realSOC = 99;

  CAN_frame SONO_400 = {.FD = false,  //Message of Vehicle Command, 100ms
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x400,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SONO_401 = {.FD = false,  //Message of Vehicle Date, 1000ms
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x400,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
