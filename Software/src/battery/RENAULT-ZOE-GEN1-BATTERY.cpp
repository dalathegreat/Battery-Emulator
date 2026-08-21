#include "RENAULT-ZOE-GEN1-BATTERY.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

/* Information in this file is based of the OVMS V3 vehicle_renaultzoe.cpp component 
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe/src/vehicle_renaultzoe.cpp
The Zoe BMS apparently does not send total pack voltage, so we use the polled 96x cellvoltages summed up as total voltage
Still TODO:
- Automatically detect what vehicle and battery size we are on (Zoe 22/41 , Kangoo 33, Fluence ZE 22/36)

 Do not change code below unless you are sure what you are doing */
void RenaultZoeGen1Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer_battery->status.soh_pptt = (LB_SOH * 100);  // Increase range from 99% -> 99.00%

  datalayer_battery->status.real_soc = (uint16_t)(LB_Display_SOC * 0.25f);  // 0.0025% per bit -> pptt (0.01% units)
  // Alternative: datalayer_battery->status.real_soc = (LB_SOC * 100); // Use raw BMS Chemical SOC% (0x654)

  datalayer_battery->status.current_dA = (((int32_t)LB_Current_raw * 10) / 4) - 5000;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = LB_Discharge_allowed_W;

  datalayer_battery->status.max_charge_power_W = LB_Regen_allowed_W;

  datalayer_battery->status.temperature_min_dC = LB_Cell_minimum_temperature * 10;
  datalayer_battery->status.temperature_max_dC = LB_Cell_maximum_temperature * 10;
  // Calculate total pack voltage on packs that require this. Only calculate once all cellvoltages have been read
  if (datalayer_battery->status.cell_voltages_mV[95] > 0) {
    calculated_total_pack_voltage_mV = 0;
    for (uint8_t i = 0; i < datalayer_battery->info.number_of_cells; ++i) {
      calculated_total_pack_voltage_mV += datalayer_battery->status.cell_voltages_mV[i];
    }
  }

  if (LB_Cell_minimum_voltage < 4400) {  //Value is initialized large for some reason
    datalayer_battery->status.cell_min_voltage_mV = LB_Cell_minimum_voltage;
  }

  if (LB_Cell_maximum_voltage < 4400) {  //Value is initialized large for some reason
    datalayer_battery->status.cell_max_voltage_mV = LB_Cell_maximum_voltage;
  }

  datalayer_battery->status.voltage_dV = ((calculated_total_pack_voltage_mV / 100));  // mV to dV
}

uint16_t RenaultZoeGen1Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `data`
  // points at the raw value bytes, starting right after the echoed local
  // identifier (the response is `61 <local ID> <value...>`).
  switch (pid) {
    case GROUP1_CELLVOLTAGES_1_POLL:  // 0x41, cells 1-62
      if (length >= 124) {
        for (uint8_t cell = 0; cell < 62; cell++) {
          datalayer_battery->status.cell_voltages_mV[cell] = (data[cell * 2] << 8) | data[cell * 2 + 1];
        }
        // Cell 47 measurement is inbetween pack halves. If low, fuse blown
        if (datalayer_battery->status.cell_voltages_mV[47] < 100) {
          set_event(EVENT_BATTERY_FUSE, datalayer_battery->status.cell_voltages_mV[47], battery_index);

        } else {
          clear_event(EVENT_BATTERY_FUSE, battery_index);
        }
      }
      break;
    case GROUP2_CELLVOLTAGES_2_POLL:  // 0x42, cells 63-96
      if (length >= 68) {
        for (uint8_t cell = 0; cell < 34; cell++) {
          datalayer_battery->status.cell_voltages_mV[62 + cell] = (data[cell * 2] << 8) | data[cell * 2 + 1];
        }
      }
      break;
    case GROUP3_METRICS:  // 0x61, mileage + alltime energy
      if (length >= 17) {
        battery_mileage_in_km = (data[11] << 8) | data[12];
        kWh_from_beginning_of_battery_life = (data[15] << 8) | data[16];
      }
      break;
    case GROUP6_BALANCING:  // 0x07, one bit per cell, MSB first within each byte
      for (uint8_t cell = 0; cell < 96; cell++) {
        if ((cell >> 3) >= length) {
          break;
        }
        datalayer_battery->status.cell_balancing_status[cell] = (data[cell >> 3] >> (7 - (cell & 7))) & 0x01;
      }
      break;
    default:  //Unknown PID, ignore
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void RenaultZoeGen1Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x7BB replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  switch (rx_frame.ID) {
    case 0x155:  //10ms - Charging power, current and SOC - Confirmed sent by: Fluence ZE40, Zoe 22/41kWh, Kangoo 33kWh
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_Charging_Power_W = rx_frame.data.u8[0] * 300;
      LB_Current_raw = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2];
      LB_Display_SOC = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;

    case 0x42E:  //NOTE: Not present on 41kWh battery!
      LB_Battery_Voltage = (((((rx_frame.data.u8[3] << 8) | (rx_frame.data.u8[4])) >> 5) & 0x3ff) * 0.5);  //0.5V/bit
      LB_Average_Temperature = (((((rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6])) >> 5) & 0x7F) - 40);
      break;
    case 0x424:  //100ms - Charge limits, Temperatures, SOH - Confirmed sent by: Fluence ZE40, Zoe 22/41kWh, Kangoo 33kWh
      LB_Heartbeat = rx_frame.data.u8[6];  // Alternates between 0x55 and 0xAA every 500ms (Same as on Nissan LEAF)
      if ((LB_Heartbeat != 0x55) && (LB_Heartbeat != 0xAA)) {
        datalayer_battery->status.CAN_error_counter++;
        break;
      }
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      LB_CUV = (rx_frame.data.u8[0] & 0x03);
      LB_HVBIR = (rx_frame.data.u8[0] & 0x0C) >> 2;
      LB_HVBUV = (rx_frame.data.u8[0] & 0x30) >> 4;
      LB_EOCR = (rx_frame.data.u8[0] & 0xC0) >> 6;
      LB_HVBOC = (rx_frame.data.u8[1] & 0x03);
      LB_HVBOT = (rx_frame.data.u8[1] & 0x0C) >> 2;
      LB_HVBOV = (rx_frame.data.u8[1] & 0x30) >> 4;
      LB_COV = (rx_frame.data.u8[1] & 0xC0) >> 6;
      LB_Regen_allowed_W = rx_frame.data.u8[2] * 500;
      LB_Discharge_allowed_W = rx_frame.data.u8[3] * 500;
      LB_Cell_minimum_temperature = (rx_frame.data.u8[4] - 40);
      LB_SOH = rx_frame.data.u8[5];
      LB_Cell_maximum_temperature = (rx_frame.data.u8[7] - 40);
      break;
    case 0x425:  //100ms Cellvoltages and kWh remaining - Confirmed sent by: Fluence ZE40 & Zoe Gen1
      LB_Cell_maximum_voltage = (((((rx_frame.data.u8[4] & 0x03) << 7) | (rx_frame.data.u8[5] >> 1)) * 10) + 1000);
      LB_Cell_minimum_voltage = (((((rx_frame.data.u8[6] & 0x01) << 8) | rx_frame.data.u8[7]) * 10) + 1000);
      break;
    case 0x427:  // NOTE: Not present on 41kWh battery!
      LB_kWh_Remaining = (((((rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7])) >> 6) & 0x3ff) * 0.1);
      break;
    case 0x445:                            //100ms - Confirmed sent by: Fluence ZE40 & Zoe Gen1
      LB_Heartbeat = rx_frame.data.u8[2];  // Alternates between 0x55 and 0xAA every 500ms (Same as on Nissan LEAF)
      if ((LB_Heartbeat != 0x55) && (LB_Heartbeat != 0xAA)) {
        datalayer_battery->status.CAN_error_counter++;
        break;
      }
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x654:  //SOC
      LB_SOC = rx_frame.data.u8[3];
      break;
    default:
      break;
  }
}

void RenaultZoeGen1Battery::transmit_can(unsigned long currentMillis) {

  // Send 100ms CAN Message (the BMS only answers diagnostic requests while it
  // receives this wakeup frame)
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    transmit_can_frame(&ZOE_423);

    if ((counter_423 / 5) % 2 == 0) {  // Alternate every 5 messages between these two
      ZOE_423.data.u8[4] = 0xB2;
      ZOE_423.data.u8[6] = 0xB2;
    } else {
      ZOE_423.data.u8[4] = 0x5D;
      ZOE_423.data.u8[6] = 0x5D;
    }
    counter_423 = (counter_423 + 1) % 10;
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String RenaultZoeGen1Battery::get_uds_info_html() {
  String content;
  content.reserve(270);

  // clang-format off
  content << "Cell Under Voltage: " << (LB_CUV >= 2 ? "FAULT" : "OK") << "<br>"
             "Cell Over Voltage: " << (LB_COV >= 2 ? "FAULT" : "OK") << "<br>"
             "Pack Under Voltage: " << (LB_HVBUV >= 2 ? "FAULT" : "OK") << "<br>"
             "Pack Over Voltage: " << (LB_HVBOV >= 2 ? "FAULT" : "OK") << "<br>"
             "Pack Over Current: " << (LB_HVBOC >= 2 ? "FAULT" : "OK") << "<br>"
             "Over Temp: " << (LB_HVBOT >= 2 ? "FAULT" : "OK") << "<br>"
             "Isolation: " << (LB_HVBIR >= 2 ? "FAULT" : "OK") << "<br>"
             "End Of Charge: " << (LB_EOCR >= 2 ? "YES" : "NO") << "<br>"
             "Battery Mileage: " << battery_mileage_in_km << " km<br>"
             "Lifetime Energy: " << kWh_from_beginning_of_battery_life << " kWh<br>";
  // clang-format on

  return content;
}

void RenaultZoeGen1Battery::setup(void) {  // Performs one time setup at startup
  // UDS: send requests/flow control to 0x79B, accept replies from the BMS on 0x7BB.
  setup_uds(0x79B, 0x7BB);

  // The Zoe Gen1 BMS only speaks KWP2000-style one-byte local identifiers.
  set_pid_scan_mode(PidScanMode::OneByteLocalId);

  static const uint16_t pid_scan_list[] = {
      GROUP1_CELLVOLTAGES_1_POLL,  // Cells 1-62
      GROUP2_CELLVOLTAGES_2_POLL,  // Cells 63-96
      GROUP6_BALANCING,            // Balancing status bits
      GROUP3_METRICS,              // Mileage + alltime energy
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

static const uint16_t ZOE_STATE_OPEN_SESSION = 1;

void RenaultZoeGen1Battery::read_DTC() {
  start_sequence(ZOE_STATE_OPEN_SESSION);
}

void RenaultZoeGen1Battery::on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) {
  if (state == ZOE_STATE_OPEN_SESSION) {
    send_sequence_message(ZOE_STATE_OPEN_SESSION + 10, SID::DiagnosticSessionControl, (const uint8_t*)"\xC0", 1, 20, 2);
  } else if (state == ZOE_STATE_OPEN_SESSION + 10 && sid == UDS_RESPONSE_SID_OF(SID::DiagnosticSessionControl)) {
    // Session 0xC0 granted! Transmit UDS ReadDTCInformation with status mask 0x09 (Active/Confirmed DTCs)
    send_sequence_message(UDS_STATE_READ_DTC, SID::ReadDTCInformation, (const uint8_t*)"\x02\x09", 2, 20, 2);
  }
}
