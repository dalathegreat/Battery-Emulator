#include "PYLON-LV-RS485.h"
#include "../battery/BATTERIES.h"
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "../devboard/utils/events.h"
#include "INVERTERS.h"

bool PylonLV485InverterProtocol::setup() {
  // Initialize Serial2 with proper pins from HAL
  auto rx_pin = esp32hal->RS485_RX_PIN();
  auto tx_pin = esp32hal->RS485_TX_PIN();

  if (!esp32hal->alloc_pins(Name, rx_pin, tx_pin)) {
    logging.println("Failed to allocate RS485 pins!");
    return false;
  }

  //initialize safe defaults before we start receiving real data
  datalayer.battery.status.voltage_dV = 480;  // 48.0V

  Serial2.begin(baud_rate(), SERIAL_8N1, rx_pin, tx_pin);
  return true;
}

void PylonLV485InverterProtocol::update_values() {

  voltage_mv = datalayer.battery.status.voltage_dV * 100;

  current_ca = datalayer.battery.status.current_dA * 10;

  soc_percent = datalayer.battery.status.reported_soc / 100;

  temp_deci_k = (datalayer.battery.status.temperature_max_dC + 273.15) * 10;

  max_cell_v = datalayer.battery.status.cell_max_voltage_mV;

  min_cell_v = datalayer.battery.status.cell_min_voltage_mV;

  max_charge_v_mv = datalayer.battery.info.max_design_voltage_dV * 100;

  min_discharge_v_mv = datalayer.battery.info.min_design_voltage_dV * 100;

  max_charge_i_da = datalayer.battery.status.max_charge_current_dA;

  max_discharge_i_da = datalayer.battery.status.max_discharge_current_dA;

  if (incoming_message_counter > 0) {
    incoming_message_counter--;
  }

  if (incoming_message_counter == 0) {
    set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  } else {
    clear_event(EVENT_MODBUS_INVERTER_MISSING);
  }
}

void PylonLV485InverterProtocol::receive() {
  // Read incoming RS485 data
  while (Serial2.available()) {
    char byte = Serial2.read();

    if (byte == '~') {
      rx_buffer.clear();
      rx_buffer += byte;
    } else if (!rx_buffer.empty()) {
      rx_buffer += byte;
      if (byte == '\r') {
        incoming_message_counter = RS485_HEALTHY;
        // logging.printf("RX: Frame received (%u bytes): %s\n", rx_buffer.length(), rx_buffer.c_str());
        route_frame_request(rx_buffer);
        rx_buffer.clear();
      }
    }
  }
}

void PylonLV485InverterProtocol::route_frame_request(const std::string& frame_str) {
  // Minimal validation
  if (frame_str.length() < 18) {
    // logging.printf("RX: Frame too short (%u bytes)\n", frame_str.length());
    return;
  }

  // Extract command (CID2) from positions 7-8
  std::string cid2 = frame_str.substr(7, 2);

  if (cid2 == "61") {
    handle_command_61();
  } else if (cid2 == "62") {
    handle_command_62();
  } else if (cid2 == "63") {
    handle_command_63();
  } else {
    logging.printf("RX: Unknown command 0x%s\n", cid2.c_str());
  }
}

void PylonLV485InverterProtocol::handle_command_61() {
  // Command 0x61: System Analog Value (26 values, 49 bytes total)
  // Uses safe defaults, overridden by datalayer values if available

  uint16_t cycles = 100;
  uint8_t soh = 100;
  uint16_t max_cell_v = 3350;
  uint16_t min_cell_v = 3350;
  uint16_t avg_cell_temp = temp_deci_k;
  uint16_t max_cell_temp = temp_deci_k;
  uint16_t min_cell_temp = temp_deci_k;
  uint16_t avg_mosfet_temp = temp_deci_k;
  uint16_t max_mosfet_temp = temp_deci_k;
  uint16_t min_mosfet_temp = temp_deci_k;
  uint16_t avg_bms_temp = temp_deci_k;
  uint16_t max_bms_temp = temp_deci_k;
  uint16_t min_bms_temp = temp_deci_k;

  // Override with real datalayer values if available
  if (datalayer.battery.status.cell_max_voltage_mV > 0)
    max_cell_v = datalayer.battery.status.cell_max_voltage_mV;
  if (datalayer.battery.status.cell_min_voltage_mV > 0)
    min_cell_v = datalayer.battery.status.cell_min_voltage_mV;
  if (datalayer.battery.status.temperature_max_dC > -273)
    max_cell_temp = (datalayer.battery.status.temperature_max_dC + 273.15) * 10;
  if (datalayer.battery.status.temperature_min_dC > -273)
    min_cell_temp = (datalayer.battery.status.temperature_min_dC + 273.15) * 10;

  // Log what we're sending for 0x61
  if (datalayer.system.info.web_logging_active) {
    // logging.printf("[FakePylontech485] TX 0x61: V=%u mV, I=%d cA, SOC=%u%%, Cycles=%u, SOH=%u%%, MaxCell=%u mV, MinCell=%u mV\n",
    //                voltage_mv, current_ca, soc_percent, cycles, soh, max_cell_v, min_cell_v);
  }

  // Build INFO payload (26 values, 49 bytes when expanded to ASCII)
  char info_payload[150];  // Increased buffer size to avoid truncation warnings
  snprintf(info_payload, sizeof(info_payload),
           "%04X%04X%02X%04X%04X%02X%02X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X%04X",
           voltage_mv, (uint16_t)current_ca, soc_percent, cycles, cycles, soh, soh, max_cell_v, 0x0101, min_cell_v,
           0x0108, avg_cell_temp, max_cell_temp, 0x0103, min_cell_temp, 0x0109, avg_mosfet_temp, max_mosfet_temp,
           0x0101, min_mosfet_temp, 0x0101, avg_bms_temp, max_bms_temp, 0x0101, min_bms_temp, 0x0101);

  std::string info_str(info_payload);
  std::string length_field = calculate_length_field(info_str.length());
  std::string frame_data =
      std::string(PROTOCOL_VERSION) + std::string(RESPONSE_ADDRESS) + "4661" + length_field + info_str;
  std::string checksum = calculate_checksum(frame_data);
  std::string full_frame = "~" + frame_data + checksum + "\r";

  Serial2.write((const uint8_t*)full_frame.c_str(), full_frame.length());
}

void PylonLV485InverterProtocol::handle_command_62() {
  // Command 0x62: System Alarm/Protection Status (4 bytes)
  // All bits start as 0 (no alarms/protections triggered)

  uint8_t alarm_status_1 = 0;
  uint8_t alarm_status_2 = 0;
  uint8_t protection_status_1 = 0;
  uint8_t protection_status_2 = 0;

  // Could check datalayer for alarm states and set bits accordingly
  // For now, just send all zeros (system OK)

  char info_payload[9];
  snprintf(info_payload, sizeof(info_payload), "%02X%02X%02X%02X", alarm_status_1, alarm_status_2, protection_status_1,
           protection_status_2);

  std::string info_str(info_payload);
  std::string length_field = calculate_length_field(info_str.length());
  std::string frame_data =
      std::string(PROTOCOL_VERSION) + std::string(RESPONSE_ADDRESS) + "4662" + length_field + info_str;
  std::string checksum = calculate_checksum(frame_data);
  std::string full_frame = "~" + frame_data + checksum + "\r";

  Serial2.write((const uint8_t*)full_frame.c_str(), full_frame.length());

  if (datalayer.system.info.web_logging_active) {
    // logging.printf("[FakePylontech485] Frame TX 0x62: %s\n", full_frame.c_str());
  }
}

void PylonLV485InverterProtocol::handle_command_63() {
  // Command 0x63: Charge/Discharge Management (9 bytes)
  // Format: Max_Charge_Voltage(2) | Min_Discharge_Voltage(2) | Max_Charge_Current(2) | Max_Discharge_Current(2) | Status(1)

  uint8_t status_byte = 0xC0;  // Bit 7 & 6: Charge & Discharge enabled

  char info_payload[19];
  snprintf(info_payload, sizeof(info_payload), "%04X%04X%04X%04X%02X", max_charge_v_mv, min_discharge_v_mv,
           max_charge_i_da, max_discharge_i_da, status_byte);

  std::string info_str(info_payload);
  std::string length_field = calculate_length_field(info_str.length());
  std::string frame_data =
      std::string(PROTOCOL_VERSION) + std::string(RESPONSE_ADDRESS) + "4663" + length_field + info_str;
  std::string checksum = calculate_checksum(frame_data);
  std::string full_frame = "~" + frame_data + checksum + "\r";

  Serial2.write((const uint8_t*)full_frame.c_str(), full_frame.length());

  last_cmd63_ms = millis();
}

std::string PylonLV485InverterProtocol::calculate_checksum(const std::string& frame_data) {
  // Two's complement checksum: sum all bytes, invert, add 1
  uint16_t sum = 0;
  for (char c : frame_data) {
    sum += (uint8_t)c;
  }
  sum = ~sum;
  sum += 1;

  char checksum_str[5];
  snprintf(checksum_str, sizeof(checksum_str), "%04X", sum);
  return std::string(checksum_str);
}

std::string PylonLV485InverterProtocol::calculate_length_field(int info_len) {
  // LENGTH field format: LCHKSUM(1 hex digit) + LENID(3 hex digits)
  // LENID = info_len in hex (number of ASCII hex characters)
  // LCHKSUM = ~(sum of LENID nibbles) + 1, mod 16

  uint8_t n1 = (info_len >> 8) & 0x0F;
  uint8_t n2 = (info_len >> 4) & 0x0F;
  uint8_t n3 = info_len & 0x0F;
  uint8_t sum = n1 + n2 + n3;
  uint8_t lchksum = (~sum & 0x0F) + 1;

  char length_str[20];  // Increased buffer size to avoid truncation warnings
  snprintf(length_str, sizeof(length_str), "%1X%03X", lchksum, info_len);
  return std::string(length_str);
}
