#include "FISKER-OCEAN-BATTERY.h"
#include <cstring>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  // For the CRC table
#include "FISKER-OCEAN-DID-DECODE.h"

// CRC-8, polynomial 0x1D, init 0xFF, with a per-message XorOut. Byte 0 holds the
// CRC itself, so it is computed over bytes 1-7.
static uint8_t expected_CRC(CAN_frame* frame, uint8_t xor_out) {
  uint8_t crc = 0xFF;  // Init
  for (uint8_t i = 1; i < 8; i++) {
    crc = crc8_table_SAE_J1850_ZER0[crc ^ frame->data.u8[i]];
  }
  crc ^= xor_out;
  return crc;
}

void FiskerOceanBattery::update_values() {
  datalayer_battery->status.voltage_dV = pack_voltage / 10;

  // BMS broadcast power limits are not decoded yet, so follow the operator-
  // configured limits, held under a conservative development current ceiling.
  const uint32_t test_power_limit_W =
      (static_cast<uint32_t>(TEST_CURRENT_LIMIT_DA) * datalayer_battery->status.voltage_dV) / 100;
  datalayer_battery->status.max_charge_power_W =
      min(datalayer_battery->status.override_charge_power_W, test_power_limit_W);
  datalayer_battery->status.max_discharge_power_W =
      min(datalayer_battery->status.override_discharge_power_W, test_power_limit_W);

  if (broadcast_soc_valid) {
    datalayer_battery->status.real_soc = broadcast_soc_percent * 100;
  }

  datalayer_battery->status.temperature_min_dC = cell_temperature_min_C * 10;
  datalayer_battery->status.temperature_max_dC = cell_temperature_max_C * 10;
  datalayer_battery->info.number_of_cells = NUM_CELLS;
}

void FiskerOceanBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  if (handle_incoming_uds_can_frame(rx_frame)) {
    datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    return;  // Handled by the UDS layer (DTC responses).
  }

  // Only frames we recognise as coming from the pack renew the alive timeout;
  // an unrelated frame must not keep a silent BMS looking alive.
  switch (rx_frame.ID) {
    case 0x0E9:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] == expected_CRC(&rx_frame, 0x05)) {
        const int16_t fisker_current_raw =
            static_cast<int16_t>((static_cast<uint16_t>(rx_frame.data.u8[4]) << 8) | rx_frame.data.u8[5]);
        // Fisker uses 0.05 A/bit with positive = discharge. Battery Emulator uses
        // deciamps with positive = charge, so invert and halve the raw value.
        datalayer_battery->status.current_dA = -(fisker_current_raw / 2);
        pack_voltage = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      } else {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x0F2:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 6) {
        const uint16_t cell_max_mV = ((static_cast<uint16_t>(rx_frame.data.u8[0]) << 8) | rx_frame.data.u8[1]) / 10;
        const uint16_t cell_min_mV = ((static_cast<uint16_t>(rx_frame.data.u8[2]) << 8) | rx_frame.data.u8[3]) / 10;
        if (cell_max_mV > 0 && cell_min_mV > 0) {
          datalayer_battery->status.cell_max_voltage_mV = cell_max_mV;
          datalayer_battery->status.cell_min_voltage_mV = cell_min_mV;
        }
      }
      break;
    case 0x2F5:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      broadcast_soc_percent = rx_frame.data.u8[0];
      broadcast_soc_valid = rx_frame.data.u8[0] <= 100;
      break;
    case 0x372:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      // Byte 4/5 match the per-sensor readings in 0x6D0/0x6D1 (offset -50).
      cell_temperature_max_C = rx_frame.data.u8[4] - 50;
      cell_temperature_min_C = rx_frame.data.u8[5] - 50;
      break;
    case 0x6B0 ... 0x6CD: {
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      const uint8_t frame_offset = rx_frame.ID - CELLVOLTAGE_FRAME_START;
      for (uint8_t i = 0; i < 4; i++) {
        const uint16_t raw = (rx_frame.data.u8[i * 2] << 8) | rx_frame.data.u8[i * 2 + 1];
        if (raw == 0 || raw == 0xFFFF) {
          continue;  // Padding slot, no cell present (e.g. 2nd half of 0x6C9).
        }
        const uint8_t cell_index = (frame_offset * 4) + i;
        if (cell_index < NUM_CELLS) {
          datalayer_battery->status.cell_voltages_mV[cell_index] = raw / 10;
        }
      }

      uint16_t cell_min_mV = UINT16_MAX;
      uint16_t cell_max_mV = 0;
      for (uint8_t i = 0; i < NUM_CELLS; i++) {
        const uint16_t cell_mV = datalayer_battery->status.cell_voltages_mV[i];
        if (cell_mV == 0) {
          continue;
        }
        cell_min_mV = min(cell_min_mV, cell_mV);
        cell_max_mV = max(cell_max_mV, cell_mV);
      }
      if (cell_max_mV > 0) {
        datalayer_battery->status.cell_min_voltage_mV = cell_min_mV;
        datalayer_battery->status.cell_max_voltage_mV = cell_max_mV;
      }
      break;
    }
    default:
      break;
  }
}

void FiskerOceanBattery::transmit_can(unsigned long currentMillis) {
  if (wake_transmit_active) {
    if (currentMillis - previousMillis093 >= INTERVAL_20_MS) {
      previousMillis093 = currentMillis;
      transmit_ready_frame(&FISKER_READY_093, wake_093_counter, 0xE0, 0xBB);
    }
    if (currentMillis - previousMillis333 >= INTERVAL_50_MS) {
      previousMillis333 = currentMillis;
      transmit_ready_frame(&FISKER_READY_333, wake_333_counter, 0xD0, 0x34);
    }
    if (currentMillis - previousMillis6A5 >= INTERVAL_1_S) {
      previousMillis6A5 = currentMillis;
      transmit_can_frame(&FISKER_MODE_6A5);
    }
  }

  transmit_uds_can(currentMillis);  // Drives DTC read/clear requests.
}

void FiskerOceanBattery::setup() {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer_battery->info.number_of_cells = NUM_CELLS;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_113S_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_106S_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // The BMS owns its own contactors; the emulator only has to permit closing.
  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }

  // Enable UDS for DTC read/clear on the BMS diagnostic addresses, then scan the
  // diagnostic identifier set; responses are decoded on the battery page.
  setup_uds(0x7E1, 0x7E9);
  for (uint16_t i = 0; i < NUM_DIDS; i++) {
    did_results[i].did = poll_commands[i];
  }
  set_pid_scan_list(poll_commands, NUM_DIDS);
}

const uint16_t FiskerOceanBattery::poll_commands[FiskerOceanBattery::NUM_DIDS] = {
    0x2003, 0x2004, 0x2005, 0x2008, 0x2009, 0x2011, 0x2016, 0x2019, 0x2024, 0x2026, 0x2027, 0x2031,
    0x2032, 0x2033, 0x2034, 0x2038, 0x2039, 0x2040, 0x2041, 0x2042, 0x2043, 0x2047, 0x2048, 0x2049,
    0x2050, 0x2053, 0x2054, 0x2055, 0x2056, 0x2057, 0x2058, 0x2059, 0x2060, 0x2061, 0x2062, 0x2063,
    0x2064, 0x2069, 0x2070, 0x2078, 0x2079, 0x2080, 0x2081, 0x2089, 0x2090, 0x2091, 0x2092, 0x2093,
    0x2094, 0x2107, 0x2108, 0x2109, 0x2117, 0x2130, 0x2133, 0x2134, 0x2136, 0x2137, 0x2138, 0x2143,
    0x2144, 0x2145, 0xEFF6, 0xEFF7, 0xEFF8, 0xEFF9, 0xEFFE, 0xF040, 0xF055, 0xF060, 0xF184, 0xF190};

uint16_t FiskerOceanBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  (void)value;
  for (uint16_t i = 0; i < NUM_DIDS; i++) {
    if (did_results[i].did != pid) {
      continue;
    }
    did_results[i].payload_length = length < MAX_DID_PAYLOAD ? static_cast<uint8_t>(length) : MAX_DID_PAYLOAD;
    memcpy(did_results[i].payload, data, did_results[i].payload_length);
    did_results[i].last_update_ms = millis();
    did_results[i].valid = true;
    break;
  }
  return 0;
}

String FiskerOceanBattery::get_uds_info_html() {
  String content =
      "<h4>Fisker diagnostic identifiers (UDS 0x22)</h4>"
      "<div style='overflow-x:auto'><table style='width:100%'><thead><tr>"
      "<th style='text-align:left'>Identifier</th><th>Decoded</th><th>Response</th></tr></thead><tbody>";
  char buf[8];
  for (uint16_t i = 0; i < NUM_DIDS; i++) {
    const DidResult& r = did_results[i];
    snprintf(buf, sizeof(buf), "0x%04X", poll_commands[i]);
    content += "<tr><td style='text-align:left'><strong>" + String(FiskerDidDecode::name(poll_commands[i])) +
               "</strong><br><code>" + buf + "</code></td><td>";
    content += r.valid ? FiskerDidDecode::decode(r.did, r.payload, r.payload_length) : String("&ndash;");
    content += "</td><td><code>";
    if (r.valid) {
      char b[4];
      content += "62 ";
      snprintf(b, sizeof(b), "%02X ", r.did >> 8);
      content += b;
      snprintf(b, sizeof(b), "%02X", r.did & 0xFF);
      content += b;
      for (uint8_t k = 0; k < r.payload_length; k++) {
        snprintf(b, sizeof(b), " %02X", r.payload[k]);
        content += b;
      }
      content += "</code><br><small>" + String((millis() - r.last_update_ms) / 1000) + "s ago</small>";
    } else {
      content += "no response</code>";
    }
    content += "</td></tr>";
  }
  content += "</tbody></table></div>";
  return content;
}

void FiskerOceanBattery::transmit_ready_frame(CAN_frame* frame, uint8_t& counter, uint8_t high_nibble,
                                              uint8_t xor_out) {
  frame->data.u8[1] = high_nibble | counter;
  frame->data.u8[0] = expected_CRC(frame, xor_out);
  transmit_can_frame(frame);
  counter = counter >= 0x0E ? 0 : counter + 1;
}
