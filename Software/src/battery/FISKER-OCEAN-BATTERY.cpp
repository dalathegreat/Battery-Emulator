#include "FISKER-OCEAN-BATTERY.h"
#include <cstring>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table

uint8_t expected_CRC(CAN_frame* frame, uint8_t xor_out) {
  //CRC-8, Polynomial = 0x1D, Init = 0xFF, XorOut varies per message
  //We check bytes 1-7, since byte 0 is the CRC itself

  //We can get the table crc8_table_SAE_J1850_ZER0 already in the codebase
  uint8_t crc = 0xFF;  //Init

  for (uint8_t i = 1; i < 8; i++) {
    crc = crc8_table_SAE_J1850_ZER0[crc ^ frame->data.u8[i]];
  }

  crc ^= xor_out;

  return crc;  //Return the expected CRC value
}

void FiskerOceanBattery::update_values() {
  /*
  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.info.max_design_voltage_dV;

  datalayer.battery.info.min_design_voltage_dV;
  */

  datalayer.battery.status.voltage_dV = pack_voltage / 10;

  if (datalayer_extended.fiskerOcean.broadcast_soc_valid) {
    datalayer.battery.status.real_soc = datalayer_extended.fiskerOcean.broadcast_soc_percent * 100;
  }

  datalayer.battery.status.temperature_min_dC = cell_temperature_min_C * 10;

  datalayer.battery.status.temperature_max_dC = cell_temperature_max_C * 10;

  datalayer.battery.info.number_of_cells = NUM_CELLS;
}

void FiskerOceanBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  if (handle_incoming_uds_can_frame(rx_frame)) {
    datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    return;
  }

  switch (rx_frame.ID) {
    case 0x100:  //BBus 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x103:  //BBus 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x110:  //BBus 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x180:  //BBus 50ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x181:  //BBus 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x182:  //BBus 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x310:  //BBus 30ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame6 has counter low nibble, 0-F incrementing every frame
      break;
    case 0x0E9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] == expected_CRC(&rx_frame, 0x05)) {  //If CRC matches expected
        pack_voltage = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      } else {  //If CRC does not match expected, increment error counter
        datalayer.battery.status.CAN_error_counter++;
      }
      break;
    case 0x0EB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0EE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0F2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x215:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x24A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x278:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x279:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2EE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2EF:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      datalayer_extended.fiskerOcean.broadcast_soc_percent = rx_frame.data.u8[0];
      datalayer_extended.fiskerOcean.broadcast_soc_valid = rx_frame.data.u8[0] <= 100;
      break;
    case 0x2F6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x330:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x370:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x372:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cell_temperature_max_C =
          rx_frame.data.u8[4] -
          40;  //Matches with data from 0x6D0 and 0x6D1 frames, so we can use this as the max temperature
      cell_temperature_min_C =
          rx_frame.data.u8[5] -
          40;  //Matches with data from 0x6D0 and 0x6D1 frames, so we can use this as the min temperature
      break;
    case 0x3A0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x595:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5A7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x63A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x652:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6A0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6A5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6B0 ... 0x6CD: {
      uint8_t frame_offset = rx_frame.ID - CELLVOLTAGE_FRAME_START;

      for (uint8_t i = 0; i < 4; i++) {
        uint16_t raw = (rx_frame.data.u8[i * 2] << 8) | rx_frame.data.u8[i * 2 + 1];

        if (raw == 0xFFFF) {
          // Padding, no cell present in this slot (covers 6C9's 2nd half
          // and all of 6CA..6CD)
          continue;
        }

        uint8_t cell_index = (frame_offset * 4) + i;

        if (cell_index < NUM_CELLS) {
          datalayer.battery.status.cell_voltages_mV[cell_index] = raw / 10;
        }
      }
      break;
    }
    case 0x6D0:  //Temperatures (49 49 48 48 48 48 49 48 )
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //All individual temperature measurements
      break;
    case 0x6D1:  //Temperatures (48 49 48 49 49 48 48 FF )
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D2:  //Temperatures (All FF)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D3:  //Temperatures (All FF)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DD:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DF:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      //logging.printf("Received unexpected CAN frame with ID: 0x%03X\n", rx_frame.ID);
      break;
  }
}

void FiskerOceanBattery::transmit_can(unsigned long currentMillis) {
  auto& fisker = datalayer_extended.fiskerOcean;

  if (fisker.wake_transmit_active) {
    if (currentMillis - previousMillis093 >= 16) {
      previousMillis093 = currentMillis;
      transmit_ready_frame(&FISKER_READY_093, fisker.wake_093_counter, 0xE0, 0xBB);
    }
    if (currentMillis - previousMillis333 >= 48) {
      previousMillis333 = currentMillis;
      transmit_ready_frame(&FISKER_READY_333, fisker.wake_333_counter, 0xD0, 0x34);
    }
  }

  transmit_uds_can(currentMillis);
}

void FiskerOceanBattery::setup() {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_113S_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_106S_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  auto& fisker = datalayer_extended.fiskerOcean;
  for (uint8_t i = 0; i < DATALAYER_INFO_FISKER_OCEAN::DID_COUNT; i++) {
    fisker.did_results[i].did = poll_commands[i];
  }

  setup_uds(0x7E1, 0x7E9);
  set_pid_scan_list(poll_commands, DATALAYER_INFO_FISKER_OCEAN::DID_COUNT);
  reset_DTC();
}

uint16_t FiskerOceanBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  (void)value;
  for (uint8_t i = 0; i < DATALAYER_INFO_FISKER_OCEAN::DID_COUNT; i++) {
    auto& result = datalayer_extended.fiskerOcean.did_results[i];
    if (result.did != pid)
      continue;

    result.payload_length = min(length, static_cast<uint16_t>(DATALAYER_INFO_FISKER_OCEAN::MAX_DID_PAYLOAD));
    memcpy(result.payload, data, result.payload_length);
    result.last_update_ms = millis();
    result.valid = true;
    break;
  }
  return 0;
}

void FiskerOceanBattery::transmit_ready_frame(CAN_frame* frame, uint8_t& counter, uint8_t high_nibble,
                                              uint8_t xor_out) {
  frame->data.u8[1] = high_nibble | counter;
  frame->data.u8[0] = expected_CRC(frame, xor_out);
  transmit_can_frame(frame);
  counter = counter >= 0x0E ? 0 : counter + 1;
}
