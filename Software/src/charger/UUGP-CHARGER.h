#ifndef UUGP_CHARGER_H
#define UUGP_CHARGER_H

#include <Arduino.h>
#include <HardwareSerial.h>

#include "CanCharger.h"
#include "../communication/Transmitter.h"
#include "../communication/rs485/comm_rs485.h"

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
  static constexpr uint16_t REG_DAY = 0x4004;
  static constexpr uint16_t REG_HOUR = 0x4005;
  static constexpr uint16_t REG_MINUTE = 0x4006;
  static constexpr uint16_t REG_SECOND = 0x4007;

  static constexpr uint16_t REG_POWER_LIMIT = 0x4011;
  static constexpr uint16_t REG_DISCHARGE_CUTOFF_SOC = 0x4012;
  static constexpr uint16_t REG_CONTROL_MODE = 0x4013;

  static constexpr uint16_t REG_START_MODE = 0x4033;

  static constexpr uint16_t REG_VBUS_UPPER = 0x4050;
  static constexpr uint16_t REG_VBUS_LOWER = 0x4051;
  static constexpr uint16_t REG_PCS_MODEL = 0x4052;

  static constexpr uint16_t REG_EV_VOLTAGE = 0x3020;
  static constexpr uint16_t REG_EV_CURRENT = 0x3021;
  static constexpr uint16_t REG_POWER_FACTOR = 0x3023;
  static constexpr uint16_t REG_MAX_OUTPUT_VOLTAGE = 0x3026;
  static constexpr uint16_t REG_MAX_OUTPUT_CURRENT = 0x3027;
  static constexpr uint16_t REG_RATED_POWER = 0x3028;

  static constexpr uint16_t REG_DC_VOLTAGE = 0x302F;
  static constexpr uint16_t REG_DC_CURRENT = 0x3030;
  static constexpr uint16_t REG_DC_DERATING = 0x3031;

  static constexpr uint16_t REG_CHARGE_MODE = 0x3040;
  static constexpr uint16_t REG_CHARGE_VOLTAGE = 0x3041;
  static constexpr uint16_t REG_CHARGE_CURRENT = 0x3042;
  static constexpr uint16_t REG_ACTIVE_POWER = 0x3043;
  static constexpr uint16_t REG_VEHICLE_SOC = 0x3044;

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

  uint16_t expected_register = 0;
  uint16_t expected_count = 0;

  uint32_t last_setting_ms = 0;
  uint32_t last_status_ms = 0;
  uint32_t last_response_ms = 0;

  uint8_t rx_buffer[256] = {};
  size_t rx_length = 0;

  bool ensure_serial();

  uint16_t next_transaction();

  void send_frame(uint8_t function,
                  const uint8_t* payload,
                  size_t payload_length);

  void read_registers(uint8_t function,
                      uint16_t address,
                      uint16_t count);

  void write_single(uint16_t address,
                    uint16_t value);

  void write_multiple(uint16_t address,
                      const uint16_t* values,
                      uint16_t count);

  void initialize();

  void initialize_system_time();
  void initialize_current_limiting();
  void initialize_pcs_information();
  void initialize_start_mode();

  void poll_status();

  void process_response(const uint8_t* frame,
                        size_t length);

  void process_input_registers(uint16_t address,
                               const uint16_t* values,
                               uint16_t count);

  void process_holding_registers(uint16_t address,
                                 const uint16_t* values,
                                 uint16_t count);

  void update_power_limit();

  uint16_t bms_power_limit_W() const;

  uint16_t get_max_pack_voltage_dV() const;
};

extern volatile uint16_t uugp_power_limit_W;
extern volatile uint16_t uugp_discharge_cutoff_soc;
extern volatile bool uugp_allow_discharge_to_home_grid;

#endif
