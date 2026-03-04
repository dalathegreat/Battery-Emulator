#ifndef PYLON_LV_RS485_H
#define PYLON_LV_RS485_H
#include <stdint.h>
#include "Rs485InverterProtocol.h"

class PylonLV485InverterProtocol : public Rs485InverterProtocol {
 public:
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

  int baud_rate() { return 9600; }
  // Binary mode frame types based on your log
    enum BinaryFrameType {
        FRAME_UNKNOWN = 0,
        FRAME_201 = 0x201,  // Pattern starting with 0x10 0x08
        FRAME_202 = 0x202,
        FRAME_203 = 0x203,
        FRAME_204 = 0x204,
        FRAME_205 = 0x205,
        FRAME_206 = 0x206,
        FRAME_210 = 0x210,  // Pattern starting with 0x0C
        FRAME_211 = 0x211,
        FRAME_212 = 0x212,
        FRAME_213 = 0x213,
        FRAME_246 = 0x246,  // Pattern with 0x3D (===)
        FRAME_261 = 0x261,
        FRAME_262 = 0x262,
        FRAME_263 = 0x263,
        FRAME_264 = 0x264
    };
    
    struct BinaryFrame {
        uint16_t id;
        uint8_t data[8];
        uint8_t len;
    };
    
    void process_binary_frame(const uint8_t* frame, uint16_t len);
    void handle_binary_command(uint16_t frame_id, const uint8_t* data, uint8_t len);

        // Mode detection
    bool binary_mode = false;
    bool mode_detected = false;
    int ascii_frame_count = 0;
    int binary_frame_count = 0;


  // Protocol constants
  static constexpr uint8_t SOI = 0x7E;           // Start of frame '~'
  static constexpr uint8_t EOI = 0x0D;           // End of frame (CR)
  static constexpr uint8_t CID1_BATTERY = 0x46;  // Battery data group

  // CID2 command codes (from PDF page 10-11)
  enum CommandCode {
    CMD_GET_ANALOG_VALUE = 0x42,
    CMD_GET_ALARM_INFO = 0x44,
    CMD_GET_SYSTEM_PARAM = 0x47,
    CMD_GET_PROTOCOL_VERSION = 0x4F,
    CMD_GET_MANUFACTURER_INFO = 0x51,
    CMD_GET_CHARGE_DISCHARGE_INFO = 0x92,
    CMD_GET_SERIAL_NUMBER = 0x93,
    CMD_SET_CHARGE_DISCHARGE_INFO = 0x94,
    CMD_TURN_OFF = 0x95,
    CMD_GET_SOFTWARE_VERSION = 0x96,
    // Unknown/proprietary commands - respond with success
    CMD_UNKNOWN_61 = 0x61,
    CMD_UNKNOWN_63 = 0x63
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
  unsigned long last_debug_time = 0;

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
  uint8_t hex_to_ascii_high(uint8_t value);
  uint8_t hex_to_ascii_low(uint8_t value);
  void append_ascii_byte(uint8_t* buffer, uint16_t& pos, uint8_t value);
  void append_ascii_word(uint8_t* buffer, uint16_t& pos, uint16_t value);
  uint16_t calculate_lchksum(uint16_t lenid);
  uint16_t calculate_chksum(const uint8_t* frame, uint16_t len);
  void send_response(uint8_t adr, uint8_t rtn, const uint8_t* data, uint16_t data_len);
  void send_error_response(uint8_t adr, uint8_t rtn);
  void debug_print_frame(const char* direction, const uint8_t* frame, uint16_t len);

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
};

#endif
