#ifndef RENAULT_TWIZY_BATTERY_H
#define RENAULT_TWIZY_BATTERY_H
#include "../include.h"
#include "CanBattery.h"

#ifdef RENAULT_TWIZY_BATTERY
#define SELECTED_BATTERY_CLASS RenaultTwizyBattery
#endif

class RenaultTwizyBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Renault Twizy";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 579;  // 57.9V at 100% SOC (with 70% SOH, new one might be higher)
  static const int MIN_PACK_VOLTAGE_DV = 480;  // 48.4V at 13.76% SOC
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4200;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 3400;  //Battery is put into emergency stop if one cell goes below this value

  int16_t cell_temperatures_dC[7] = {0};
  int16_t current_dA = 0;
  uint16_t voltage_dV = 0;
  int16_t cellvoltages_mV[14] = {0};
  int16_t max_discharge_power = 0;
  int16_t max_recup_power = 0;
  int16_t max_charge_power = 0;
  uint16_t SOC = 0;
  uint16_t SOH = 0;
  uint16_t remaining_capacity_Wh = 0;
};

#endif
