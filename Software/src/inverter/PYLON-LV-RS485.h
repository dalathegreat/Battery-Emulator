#ifndef PYLON_LV_RS485_H
#define PYLON_LV_RS485_H
#include <stdint.h>
#include "Rs485InverterProtocol.h"

class PylonLV485InverterProtocol : public Rs485InverterProtocol {
 public:
  PylonLV485InverterProtocol();
  const char* name() override { return Name; }
  bool setup() override;
  void receive();
  void update_values();
  static constexpr const char* Name = "Pylon low voltage via RS485";

 private:
  /* How many value updates we can go without inverter gets reported as missing
  e.g. value set to 12, 12*5sec=60seconds without comm before event is raised */
  const int RS485_HEALTHY = 12;
  uint8_t incoming_message_counter = RS485_HEALTHY;

  int baud_rate() { return 115200; }
  // Protocol constants
  static constexpr uint8_t SOI = 0x7E;           // Start of frame
  static constexpr uint8_t EOI = 0x0D;           // End of frame (CR)
  static constexpr uint8_t CID1_BATTERY = 0x46;  // Battery data group

  // Frame positions
  enum FramePos {
    POS_SOI = 0,
    POS_VER = 1,
    POS_ADR_H = 2,
    POS_ADR_L = 3,
    POS_CID1 = 4,
    POS_CID2 = 5,
    POS_LEN_H = 6,
    POS_LEN_L = 7,
    POS_INFO_START = 8
  };

  // Response codes
  enum ResponseCode {
    RTN_NORMAL = 0x00,
    RTN_VER_ERROR = 0x01,
    RTN_CHKSUM_ERROR = 0x02,
    RTN_LCHKSUM_ERROR = 0x03,
    RTN_CID2_INVALID = 0x04,
    RTN_CMD_FORMAT_ERROR = 0x05,
    RTN_DATA_INVALID = 0x06,
    RTN_ADR_ERROR = 0x90,
    RTN_COMM_ERROR = 0x91
  };

  // Buffer for incoming messages
  static constexpr int RX_BUFFER_SIZE = 300;
  uint8_t rx_buffer[RX_BUFFER_SIZE];
  uint16_t rx_index = 0;

  // Timing
  unsigned long currentMillis = 0;
  unsigned long last_command_time = 0;

  // Status bits
  uint8_t status1 = 0x00;
  uint8_t status2 = 0x06;  // Both charge/discharge MOSFETs on
  uint8_t status3 = 0x00;
  uint8_t status4 = 0x00;
  uint8_t status5 = 0x00;

  // System parameters
  uint16_t cell_high_voltage_limit = 3650;      // mV
  uint16_t cell_low_voltage_limit = 3000;       // mV (alarm)
  uint16_t cell_under_voltage_limit = 2800;     // mV (protect)
  uint16_t charge_high_temp_limit = 3181;       // K (45°C)
  uint16_t charge_low_temp_limit = 2731;        // K (0°C)
  uint16_t charge_current_limit = 5000;         // A*100 (50.00A)
  uint16_t module_high_voltage_limit = 54600;   // mV
  uint16_t module_low_voltage_limit = 48000;    // mV (alarm)
  uint16_t module_under_voltage_limit = 45000;  // mV (protect)
  uint16_t discharge_high_temp_limit = 3231;    // K (50°C)
  uint16_t discharge_low_temp_limit = 2531;     // K (-20°C)
  uint16_t discharge_current_limit = 5000;      // A*100 (50.00A)

  // Charge/discharge management
  uint16_t charge_voltage_limit = 54600;     // mV
  uint16_t discharge_voltage_limit = 48000;  // mV
  uint16_t max_charge_current = 5000;        // A*100
  uint16_t max_discharge_current = 5000;     // A*100
  uint8_t charge_discharge_status = 0xC0;    // Bit7: Charge enable, Bit6: Discharge enable

  // Helper functions
  uint8_t ascii_to_hex(uint8_t high, uint8_t low);
  uint16_t calculate_lchksum(uint16_t lenid);
  uint16_t calculate_chksum(const uint8_t* frame, uint16_t len);
  void send_response(uint8_t adr, uint8_t rtn, const uint8_t* data, uint16_t data_len);
  void send_error_response(uint8_t adr, uint8_t rtn);

  // Command handlers
  void handle_get_protocol_version(uint8_t adr);
  void handle_get_manufacturer_info(uint8_t adr);
  void handle_get_analog_value(uint8_t adr, uint8_t command);
  void handle_get_system_parameter(uint8_t adr);
  void handle_get_alarm_info(uint8_t adr, uint8_t command);
  void handle_get_charge_discharge_info(uint8_t adr, uint8_t command);
  void handle_get_serial_number(uint8_t adr, uint8_t command);
  void handle_set_charge_discharge_info(uint8_t adr, const uint8_t* data, uint16_t len);
  void handle_turn_off(uint8_t adr, uint8_t command);
  void handle_get_software_version(uint8_t adr, uint8_t command);

  // Data conversion helpers
  uint8_t hex_to_ascii_high(uint8_t value);
  uint8_t hex_to_ascii_low(uint8_t value);
  void append_ascii_byte(uint8_t* buffer, uint16_t& pos, uint8_t value);
  void append_ascii_word(uint8_t* buffer, uint16_t& pos, uint16_t value);
};

#endif
