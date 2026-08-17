#ifndef BYD_MODBUS_H
#define BYD_MODBUS_H

#include "../devboard/utils/types.h"
#include "ModbusInverterProtocol.h"

class BydModbusInverter : public ModbusInverterProtocol {
 public:
  BydModbusInverter() : ModbusInverterProtocol(21) {}
  const char* name() override { return Name; }
  bool setup() override;
  void update_values();
  static constexpr const char* Name = "BYD 11kWh HVM battery over Modbus RTU";

 private:
  void handle_static_data();
  void verify_temperature();
  void verify_inverter_modbus();
  void handle_update_data_modbusp201_byd();
  void handle_update_data_modbusp301_byd();
  int16_t byd_power_W();

  // Register 303 status bits sent back to the inverter: bit 7 = normal operation,
  // bit 0 = charging, bit 1 = discharging
  static const uint16_t BYD_MODE_IDLE = 128;
  static const uint16_t BYD_MODE_CHARGING = 129;
  static const uint16_t BYD_MODE_DISCHARGING = 130;

  static const uint8_t HISTORY_LENGTH =
      5;  // Amount of samples(minutes) that needs to match for register to be considered stale
  unsigned long previousMillis60s = 0;  // will store last time a 60s event occured
  uint32_t user_configured_max_discharge_W = 0;
  uint32_t user_configured_max_charge_W = 0;
  uint32_t max_discharge_W = 0;
  uint32_t max_charge_W = 0;
  uint16_t register_401_history[5] = {0};
  uint8_t history_index = 0;
  uint16_t bms_char_dis_status = BYD_MODE_IDLE;
  bool all_401_values_equal = false;
};

#endif
