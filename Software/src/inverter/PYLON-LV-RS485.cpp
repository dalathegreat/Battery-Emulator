#include "PYLON-LV-RS485.h"
#include "../battery/BATTERIES.h"
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "../devboard/utils/events.h"
#include "INVERTERS.h"

bool PylonLV485InverterProtocol::setup(void) {  // Performs one time setup at startup
  memset(rx_buffer, 0, sizeof(rx_buffer));

  auto rx_pin = esp32hal->RS485_RX_PIN();
  auto tx_pin = esp32hal->RS485_TX_PIN();

  if (!esp32hal->alloc_pins(Name, rx_pin, tx_pin)) {
    return false;
  }

  Serial2.begin(baud_rate(), SERIAL_8N1, rx_pin, tx_pin);

  return true;
}

void PylonLV485InverterProtocol::debug_print_frame(const char* direction, const uint8_t* frame, uint16_t len) {
  logging.print(direction);
  logging.print(": ");
  for (uint16_t i = 0; i < len; i++) {
    if (frame[i] >= 0x20 && frame[i] <= 0x7E) {
      logging.print((char)frame[i]);
    } else {
      logging.print("\\x");
      if (frame[i] < 0x10)
        logging.print("0");
      logging.print(frame[i], HEX);
    }
  }
  logging.println();
}

uint8_t PylonLV485InverterProtocol::ascii_to_hex(uint8_t high, uint8_t low) {
  uint8_t result = 0;

  if (high >= '0' && high <= '9')
    result |= (high - '0') << 4;
  else if (high >= 'A' && high <= 'F')
    result |= (high - 'A' + 10) << 4;
  else if (high >= 'a' && high <= 'f')
    result |= (high - 'a' + 10) << 4;

  if (low >= '0' && low <= '9')
    result |= (low - '0');
  else if (low >= 'A' && low <= 'F')
    result |= (low - 'A' + 10);
  else if (low >= 'a' && low <= 'f')
    result |= (low - 'a' + 10);

  return result;
}

uint8_t PylonLV485InverterProtocol::hex_to_ascii_high(uint8_t value) {
  uint8_t high = (value >> 4) & 0x0F;
  return (high < 10) ? ('0' + high) : ('A' + high - 10);
}

uint8_t PylonLV485InverterProtocol::hex_to_ascii_low(uint8_t value) {
  uint8_t low = value & 0x0F;
  return (low < 10) ? ('0' + low) : ('A' + low - 10);
}

void PylonLV485InverterProtocol::append_ascii_byte(uint8_t* buffer, uint16_t& pos, uint8_t value) {
  buffer[pos++] = hex_to_ascii_high(value);
  buffer[pos++] = hex_to_ascii_low(value);
}

void PylonLV485InverterProtocol::append_ascii_word(uint8_t* buffer, uint16_t& pos, uint16_t value) {
  append_ascii_byte(buffer, pos, (value >> 8) & 0xFF);
  append_ascii_byte(buffer, pos, value & 0xFF);
}

uint16_t PylonLV485InverterProtocol::calculate_lchksum(uint16_t lenid) {
  // From PDF page 8: D11D10D9D8 + D7D6D5D4 + D3D2D1D0, sum, mod 16, invert, +1
  uint16_t d11d8 = (lenid >> 8) & 0x0F;
  uint16_t d7d4 = (lenid >> 4) & 0x0F;
  uint16_t d3d0 = lenid & 0x0F;

  uint16_t sum = d11d8 + d7d4 + d3d0;
  uint16_t remainder = sum & 0x0F;
  uint16_t result = ((~remainder) & 0x0F) + 1;

  return (result << 12) | lenid;
}

uint16_t PylonLV485InverterProtocol::calculate_chksum(const uint8_t* frame, uint16_t len) {
  // From PDF page 9: Sum ASCII values, mod 65536, invert, +1
  uint32_t sum = 0;
  for (uint16_t i = 0; i < len; i++) {
    sum += frame[i];
  }
  sum = (~sum) + 1;
  return sum & 0xFFFF;
}

void PylonLV485InverterProtocol::send_response(uint8_t adr, uint8_t rtn, const uint8_t* data, uint16_t data_len) {
  uint8_t response[512];
  uint16_t pos = 0;

  // SOI
  response[pos++] = SOI;

  // VER (protocol version 2.1 = 0x21)
  append_ascii_byte(response, pos, 0x21);

  // ADR
  append_ascii_byte(response, pos, adr);

  // CID1 (always 0x46 for battery data)
  append_ascii_byte(response, pos, CID1_BATTERY);

  // CID2 (response code)
  append_ascii_byte(response, pos, rtn);

  // LENGTH
  uint16_t info_ascii_len = data_len * 2;  // Each data byte becomes 2 ASCII chars
  uint16_t length_with_chksum = calculate_lchksum(info_ascii_len);
  append_ascii_word(response, pos, length_with_chksum);

  // INFO data
  for (uint16_t i = 0; i < data_len; i++) {
    append_ascii_byte(response, pos, data[i]);
  }

  // CHKSUM (excluding SOI, EOI, and CHKSUM itself)
  uint16_t chksum = calculate_chksum(&response[1], pos - 1);
  append_ascii_word(response, pos, chksum);

  // EOI
  response[pos++] = EOI;

  // Debug output
  debug_print_frame("SEND", response, pos);

  // Send response
  Serial2.write(response, pos);
}

void PylonLV485InverterProtocol::send_error_response(uint8_t adr, uint8_t rtn) {
  send_response(adr, rtn, nullptr, 0);
}

// Command handlers
void PylonLV485InverterProtocol::handle_get_protocol_version(uint8_t adr) {
  logging.println("Handling get protocol version");
  send_response(adr, RTN_NORMAL, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_get_manufacturer_info(uint8_t adr) {
  logging.println("Handling get manufacturer info");
  uint8_t data[64] = {0};
  uint16_t pos = 0;

  // Device name (10 bytes)
  const char* name = "US2000B";
  for (int i = 0; i < 10; i++) {
    data[pos++] = (i < (int)strlen(name)) ? name[i] : ' ';
  }

  // Software version (23 bytes)
  const char* sw_version = "V3.3 20210821";
  for (int i = 0; i < 23; i++) {
    data[pos++] = (i < (int)strlen(sw_version)) ? sw_version[i] : ' ';
  }

  // Manufacturer name (20 bytes)
  const char* manufacturer = "PYLONTECH CO.,LTD";
  for (int i = 0; i < 20; i++) {
    data[pos++] = (i < (int)strlen(manufacturer)) ? manufacturer[i] : ' ';
  }

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_analog_value(uint8_t adr, uint8_t command) {
  uint8_t data[256];
  uint16_t pos = 0;

  // Command value
  data[pos++] = command;

  // Number of cells
  data[pos++] = datalayer.battery.info.number_of_cells;

  // Cell voltages (2 bytes each, high byte first)
  for (int i = 0; i < datalayer.battery.info.number_of_cells; i++) {
    data[pos++] = (datalayer.battery.status.cell_voltages_mV[i] >> 8) & 0xFF;
    data[pos++] = datalayer.battery.status.cell_voltages_mV[i] & 0xFF;
  }

  // Number of temperature sensors
  data[pos++] = 5;  //hardcoded, actually we only have min/max

  // BMS temperature (hardcoded since we do not have this value)
  // Temperatures (K = °C * 10 + 2731)
  //2986;  // 25.5°C
  data[pos++] = (2986 >> 8) & 0xFF;
  data[pos++] = 2986 & 0xFF;

  // Cell temperatures (4 groups, send max temp in all of them)
  for (int i = 0; i < 4; i++) {
    data[pos++] = (datalayer.battery.status.temperature_max_dC >> 8) & 0xFF;
    data[pos++] = datalayer.battery.status.temperature_max_dC & 0xFF;
  }

  // Current (int16t, TODO, test this)
  data[pos++] = ((int16_t)datalayer.battery.status.current_dA >> 8) & 0xFF;
  data[pos++] = (int16_t)datalayer.battery.status.current_dA & 0xFF;

  // Module voltage (mV)
  data[pos++] = ((datalayer.battery.status.voltage_dV * 100) >> 8) & 0xFF;
  data[pos++] = (datalayer.battery.status.voltage_dV * 100) & 0xFF;

  // Remain capacity 1 (Should be AH, not WH! TODO, fix this) For capacity >65Ah, use 0xFFFF
  data[pos++] = (datalayer.battery.status.remaining_capacity_Wh >> 8) & 0xFF;
  data[pos++] = datalayer.battery.status.remaining_capacity_Wh & 0xFF;

  // User defined items (2 for ≤65Ah, 4 for >65Ah)
  data[pos++] = 4;

  // Module total capacity 1 (Should be AH, not WH! TODO, fix this) For capacity >65Ah, use 0xFFFF
  data[pos++] = (datalayer.battery.info.total_capacity_Wh >> 8) & 0xFF;
  data[pos++] = datalayer.battery.info.total_capacity_Wh & 0xFF;

  // Cycle number (hardcoded to 2)
  data[pos++] = (2 >> 8) & 0xFF;
  data[pos++] = 2 & 0xFF;

  // Remain capacity 2
  data[pos++] = (datalayer.battery.status.remaining_capacity_Wh >> 8) & 0xFF;
  data[pos++] = datalayer.battery.status.remaining_capacity_Wh & 0xFF;

  // Module total capacity 2
  data[pos++] = (datalayer.battery.info.total_capacity_Wh >> 8) & 0xFF;
  data[pos++] = datalayer.battery.info.total_capacity_Wh & 0xFF;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_system_parameter(uint8_t adr) {
  logging.println("Handling get system parameter");
  uint8_t data[48];
  uint16_t pos = 0;

  // Cell high voltage limit
  data[pos++] = (datalayer.battery.info.max_cell_voltage_mV >> 8) & 0xFF;
  data[pos++] = datalayer.battery.info.max_cell_voltage_mV & 0xFF;

  // Cell low voltage limit
  data[pos++] = ((datalayer.battery.info.min_cell_voltage_mV + 100) >> 8) & 0xFF;
  data[pos++] = (datalayer.battery.info.min_cell_voltage_mV + 100) & 0xFF;

  // Cell under voltage limit
  data[pos++] = (datalayer.battery.info.min_cell_voltage_mV >> 8) & 0xFF;
  data[pos++] = datalayer.battery.info.min_cell_voltage_mV & 0xFF;

  // Charge high temperature limit //3181 = K (45°C)
  data[pos++] = (3181 >> 8) & 0xFF;
  data[pos++] = 3181 & 0xFF;

  // Charge low temperature limit //2531 = K (-20°C)
  data[pos++] = (2531 >> 8) & 0xFF;
  data[pos++] = 2531 & 0xFF;

  // Charge current limit
  data[pos++] = (charge_current_limit >> 8) & 0xFF;
  data[pos++] = charge_current_limit & 0xFF;

  // Module high voltage limit (mV)
  data[pos++] = (charge_voltage_limit >> 8) & 0xFF;
  data[pos++] = charge_voltage_limit & 0xFF;

  // Module low voltage limit (mV) [Alarm]
  data[pos++] = (((datalayer.battery.info.min_design_voltage_dV * 100) + 3000) >> 8) & 0xFF;
  data[pos++] = ((datalayer.battery.info.min_design_voltage_dV * 100) + 3000) & 0xFF;

  // Module under voltage limit [Protect]
  data[pos++] = ((datalayer.battery.info.min_design_voltage_dV * 100) >> 8) & 0xFF;
  data[pos++] = (datalayer.battery.info.min_design_voltage_dV * 100) & 0xFF;

  // Discharge high temperature limit, hardcoded,  3231 =  K (50°C)
  data[pos++] = (3231 >> 8) & 0xFF;
  data[pos++] = 3231 & 0xFF;

  // Discharge low temperature limit, hardcoded, 2531 = K (-20°C)
  data[pos++] = (2531 >> 8) & 0xFF;
  data[pos++] = 2531 & 0xFF;

  // Discharge current limit
  data[pos++] = ((datalayer.battery.status.max_discharge_current_dA * 10) >> 8) & 0xFF;
  data[pos++] = (datalayer.battery.status.max_discharge_current_dA * 10) & 0xFF;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_alarm_info(uint8_t adr, uint8_t command) {
  logging.print("Handling get alarm info, command=0x");
  logging.println(command, HEX);
  uint8_t data[256];
  uint16_t pos = 0;

  // Command value
  data[pos++] = command;

  // Number of cells
  data[pos++] = datalayer.battery.info.number_of_cells;

  // Cell voltage status (1=alarm)
  for (int i = 0; i < datalayer.battery.info.number_of_cells; i++) {
    data[pos++] =
        (datalayer.battery.status.cell_voltages_mV[i] > 4200 || datalayer.battery.status.cell_voltages_mV[i] < 2800)
            ? 0x01
            : 0x00;
  }

  // Number of temperatures
  data[pos++] = datalayer.battery.info.number_of_cells;

  // BMS temperature status (essentially not checked)
  data[pos++] = (2986 > 3231 || 2986 < 2531) ? 0x02 : 0x00;

  // Cell temperature status (essentially not checked)
  for (int i = 0; i < 4; i++) {
    data[pos++] = (3000 > 3231 || 3000 < 2531) ? 0x02 : 0x00;
  }

  // Charge current status (essentially not checked)
  data[pos++] = (1000 > 5000) ? 0x02 : 0x00;

  // Module voltage status, Check if value is between 45V-54,6V
  data[pos++] =
      ((datalayer.battery.status.voltage_dV * 100) > 54600 || (datalayer.battery.status.voltage_dV * 100) < 45000)
          ? 0x02
          : 0x00;

  // Discharge current status (essentially not checked)
  data[pos++] = (1000 < -5000) ? 0x02 : 0x00;

  // Status 1-5
  data[pos++] = 0x00;
  data[pos++] = 0x06;  // Both charge/discharge MOSFETs on
  data[pos++] = 0x00;
  data[pos++] = 0x00;
  data[pos++] = 0x00;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_charge_discharge_info(uint8_t adr, uint8_t command) {
  logging.println("Handling get charge/discharge info");
  uint8_t data[10];
  uint16_t pos = 0;

  data[pos++] = command;

  // Charge voltage limit
  data[pos++] = (charge_voltage_limit >> 8) & 0xFF;
  data[pos++] = charge_voltage_limit & 0xFF;

  // Discharge voltage limit
  data[pos++] = (discharge_voltage_limit >> 8) & 0xFF;
  data[pos++] = discharge_voltage_limit & 0xFF;

  // Max charge current
  data[pos++] = (max_charge_current >> 8) & 0xFF;
  data[pos++] = max_charge_current & 0xFF;

  // Max discharge current
  data[pos++] = (max_discharge_current >> 8) & 0xFF;
  data[pos++] = max_discharge_current & 0xFF;

  // Charge/discharge status
  data[pos++] = charge_discharge_status;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_serial_number(uint8_t adr, uint8_t command) {
  logging.println("Handling get serial number");
  uint8_t data[17];
  uint16_t pos = 0;

  data[pos++] = command;

  // Serial number (16 bytes ASCII)
  const char* sn = "PYL1234567890123";
  for (int i = 0; i < 16; i++) {
    data[pos++] = sn[i];
  }

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_set_charge_discharge_info(uint8_t adr, const uint8_t* data, uint16_t len) {
  logging.println("Handling set charge/discharge info");
  if (len >= 9) {
    // Parse and update settings
    charge_voltage_limit = (data[1] << 8) | data[2];
    discharge_voltage_limit = (data[3] << 8) | data[4];
    max_charge_current = (data[5] << 8) | data[6];
    max_discharge_current = (data[7] << 8) | data[8];
    logging.print("Updated charge voltage limit");
    // Note: According to protocol, this command needs to be sent periodically
    // If not received for 10 seconds, battery reverts to automatic settings
    last_command_time = millis();
  }

  send_response(adr, RTN_NORMAL, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_turn_off(uint8_t adr, uint8_t command) {
  logging.println("Handling turn off command");
  // Implement shutdown logic here
  send_response(adr, RTN_NORMAL, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_get_software_version(uint8_t adr, uint8_t command) {
  logging.println("Handling get software version");
  uint8_t data[6];
  uint16_t pos = 0;

  data[pos++] = command;

  // Software version (5 bytes: manufacturer version + main line version)
  // Example: "V3.3"
  data[pos++] = 'V';
  data[pos++] = '3';
  data[pos++] = '.';
  data[pos++] = '3';
  data[pos++] = 0x00;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::receive() {
  currentMillis = millis();

  // Debug heartbeat every 5 seconds
  if (currentMillis - last_debug_time > 5000) {
    logging.println("PylonLV485 protocol running...");
    last_debug_time = currentMillis;
  }

  while (Serial2.available()) {
    uint8_t byte = Serial2.read();

    if (byte == SOI) {
      incoming_message_counter = RS485_HEALTHY;
      // Start of new frame
      rx_index = 0;
      rx_buffer[rx_index++] = byte;
    } else if (byte == EOI && rx_index > 0) {
      // End of frame
      rx_buffer[rx_index++] = byte;

      // Process frame
      debug_print_frame("RECV", rx_buffer, rx_index);

      // Minimum frame length check
      if (rx_index >= 18) {
        // Parse frame (positions from PDF page 6)
        // SOI(1) + VER(2) + ADR(2) + CID1(2) + CID2(2) + LENGTH(4) + INFO(n) + CHKSUM(4) + EOI(1)

        // Extract fields
        uint8_t ver = ascii_to_hex(rx_buffer[1], rx_buffer[2]);
        uint8_t adr = ascii_to_hex(rx_buffer[3], rx_buffer[4]);
        uint8_t cid1 = ascii_to_hex(rx_buffer[5], rx_buffer[6]);
        uint8_t cid2 = ascii_to_hex(rx_buffer[7], rx_buffer[8]);

        // Get LENGTH field
        uint16_t len_high = ascii_to_hex(rx_buffer[9], rx_buffer[10]);
        uint16_t len_low = ascii_to_hex(rx_buffer[11], rx_buffer[12]);
        uint16_t length = (len_high << 8) | len_low;
        uint16_t info_ascii_len = length & 0x0FFF;  // Lower 12 bits are LENID

        logging.print("Frame: VER=0x");
        logging.print(ver, HEX);
        logging.print(" ADR=0x");
        logging.print(adr, HEX);
        logging.print(" CID1=0x");
        logging.print(cid1, HEX);
        logging.print(" CID2=0x");
        logging.print(cid2, HEX);
        logging.print(" LENID=");
        logging.println(info_ascii_len);

        // Verify CID1
        if (cid1 != CID1_BATTERY) {
          logging.println("Invalid CID1");
          continue;
        }

        // Extract INFO data
        uint8_t info_data[256];
        uint16_t info_len = 0;

        for (int i = 0; i < info_ascii_len / 2; i++) {
          uint8_t high = rx_buffer[13 + i * 2];
          uint8_t low = rx_buffer[13 + i * 2 + 1];
          info_data[info_len++] = ascii_to_hex(high, low);
        }

        // Handle command
        switch (cid2) {
          case CMD_GET_PROTOCOL_VERSION:
            handle_get_protocol_version(adr);
            break;

          case CMD_GET_MANUFACTURER_INFO:
            handle_get_manufacturer_info(adr);
            break;

          case CMD_GET_ANALOG_VALUE:
            if (info_len >= 1) {
              handle_get_analog_value(adr, info_data[0]);
            }
            break;

          case CMD_GET_SYSTEM_PARAM:
            handle_get_system_parameter(adr);
            break;

          case CMD_GET_ALARM_INFO:
            if (info_len >= 1) {
              handle_get_alarm_info(adr, info_data[0]);
            }
            break;

          case CMD_GET_CHARGE_DISCHARGE_INFO:
            if (info_len >= 1) {
              handle_get_charge_discharge_info(adr, info_data[0]);
            }
            break;

          case CMD_GET_SERIAL_NUMBER:
            if (info_len >= 1) {
              handle_get_serial_number(adr, info_data[0]);
            }
            break;

          case CMD_SET_CHARGE_DISCHARGE_INFO:
            handle_set_charge_discharge_info(adr, info_data, info_len);
            break;

          case CMD_TURN_OFF:
            if (info_len >= 1) {
              handle_turn_off(adr, info_data[0]);
            }
            break;

          case CMD_GET_SOFTWARE_VERSION:
            if (info_len >= 1) {
              handle_get_software_version(adr, info_data[0]);
            }
            break;
          case CMD_UNKNOWN_61:
          case CMD_UNKNOWN_63:
            Serial.print("Handling unknown command: 0x");
            Serial.println(cid2, HEX);
            // Respond with success - these might be keepalive or discovery commands
            send_response(adr, RTN_NORMAL, nullptr, 0);
            break;
          default:
            logging.print("Unknown CID2: 0x");
            logging.println(cid2, HEX);
            send_error_response(adr, RTN_CID2_INVALID);
            break;
        }
      }

      rx_index = 0;
    } else if (rx_index < RX_BUFFER_SIZE - 1) {
      rx_buffer[rx_index++] = byte;
    } else {
      logging.println("RX buffer overflow");
      rx_index = 0;
    }
  }
}

void PylonLV485InverterProtocol::update_values() {

  max_discharge_current = (datalayer.battery.status.max_charge_current_dA * 10);
  max_charge_current = (datalayer.battery.status.max_charge_current_dA * 10);

  charge_voltage_limit = (datalayer.battery.info.max_design_voltage_dV * 100);
  discharge_voltage_limit = (datalayer.battery.info.min_design_voltage_dV * 100);

  charge_current_limit = (datalayer.battery.status.max_charge_current_dA * 10);
  discharge_current_limit = (datalayer.battery.status.max_discharge_current_dA * 10);

  charge_discharge_status = 0xC0;  // Bit7: Charge enable, Bit6: Discharge enable TODO, utilize this

  if (incoming_message_counter > 0) {
    incoming_message_counter--;
  }

  if (incoming_message_counter == 0) {
    set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  } else {
    clear_event(EVENT_MODBUS_INVERTER_MISSING);
  }
}
