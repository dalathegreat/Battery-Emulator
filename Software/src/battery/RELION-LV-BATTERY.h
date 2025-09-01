#ifndef RELION_BATTERY_H
#define RELION_BATTERY_H

#include "../system_settings.h"
#include "CanBattery.h"

class RelionBattery : public CanBattery {
 public:
  RelionBattery() : CanBattery(CAN_Speed::CAN_SPEED_250KBPS) {}

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Relion LV protocol via 250kbps CAN";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 584;  //58.4V recommended charge voltage. BMS protection steps in at 60.8V
  static const int MIN_PACK_VOLTAGE_DV = 440;  //44.0V Recommended LV disconnect. BMS protection steps in at 40.0V
  static const int MAX_CELL_DEVIATION_MV = 300;
  static const int MAX_CELL_VOLTAGE_MV = 3800;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  uint16_t battery_total_voltage = 500;
  int16_t battery_total_current = 0;
  uint8_t system_state = 0;
  uint8_t battery_soc = 50;
  uint8_t battery_soh = 99;
  uint8_t most_serious_fault = 0;
  uint16_t max_cell_voltage = 3300;
  uint16_t min_cell_voltage = 3300;
  int16_t max_cell_temperature = 0;
  int16_t min_cell_temperature = 0;
  int16_t charge_current_A = 0;
  int16_t regen_charge_current_A = 0;
  int16_t discharge_current_A = 0;
};

#endif
