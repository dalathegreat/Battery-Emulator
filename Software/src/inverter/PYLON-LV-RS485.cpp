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
  // D11D10D9D8 + D7D6D5D4 + D3D2D1D0, then mod 16, invert, +1
  uint16_t d11d8 = (lenid >> 8) & 0x0F;
  uint16_t d7d4 = (lenid >> 4) & 0x0F;
  uint16_t d3d0 = lenid & 0x0F;

  uint16_t sum = d11d8 + d7d4 + d3d0;
  uint16_t remainder = sum & 0x0F;
  uint16_t result = ((~remainder) & 0x0F) + 1;

  return (result << 12) | lenid;
}

uint16_t PylonLV485InverterProtocol::calculate_chksum(const uint8_t* frame, uint16_t len) {
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
  response[pos++] = '2';
  response[pos++] = '1';

  // ADR (2 bytes ASCII)
  response[pos++] = hex_to_ascii_high(adr);
  response[pos++] = hex_to_ascii_low(adr);

  // CID1 (always 0x46 for battery data)
  response[pos++] = '4';
  response[pos++] = '6';

  // CID2 (response code)
  response[pos++] = hex_to_ascii_high(rtn);
  response[pos++] = hex_to_ascii_low(rtn);

  // LENGTH (including LENID and LCHKSUM)
  uint16_t info_ascii_len = data_len * 2;  // Each byte becomes 2 ASCII chars
  uint16_t lenid = info_ascii_len;
  uint16_t length_with_chksum = calculate_lchksum(lenid);

  response[pos++] = hex_to_ascii_high((length_with_chksum >> 8) & 0xFF);
  response[pos++] = hex_to_ascii_low((length_with_chksum >> 8) & 0xFF);
  response[pos++] = hex_to_ascii_high(length_with_chksum & 0xFF);
  response[pos++] = hex_to_ascii_low(length_with_chksum & 0xFF);

  // INFO (data in ASCII format)
  for (uint16_t i = 0; i < data_len; i++) {
    response[pos++] = hex_to_ascii_high(data[i]);
    response[pos++] = hex_to_ascii_low(data[i]);
  }

  // CHKSUM (excluding SOI, EOI, and CHKSUM itself)
  uint16_t chksum = calculate_chksum(&response[1], pos - 1);
  response[pos++] = hex_to_ascii_high((chksum >> 8) & 0xFF);
  response[pos++] = hex_to_ascii_low((chksum >> 8) & 0xFF);
  response[pos++] = hex_to_ascii_high(chksum & 0xFF);
  response[pos++] = hex_to_ascii_low(chksum & 0xFF);

  // EOI
  response[pos++] = EOI;

  // Send response
  Serial2.write(response, pos);
}

void PylonLV485InverterProtocol::send_error_response(uint8_t adr, uint8_t rtn) {
  send_response(adr, rtn, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_get_protocol_version(uint8_t adr) {
  // Protocol version 2.1
  send_response(adr, RTN_NORMAL, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_get_manufacturer_info(uint8_t adr) {
  uint8_t data[32] = {0};
  uint16_t pos = 0;

  // Battery name (10 bytes ASCII)
  const char* name = "PYLON";
  for (int i = 0; i < 10; i++) {
    data[pos++] = (i < strlen(name)) ? name[i] : ' ';
  }

  // Software version (23 bytes ASCII)
  const char* sw_version = "V3.3";
  for (int i = 0; i < 23; i++) {
    data[pos++] = (i < strlen(sw_version)) ? sw_version[i] : ' ';
  }

  // Manufacturer name (20 bytes ASCII)
  const char* manufacturer = "PYLONTECH";
  for (int i = 0; i < 20; i++) {
    data[pos++] = (i < strlen(manufacturer)) ? manufacturer[i] : ' ';
  }

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_system_basic_info(uint8_t adr) {
  uint8_t data[256];
  uint16_t pos = 0;

  // Battery name (10 bytes, space‑padded)
  const char* batt_name = "PYLON     ";
  for (int i = 0; i < 10; i++)
    data[pos++] = batt_name[i];

  // Manufacturer name (20 bytes)
  const char* manuf = "PYLONTECH          ";
  for (int i = 0; i < 20; i++)
    data[pos++] = manuf[i];

  // Software version (24 bytes)
  const char* sw_ver = "V3.3                   ";
  for (int i = 0; i < 24; i++)
    data[pos++] = sw_ver[i];

  // Number of batteries (always 1 for a single unit)
  data[pos++] = 0x01;

  // Barcode for battery 1 (16 bytes) – you may replace with a real serial
  const char* barcode = "1234567890123456";
  for (int i = 0; i < 16; i++)
    data[pos++] = barcode[i];

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_system_analog_value(uint8_t adr) {
  uint8_t data[52];  // 26 * 2 bytes
  uint16_t pos = 0;

  auto append_word = [&](uint16_t val) {
    data[pos++] = (val >> 8) & 0xFF;
    data[pos++] = val & 0xFF;
  };

  // 1. Total voltage (mV)
  uint16_t total_voltage = datalayer.battery.status.voltage_dV * 100;
  append_word(total_voltage);

  // 2. Total current (mA) – positive for charge
  int16_t total_current = datalayer.battery.status.current_dA * 100;
  append_word((uint16_t)total_current);

  // 3. SOC (%) – stored as 16‑bit with high byte zero
  uint8_t soc = datalayer.battery.status.reported_soc / 100;  // Convert from pptt to %
  append_word((uint16_t)soc);

  // 4. Average cycles – not available
  append_word(0);
  // 5. Minimum cycles
  append_word(0);
  // 6. Average SOH – assume 100%
  append_word(100);
  // 7. Max cycles
  append_word(0);

  // 8. Max cell voltage (mV)
  uint16_t max_cell_mV = datalayer.battery.status.cell_max_voltage_mV;
  append_word(max_cell_mV);

  // 9. Module with max voltage (block=1, cell index 1‑based)
  uint8_t max_cell_idx = 1;
  for (int i = 0; i < datalayer.battery.info.number_of_cells; i++) {
    if (datalayer.battery.status.cell_voltages_mV[i] == max_cell_mV) {
      max_cell_idx = i + 1;
      break;
    }
  }
  append_word((0x01 << 8) | max_cell_idx);

  // 10. Min cell voltage
  uint16_t min_cell_mV = datalayer.battery.status.cell_min_voltage_mV;
  append_word(min_cell_mV);

  // 11. Module with min voltage
  uint8_t min_cell_idx = 1;
  for (int i = 0; i < datalayer.battery.info.number_of_cells; i++) {
    if (datalayer.battery.status.cell_voltages_mV[i] == min_cell_mV) {
      min_cell_idx = i + 1;
      break;
    }
  }
  append_word((0x01 << 8) | min_cell_idx);

  // 12. Average cell temperature (K * 10)
  int16_t avg_temp_dC = (datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2;
  append_word(avg_temp_dC + 2731);

  // 13. Max cell temperature
  append_word(datalayer.battery.status.temperature_max_dC + 2731);

  // 14. Module with max temperature (dummy)
  append_word(0x0101);

  // 15. Min cell temperature
  append_word(datalayer.battery.status.temperature_min_dC + 2731);

  // 16. Module with min temperature (dummy)
  append_word(0x0101);

  // 17‑26. MOSFET & BMS temperatures – use default values
  const uint16_t default_temp = 2986;  // 25.5°C
  for (int i = 17; i <= 26; i++) {
    append_word(default_temp);  // temperature values
    if (i == 18 || i == 20 || i == 22 || i == 24 || i == 26)
      append_word(0x0101);  // module address for the following item
  }

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_system_alarm_info(uint8_t adr) {
  uint8_t data[4];
  uint16_t pos = 0;

  uint8_t alarm1 = 0, alarm2 = 0, protect1 = 0, protect2 = 0;

  /* TODO: Implement this later

    uint16_t voltage_mV = datalayer.battery.status.voltage_dV * 100;
    uint16_t max_voltage_mV = datalayer.battery.info.max_design_voltage_dV * 100;
    uint16_t min_voltage_mV = datalayer.battery.info.min_design_voltage_dV * 100;
    uint16_t max_cell_mV = datalayer.battery.info.max_cell_voltage_mV;
    uint16_t min_cell_mV = datalayer.battery.info.min_cell_voltage_mV;
    int16_t current_mA = datalayer.battery.status.current_dA * 100;
    uint16_t max_charge_mA = datalayer.battery.status.max_charge_current_dA * 100;
    uint16_t max_discharge_mA = datalayer.battery.status.max_discharge_current_dA * 100;

    // Alarm status 1
    if (voltage_mV > max_voltage_mV) alarm1 |= 0x80;          // total overvoltage
    if (voltage_mV < min_voltage_mV) alarm1 |= 0x40;          // total undervoltage
    if (datalayer.battery.status.cell_max_voltage_mV > max_cell_mV) alarm1 |= 0x20; // cell overvoltage
    if (datalayer.battery.status.cell_min_voltage_mV < min_cell_mV) alarm1 |= 0x10; // cell undervoltage
    if (datalayer.battery.status.temperature_max_dC + 2731 > charge_high_temp_limit) alarm1 |= 0x08; // high temp
    if (datalayer.battery.status.temperature_min_dC + 2731 < charge_low_temp_limit) alarm1 |= 0x04;  // low temp

    // Alarm status 2
    if (current_mA > max_charge_mA) alarm2 |= 0x40;           // charge overcurrent
    if (current_mA < -max_discharge_mA) alarm2 |= 0x20;       // discharge overcurrent
    if (datalayer.battery.status.bms_status == FAULT) alarm2 |= 0x10; // internal comm error (example)

    */

  // Protection status (simplified – you can refine as needed)
  protect1 = 0;
  protect2 = 0;

  data[pos++] = alarm1;
  data[pos++] = alarm2;
  data[pos++] = protect1;
  data[pos++] = protect2;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_get_system_charge_discharge_info(uint8_t adr) {
  uint8_t data[9];
  uint16_t pos = 0;

  data[pos++] = (charge_voltage_limit >> 8) & 0xFF;
  data[pos++] = charge_voltage_limit & 0xFF;
  data[pos++] = (discharge_voltage_limit >> 8) & 0xFF;
  data[pos++] = discharge_voltage_limit & 0xFF;
  data[pos++] = (max_charge_current >> 8) & 0xFF;
  data[pos++] = max_charge_current & 0xFF;
  data[pos++] = (max_discharge_current >> 8) & 0xFF;
  data[pos++] = max_discharge_current & 0xFF;
  data[pos++] = charge_discharge_status;

  send_response(adr, RTN_NORMAL, data, pos);
}

void PylonLV485InverterProtocol::handle_system_turn_off(uint8_t adr) {
  send_response(adr, RTN_NORMAL, nullptr, 0);
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
  if (len >= 9) {
    // Parse and update settings
    charge_voltage_limit = (data[1] << 8) | data[2];
    discharge_voltage_limit = (data[3] << 8) | data[4];
    max_charge_current = (data[5] << 8) | data[6];
    max_discharge_current = (data[7] << 8) | data[8];

    // Note: According to protocol, this command needs to be sent periodically
    // If not received for 10 seconds, battery reverts to automatic settings
    last_command_time = millis();
  }

  send_response(adr, RTN_NORMAL, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_turn_off(uint8_t adr, uint8_t command) {
  // Implement shutdown logic here
  send_response(adr, RTN_NORMAL, nullptr, 0);
}

void PylonLV485InverterProtocol::handle_get_software_version(uint8_t adr, uint8_t command) {
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

  while (Serial2.available()) {
    uint8_t byte = Serial2.read();

    if (byte == SOI) {
      incoming_message_counter = RS485_HEALTHY;
      // Start of new frame
      rx_index = 0;
      rx_buffer[rx_index++] = byte;
    } else if (byte == EOI && rx_index > 0) {
      // End of frame - process it
      rx_buffer[rx_index++] = byte;

      // Minimum frame length: SOI(1) + VER(2) + ADR(2) + CID1(2) + CID2(2) + LENGTH(4) + CHKSUM(4) + EOI(1) = 18 bytes
      if (rx_index >= 18) {
        // Parse frame
        //uint8_t ver_high = rx_buffer[1];
        //uint8_t ver_low = rx_buffer[2];
        uint8_t adr_high = rx_buffer[3];
        uint8_t adr_low = rx_buffer[4];
        uint8_t cid1_high = rx_buffer[5];
        uint8_t cid1_low = rx_buffer[6];
        uint8_t cid2_high = rx_buffer[7];
        uint8_t cid2_low = rx_buffer[8];
        uint8_t len_high_high = rx_buffer[9];
        uint8_t len_high_low = rx_buffer[10];
        uint8_t len_low_high = rx_buffer[11];
        uint8_t len_low_low = rx_buffer[12];

        uint8_t adr = ascii_to_hex(adr_high, adr_low);
        uint8_t cid1 = ascii_to_hex(cid1_high, cid1_low);
        uint8_t cid2 = ascii_to_hex(cid2_high, cid2_low);

        // Verify CID1 is battery data
        if (cid1 != CID1_BATTERY) {
          continue;
        }

        // Extract INFO data if present
        uint8_t info_data[256];
        uint16_t info_len = 0;

        // Calculate INFO length from LENGTH field
        uint16_t length_high = (ascii_to_hex(len_high_high, len_high_low) << 8);
        uint16_t length_low = ascii_to_hex(len_low_high, len_low_low);
        uint16_t length = length_high | length_low;
        uint16_t info_ascii_len = length & 0x0FFF;  // Lower 12 bits are LENID

        // Extract INFO bytes (each byte is 2 ASCII chars)
        for (int i = 0; i < info_ascii_len / 2; i++) {
          uint8_t high = rx_buffer[13 + i * 2];
          uint8_t low = rx_buffer[13 + i * 2 + 1];
          info_data[info_len++] = ascii_to_hex(high, low);
        }

        // Handle command based on CID2
        switch (cid2) {
          case 0x4F:  // Get protocol version
            handle_get_protocol_version(adr);
            break;

          case 0x51:  // Get manufacturer info
            handle_get_manufacturer_info(adr);
            break;

          case 0x42:  // Get analog value
            if (info_len >= 1) {
              handle_get_analog_value(adr, info_data[0]);
            }
            break;

          case 0x47:  // Get system parameter
            handle_get_system_parameter(adr);
            break;

          case 0x44:  // Get alarm info
            if (info_len >= 1) {
              handle_get_alarm_info(adr, info_data[0]);
            }
            break;
          case 0x60:
            handle_get_system_basic_info(adr);
            break;
          case 0x61:
            handle_get_system_analog_value(adr);
            break;
          case 0x62:
            handle_get_system_alarm_info(adr);
            break;
          case 0x63:
            handle_get_system_charge_discharge_info(adr);
            break;
          case 0x64:
            handle_system_turn_off(adr);
            break;
          case 0x92:  // Get charge/discharge management info
            if (info_len >= 1) {
              handle_get_charge_discharge_info(adr, info_data[0]);
            }
            break;

          case 0x93:  // Get serial number
            if (info_len >= 1) {
              handle_get_serial_number(adr, info_data[0]);
            }
            break;

          case 0x94:  // Set charge/discharge management info
            handle_set_charge_discharge_info(adr, info_data, info_len);
            break;

          case 0x95:  // Turn off
            if (info_len >= 1) {
              handle_turn_off(adr, info_data[0]);
            }
            break;

          case 0x96:  // Get software version
            if (info_len >= 1) {
              handle_get_software_version(adr, info_data[0]);
            }
            break;

          default:
            // Unknown command
            send_error_response(adr, RTN_CID2_INVALID);
            break;
        }
      }

      rx_index = 0;
    } else if (rx_index < RX_BUFFER_SIZE - 1) {
      // Add byte to buffer
      rx_buffer[rx_index++] = byte;
    } else {
      // Buffer overflow, reset
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

  charge_discharge_status = 0xC0;  // Bit7: Charge enable, Bit6: Discharge enable TODO, utilize this later

  if (incoming_message_counter > 0) {
    incoming_message_counter--;
  }

  if (incoming_message_counter == 0) {
    set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  } else {
    clear_event(EVENT_MODBUS_INVERTER_MISSING);
  }
}
