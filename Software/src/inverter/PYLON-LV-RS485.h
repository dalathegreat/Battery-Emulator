#ifndef PYLON_LV_RS485_H
#define PYLON_LV_RS485_H
#include <stdint.h>
#include <string>
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

  // Protocol constants
  static constexpr const char* PROTOCOL_VERSION = "20";
  static constexpr const char* RESPONSE_ADDRESS = "02";

  // Helper functions
  void route_frame_request(const std::string& frame_str);
  void handle_command_61();
  void handle_command_62();
  void handle_command_63();
  std::string calculate_checksum(const std::string& frame_data);
  std::string calculate_length_field(int info_len);

  // Member variables for state
  uint32_t last_update_ms = 0;
  uint32_t last_cmd63_ms = 0;
  uint32_t update_timeout_ms = 60000;
  std::string rx_buffer;
  bool is_data_valid = false;

  // Dynamic data - defaults for safety
  uint16_t soc_percent = 80;            // 80% default
  uint16_t voltage_mv = 48000;          // 48V default
  int16_t current_ca = 100;             // 1A default
  uint16_t temp_deci_k = 2980;          // 25°C default (K*10)
  uint16_t max_cell_v = 3350;           // 3.35V default
  uint16_t min_cell_v = 3350;           // 3.35V default
  uint16_t max_charge_v_mv = 52000;     // 52V default
  uint16_t min_discharge_v_mv = 48000;  // 48V default
  uint16_t max_charge_i_da = 0;         // 0A default
  uint16_t max_discharge_i_da = 0;      // 0A default
};

#endif
