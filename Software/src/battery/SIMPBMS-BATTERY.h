#ifndef SIMPBMS_BATTERY_H
#define SIMPBMS_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef SIMPBMS_BATTERY
#define SELECTED_BATTERY_CLASS SimpBmsBattery
#endif

class SimpBmsBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "SIMPBMS battery";

 private:
  /* DEFAULT VALUES BMS will send configured */
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int MAX_CELL_DEVIATION_MV = 500;
  static const int CELL_COUNT = 96;

  static const int SIMPBMS_MAX_CELLS = 128;

  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

  //Actual content messages

  int16_t celltemperature_max_dC = 0;
  int16_t celltemperature_min_dC = 0;
  int16_t current_dA = 0;
  uint16_t voltage_dV = 0;
  uint16_t cellvoltage_max_mV = 3700;
  uint16_t cellvoltage_min_mV = 3700;
  uint16_t charge_cutoff_voltage = 0;
  uint16_t discharge_cutoff_voltage = 0;
  int16_t max_charge_current = 0;
  int16_t max_discharge_current = 0;
  uint8_t ensemble_info_ack = 0;
  uint8_t cells_in_series = 0;
  uint8_t voltage_level = 0;
  uint8_t ah_total = 0;
  uint8_t SOC = 0;
  uint8_t SOH = 99;
  uint8_t charge_forbidden = 0;
  uint8_t discharge_forbidden = 0;
  uint16_t cellvoltages_mV[SIMPBMS_MAX_CELLS] = {0};
};

#endif
