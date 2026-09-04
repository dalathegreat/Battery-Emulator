#ifndef BYD_BATTERY_BOX_BATTERY_H
#define BYD_BATTERY_BOX_BATTERY_H

#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class BYDBatteryBoxBattery : public CanBattery {
 public:
  void setup(void) override;
  void handle_incoming_can_frame(CAN_frame rx_frame) override;
  void update_values() override;
  void transmit_can(unsigned long currentMillis) override;

  static constexpr const char* Name = "BYD Battery-Box Premium";

 private:
  static const uint16_t MAX_CELL_DEVIATION_MV = 200;
  static const uint16_t MAX_CELL_VOLTAGE_MV = 3650;  //Charging stops if one cell exceeds this value
  static const uint16_t MIN_CELL_VOLTAGE_MV = 2800;  //Discharging stops if one cell goes below this value
  CAN_frame BYD_151 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x151,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_091 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x091,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_0D1 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x0D1,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_111 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x111,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BYD_191 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x191,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  unsigned long previousMillis1000 = 0;

  uint16_t voltage_dV = 3700;
  uint16_t SOC = 500;
  uint16_t SOH = 0;
  uint16_t remaining_capacity_dAh = 0;
  uint16_t fullcharge_capacity_dAh = 0;
  uint16_t maximum_discharge_power_allowed_dA = 0;
  uint16_t maximum_charge_power_allowed_dA = 0;
  uint16_t target_charge_voltage_dV = 0;
  uint16_t target_discharge_voltage_dV = 0;
  int16_t current_dA = 0;
  int16_t temperature_average = 0;
  int16_t temperature_min_dC = 0;
  int16_t temperature_max_dC = 0;
  bool we_have_identified_battery = false;
};

#endif
