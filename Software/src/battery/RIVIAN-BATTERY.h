#ifndef RIVIAN_BATTERY_H
#define RIVIAN_BATTERY_H
#include "CanBattery.h"

class RivianBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Rivian R1T large 135kWh battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4480;
  static const int MIN_PACK_VOLTAGE_DV = 2920;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4200;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 3300;  //Battery is put into emergency stop if one cell goes below this value

  uint8_t BMS_state = 0;
  uint16_t battery_voltage = 3700;
  uint16_t battery_SOC = 5000;
  int32_t battery_current = 32000;
  uint16_t kWh_available_total = 135;
  uint16_t kWh_available_max = 135;
  int16_t battery_min_temperature = 0;
  int16_t battery_max_temperature = 0;
  uint16_t battery_discharge_limit_amp = 0;
  uint16_t battery_charge_limit_amp = 0;
  static const uint8_t SLEEP = 0;
  static const uint8_t STANDBY = 1;
  static const uint8_t READY = 2;
  static const uint8_t GO = 3;
  unsigned long previousMillis10 = 0;  // will store last time a 10ms CAN Message was sent

  CAN_frame RIVIAN_150 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 6,
                          .ID = 0x150,
                          .data = {0x03, 0x00, 0x01, 0x00, 0x01, 0x00}};
  CAN_frame RIVIAN_420 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x420,
                          .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame RIVIAN_41F = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x41F, .data = {0x62, 0x10, 0x00}};
  CAN_frame RIVIAN_245 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 6,
                          .ID = 0x245,
                          .data = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame RIVIAN_200 = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x200, .data = {0x08}};
  CAN_frame RIVIAN_207 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 1,
                          .ID = 0x207,
                          .data = {0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00}};
};

#endif
