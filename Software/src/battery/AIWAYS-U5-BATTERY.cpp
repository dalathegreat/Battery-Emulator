#include "AIWAYS-U5-BATTERY.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"

//DALA: Who made this comment? AI or an actual user? NOTE: Most if the code is completely untested and comes from forum posts, se
/* Aiways U5 pack — see AIWAYS-U5-BATTERY.h. Sequence: send all VCU frames with HV off, BMS state 2; request HV on,
   BMS precharges (+0.2 s), closes main- (+2.0 s) and main+ (+2.5 s, needs 0x130 within 20 V of the pack) -> state 4.
   HV request is dropped on a system fault, when the inverter does not allow contactor closing, or when the BMS asks
   for HV off. A BMS fault level >= 3 latches until the pack loses 12 V. */

uint8_t AiwaysU5Battery::crc8_j1850(const uint8_t* data, uint8_t len) {  // CRC-8 SAE-J1850, init 0xFF, xorout 0xFF
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x1D) : (uint8_t)(crc << 1);
    }
  }
  return crc ^ 0xFF;
}

void AiwaysU5Battery::protect(CAN_frame* frame, uint8_t* counter) {  // E2E: byte 6 low nibble counter, byte 7 CRC
  frame->data.u8[6] = (frame->data.u8[6] & 0xF0) | (*counter & 0x0F);
  frame->data.u8[7] = crc8_j1850(frame->data.u8, 7);
  *counter = (*counter + 1) & 0x0F;
}

uint32_t AiwaysU5Battery::motorola(const uint8_t* d, int start, int len) {  // DBC big-endian signal, start = MSB
  uint32_t value = 0;
  int pos = start;
  for (int i = 0; i < len; i++) {
    value = (value << 1) | ((d[pos / 8] >> (pos % 8)) & 1);
    pos = (pos % 8 == 0) ? pos + 15 : pos - 1;
  }
  return value;
}

void AiwaysU5Battery::update_values() {
  datalayer_battery->status.voltage_dV = pack_voltage_dV;
  datalayer_battery->status.current_dA = CURRENT_SIGN * current_dA;
  datalayer_battery->status.real_soc = soc_percent * 100;
  datalayer_battery->status.soh_pptt = soh_percent * 100;
  datalayer_battery->status.cell_max_voltage_mV = cell_max_mV;
  datalayer_battery->status.cell_min_voltage_mV = cell_min_mV;
  for (int i = 0; i < NUM_CELLS; i++) {
    datalayer_battery->status.cell_voltages_mV[i] = cell_mV[i];
  }
  if (temps_seen) {
    int16_t tmax = temp_C[0], tmin = temp_C[0];
    for (int i = 1; i < NUM_TEMPS; i++) {
      if (temp_C[i] > tmax)
        tmax = temp_C[i];
      if (temp_C[i] < tmin)
        tmin = temp_C[i];
    }
    datalayer_battery->status.temperature_max_dC = tmax * 10;
    datalayer_battery->status.temperature_min_dC = tmin * 10;
  }
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  // the BMS's own continuous limits (0x2CD); 0 until the frame has been seen
  datalayer_battery->status.max_charge_power_W = limits_seen ? cont_charge_power * POWER_LIMIT_W_PER_BIT : 0;
  datalayer_battery->status.max_discharge_power_W = limits_seen ? cont_discharge_power * POWER_LIMIT_W_PER_BIT : 0;

  if (state == STATE_OFF_FAULT || fault_level >= 3) {
    datalayer_battery->status.real_bms_status = BMS_FAULT;
  } else if (state == STATE_HV_ON) {
    datalayer_battery->status.real_bms_status = BMS_ACTIVE;
  } else if (state != 0) {
    datalayer_battery->status.real_bms_status = BMS_STANDBY;
  }
  datalayer.system.status.battery_allows_contactor_closing = (state == STATE_HV_ON);
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String AiwaysU5Battery::get_uds_info_html() {
  String content;
  content.reserve(600);

  // clang-format off
  content << "<h4>882D: " << pid_cellvoltage << " mV</h4>";
  // clang-format on

  return content;
}

void AiwaysU5Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x6B4 PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }
  const uint8_t* d = rx_frame.data.u8;
  switch (rx_frame.ID) {
    case 0x2C8:  // 100 ms: state, fault level, SoC, SoH
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      state = motorola(d, 7, 4);
      fault_level = motorola(d, 23, 3);
      soc_percent = motorola(d, 15, 7);
      soh_percent = motorola(d, 30, 7);
      break;
    case 0x2CA:  // 100 ms: pack voltage, HV-down request
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      pack_voltage_dV = motorola(d, 7, 13);
      hv_down_request = motorola(d, 44, 1);
      break;
    case 0x09D:  // 10 ms: pack current, 14 bit, 0.1 A/bit, offset -700 A (= raw - 7000 in dA)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      current_dA = (int16_t)motorola(d, 39, 14) - 7000;
      break;
    case 0x2CD:  // 100 ms: the BMS's power limits (raw, see POWER_LIMIT_W_PER_BIT)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      peak_discharge_power = motorola(d, 7, 13);
      peak_charge_power = motorola(d, 10, 12);
      cont_discharge_power = motorola(d, 30, 13);
      cont_charge_power = motorola(d, 33, 12);
      limits_seen = true;
      break;
    case 0x2C3:  // 100 ms: max / min cell voltage, 1 mV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cell_max_mV = motorola(d, 20, 13);
      cell_min_mV = motorola(d, 52, 13);
      break;
    case 0x3E0: {  // 20 ms: cell voltages, page 0-23, 4 cells per page, 1 mV
      uint8_t page = d[0];
      if (page < NUM_CELLS / 4) {
        const int starts[4] = {15, 18, 37, 55};
        for (int i = 0; i < 4; i++) {
          cell_mV[page * 4 + i] = motorola(d, starts[i], 13);
        }
      }
      break;
    }
    case 0x3E1: {  // 100 ms: temperatures, page 0-3, 7 per page, degC = raw - 40
      uint8_t page = d[0];
      for (int i = 0; i < 7; i++) {
        int idx = page * 7 + i;
        if (idx < NUM_TEMPS) {
          temp_C[idx] = (int16_t)d[1 + i] - 40;
          temps_seen = true;
        }
      }
      break;
    }
    case 0x7E1:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

uint16_t AiwaysU5Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_PACK_VOLTAGE:
      pid_pack_voltage = (uint16_t)value;
      break;
    case PID_PACK_CURRENT:
      pid_pack_current = (int16_t)value;  //TODO; correct scaling? 0.1A steps? 1A steps?
      break;
    case PID_CELL_VOLTAGE:
      pid_cellvoltage = (uint16_t)value;  //TODO; correct scaling? 0.1mV steps? 1mV steps?
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void AiwaysU5Battery::transmit_can(unsigned long currentMillis) {

  // Send periodic CAN Messages simulating the car still being attached

  // Send 10ms messages
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    request_on = datalayer.system.status.system_status != FAULT                // fault: open contactors
                 && datalayer.system.status.inverter_allows_contactor_closing  // inverter wants them closed
                 && !hv_down_request                                           // the BMS asks for HV off
                 && fault_level < 3;
    U5_2A3.data.u8[1] = (U5_2A3.data.u8[1] & ~0x60) | (request_on ? 0x20 : 0x40);

    // 0x130: DC-link voltage at the load. Echo of the pack voltage; the BMS still checks its own link measurement.
    uint16_t volts = pack_voltage_dV / 10;
    if (volts > 1023)
      volts = 1023;
    U5_130.data.u8[0] = volts >> 2;
    U5_130.data.u8[1] = (volts & 0x03) << 6;

    protect(&U5_0B5, &counter_0B5);
    protect(&U5_2A3, &counter_2A3);
    transmit_can_frame(&U5_0B5);
    transmit_can_frame(&U5_2A3);
    if (pack_voltage_dV > 0) {
      transmit_can_frame(&U5_130);
    }
  }

  // Send 100ms messages
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    protect(&U5_2A5, &counter_2A5);
    protect(&U5_2A7, &counter_2A7);
    transmit_can_frame(&U5_2A5);
    transmit_can_frame(&U5_2A7);
    transmit_can_frame(&U5_2AD);
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);
}

void AiwaysU5Battery::setup(void) {  // Performs one time setup at startup

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 100;  //TODO set
  datalayer_battery->info.total_capacity_Wh = 63000;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  // UDS: send requests to 0x7E1, accept replies from the BMS on ???.
  setup_uds(0x7E1, 0);
  static const uint16_t pid_scan_list[] = {
      PID_PACK_VOLTAGE,
      PID_PACK_CURRENT,
      PID_CELL_VOLTAGE,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
