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
  // Consumes the ControlData block the inverter writes: WatchDogTimeout (402), UTC (403-406)
  // and RebootCommand (407)
  void handle_inverter_control_data();
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
  uint16_t last_register_407 = 0;  // Last RebootCommand seen, so only changes are acted on
  uint16_t last_register_408 = 0;  // Last DarkstartEnable seen, so only changes are logged
  // Bounds for a WatchDogTimeout accepted from register 402. 0 means the inverter is not using the
  // field, and an implausibly large value would push inverter-missing detection out of usefulness.
  static const uint32_t WATCHDOG_TIMEOUT_MIN_S = 5;
  static const uint32_t WATCHDOG_TIMEOUT_MAX_S = 3600;
  uint16_t bms_char_dis_status = BYD_MODE_IDLE;
  bool all_401_values_equal = false;
};

#endif
