#include "FISKER-OCEAN-BATTERY.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  // For the CRC table

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
  datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;

  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;  // Handled by the UDS layer (DTC responses).
  }

  switch (rx_frame.ID) {
    case 0x0E9:
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
      broadcast_soc_percent = rx_frame.data.u8[0];
      broadcast_soc_valid = rx_frame.data.u8[0] <= 100;
      break;
    case 0x372:
      // Byte 4/5 match the per-sensor readings in 0x6D0/0x6D1 (offset -50).
      cell_temperature_max_C = rx_frame.data.u8[4] - 50;
      cell_temperature_min_C = rx_frame.data.u8[5] - 50;
      break;
    case 0x6B0 ... 0x6CD: {
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

  // Enable UDS for DTC read/clear on the BMS diagnostic addresses.
  setup_uds(0x7E1, 0x7E9);
}

void FiskerOceanBattery::transmit_ready_frame(CAN_frame* frame, uint8_t& counter, uint8_t high_nibble,
                                              uint8_t xor_out) {
  frame->data.u8[1] = high_nibble | counter;
  frame->data.u8[0] = expected_CRC(frame, xor_out);
  transmit_can_frame(frame);
  counter = counter >= 0x0E ? 0 : counter + 1;
}
