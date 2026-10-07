#ifndef SMALL_FLASH_DEVICE
#ifndef UUGP_CHARGER_H
#define UUGP_CHARGER_H

#include <Arduino.h>
#include <HardwareSerial.h>

#include "../communication/Transmitter.h"
#include "../communication/rs485/comm_rs485.h"
#include "CanCharger.h"

class UUGPCharger : public Charger, public Transmitter, public Rs485Receiver {
 public:
  static constexpr const char* Name = "UUGP";

  UUGPCharger();

  const char* name() override;

  float outputPowerDC() override;
  float HVDC_output_voltage() override;
  float HVDC_output_current() override;

  void transmit(unsigned long currentMillis) override;
  void receive() override;

 private:
  static constexpr uint8_t UNIT_ID = 0x01;

  static constexpr uint8_t FC_READ_HOLDING = 0x03;
  static constexpr uint8_t FC_READ_INPUT = 0x04;
  static constexpr uint8_t FC_WRITE_SINGLE = 0x06;
  static constexpr uint8_t FC_WRITE_MULTIPLE = 0x10;

  static constexpr uint16_t REG_TIMEZONE = 0x4001;
  static constexpr uint16_t REG_YEAR = 0x4002;
  static constexpr uint16_t REG_MONTH = 0x4003;
  static constexpr uint16_t REG_DAY = 0x4004;
  static constexpr uint16_t REG_HOUR = 0x4005;
  static constexpr uint16_t REG_MINUTE = 0x4006;
  static constexpr uint16_t REG_SECOND = 0x4007;

  static constexpr uint16_t REG_POWER_LIMIT = 0x4011;
  static constexpr uint16_t REG_DISCHARGE_CUTOFF_SOC = 0x4012;
  static constexpr uint16_t REG_CONTROL_MODE = 0x4013;

  static constexpr uint16_t REG_VBUS_UPPER = 0x4050;
  static constexpr uint16_t REG_VBUS_LOWER = 0x4051;
  static constexpr uint16_t REG_PCS_STATUS = 0x4052;

// Input registers
  static constexpr uint16_t REG_EV_VOLTAGE = 0x3020;
  static constexpr uint16_t REG_EV_CURRENT = 0x3021;
  static constexpr uint16_t REG_POWER_FACTOR = 0x3023;
  static constexpr uint16_t REG_MODULE_STATUS = 0x3024;
  static constexpr uint16_t REG_MODULE_TEMPERATURE = 0x3025;
  static constexpr uint16_t REG_MAX_OUTPUT_VOLTAGE = 0x3026;
  static constexpr uint16_t REG_MAX_OUTPUT_CURRENT = 0x3027;
  static constexpr uint16_t REG_RATED_POWER = 0x3028;

  static constexpr uint16_t REG_DC_VOLTAGE = 0x302F;

  static constexpr uint16_t REG_CHARGE_MODE = 0x3040;
  static constexpr uint16_t REG_CHARGE_VOLTAGE = 0x3041;
  static constexpr uint16_t REG_CHARGE_CURRENT = 0x3042;
  static constexpr uint16_t REG_ACTIVE_POWER = 0x3043;
  static constexpr uint16_t REG_VEHICLE_SOC = 0x3044;

  static constexpr uint16_t REG_CHARGE_PROTOCOL = 0x3050;
  static constexpr uint16_t REG_REMAINING_TIME = 0x3051;
  static constexpr uint16_t REG_REQUIRED_VOLTAGE = 0x3052;
  static constexpr uint16_t REG_CURRENT_DEMAND = 0x3053;
  static constexpr uint16_t REG_MAX_ALLOWABLE_VOLTAGE = 0x3054;
  static constexpr uint16_t REG_MAX_ALLOWABLE_CURRENT = 0x3055;
  static constexpr uint16_t REG_MAX_CELL_TEMPERATURE = 0x3056;
  static constexpr uint16_t REG_MIN_CELL_TEMPERATURE = 0x3057;
  static constexpr uint16_t REG_MAX_CELL_VOLTAGE = 0x3058;
  static constexpr uint16_t REG_MIN_CELL_VOLTAGE = 0x3059;
  static constexpr uint16_t REG_DC_PLUS_TEMPERATURE = 0x305A;
  static constexpr uint16_t REG_DC_MINUS_TEMPERATURE = 0x305B;

  static constexpr uint16_t REG_PM_STATUS = 0x3070;
  static constexpr uint16_t REG_FAULT_STATUS_1 = 0x3071;
  static constexpr uint16_t REG_FAULT_STATUS_2 = 0x3073;
  static constexpr uint16_t REG_FAULT_STATUS_3 = 0x3075;

  static constexpr uint32_t BAUDRATE = 9600;

  // UUGP specifies >=15 s between setting operations.
  static constexpr uint32_t SETTING_INTERVAL_MS = 15000;

  static constexpr uint32_t STATUS_INTERVAL_MS = 500;

  HardwareSerial& serial = Serial2;

  bool serial_initialized = false;
  bool initialization_complete = false;

  uint8_t initialization_step = 0;
  uint8_t status_step = 0;

  uint16_t transaction_id = 0;
  uint16_t expected_transaction_id = 0;
  uint8_t expected_function = 0;
  uint16_t last_ack_transaction_id = 0;
  bool initialization_waiting_for_ack = false;
  uint16_t expected_register = 0;
  uint16_t expected_count = 0;

  uint32_t last_setting_ms = 0;
  uint32_t last_status_ms = 0;
  uint32_t last_response_ms = 0;

  uint8_t rx_buffer[256] = {};
  size_t rx_length = 0;

  bool ensure_serial();

  uint16_t next_transaction();

  void send_frame(uint8_t function, const uint8_t* payload, size_t payload_length);

  void read_registers(uint8_t function, uint16_t address, uint16_t count);

  void write_single(uint16_t address, uint16_t value);

  void write_multiple(uint16_t address, const uint16_t* values, uint16_t count);

  void initialize();

  void initialize_system_time();
  void initialize_current_limiting();
  void initialize_pcs_information();
  void initialize_start_mode();

  void poll_status();

  void process_response(const uint8_t* frame, size_t length);

  void process_input_registers(uint16_t address, const uint16_t* values, uint16_t count);

  void process_holding_registers(uint16_t address, const uint16_t* values, uint16_t count);

  void update_power_limit();

  uint16_t bms_power_limit_W() const;

  uint16_t get_max_pack_voltage_dV() const;
};

extern volatile uint16_t uugp_power_limit_W;
extern volatile uint16_t uugp_discharge_cutoff_soc;
extern volatile bool uugp_allow_discharge_to_home_grid;
extern volatile uint8_t uugp_start_mode;

#endif
#endif
