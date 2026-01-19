#include "EMUS-BMS.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void EmusBms::update_values() {
  // SOC from EMUS (0-100). real_soc is kept for diagnostics; reported_soc is what we want the inverter to use.
  datalayer_battery->status.real_soc = (SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer_battery->status.soh_pptt = (SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer_battery->status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)

  datalayer_battery->status.current_dA = current_dA;  //value is *10 (150 = 15.0) , invert the sign

  datalayer_battery->status.max_charge_power_W = (max_charge_current * (voltage_dV / 10));

  datalayer_battery->status.max_discharge_power_W = (-max_discharge_current * (voltage_dV / 10));

  // Option A: Use EMUS estimated remaining charge (Ah) to compute remaining Wh, then derive reported SOC.
  // This keeps the inverter-facing (total, remaining, SOC) internally consistent.
  const uint32_t total_Wh = datalayer_battery->info.total_capacity_Wh;
  const unsigned long now_ms = millis();
  const bool est_charge_fresh = est_charge_valid && ((now_ms - est_charge_last_ms) <= EST_CHARGE_TIMEOUT_MS);

  if (est_charge_fresh && (voltage_dV > 10) && (total_Wh > 0)) {
    // remWh = (est_charge_0p1Ah/10) * (voltage_dV/10) = est_charge_0p1Ah * voltage_dV / 100
    uint32_t remWh = (uint32_t)((uint64_t)est_charge_0p1Ah * (uint64_t)voltage_dV / 100ULL);
    if (remWh > total_Wh) {
      remWh = total_Wh;  // Clamp in case EMUS estimate exceeds configured total
    }
    datalayer_battery->status.remaining_capacity_Wh = remWh;

    uint32_t soc_x100 = (uint32_t)((uint64_t)remWh * 10000ULL / (uint64_t)total_Wh);
    if (soc_x100 > 10000)
      soc_x100 = 10000;
    datalayer_battery->status.reported_soc = soc_x100;
  } else {
    // Fallback:
    //  - If total capacity is known, derive remaining from EMUS SOC.
    //  - If total capacity is unknown, keep inverter SOC aligned with EMUS SOC and report 0Wh remaining.
    if (total_Wh > 0) {
      datalayer_battery->status.remaining_capacity_Wh =
          (uint32_t)((static_cast<double>(datalayer_battery->status.real_soc) / 10000.0) * (double)total_Wh);
    } else {
      datalayer_battery->status.remaining_capacity_Wh = 0;
    }
    datalayer_battery->status.reported_soc = datalayer_battery->status.real_soc;
  }

  // Update cell count if we've received individual cell data
  if (actual_cell_count > 0) {
    datalayer_battery->info.number_of_cells = actual_cell_count;
  }

  // Use Pylon protocol min/max for alarms (more stable than individual cell data)
  // Individual cell voltages from 0x10B5 frames are still available in cell_voltages_mV[] for display
  datalayer_battery->status.cell_max_voltage_mV = cellvoltage_max_mV;
  datalayer_battery->status.cell_min_voltage_mV = cellvoltage_min_mV;

  // Also populate first two cells for systems that only check those
  if (actual_cell_count == 0) {
    datalayer_battery->status.cell_voltages_mV[0] = cellvoltage_max_mV;
    datalayer_battery->status.cell_voltages_mV[1] = cellvoltage_min_mV;
  }

  datalayer_battery->status.temperature_min_dC = celltemperature_min_dC;

  datalayer_battery->status.temperature_max_dC = celltemperature_max_dC;

  datalayer_battery->info.max_design_voltage_dV = charge_cutoff_voltage;

  datalayer_battery->info.min_design_voltage_dV = discharge_cutoff_voltage;
}

void EmusBms::handle_incoming_can_frame(CAN_frame rx_frame) {
  // Handle EMUS extended ID frames first
  if (rx_frame.ID == 0x10B50000) {
    // EMUS configuration frame containing cell count
    // Byte 0-1: Unknown (00 05)
    // Byte 2-3: Unknown (00 03)
    // Byte 4-5: Unknown (00 01)
    // Byte 6-7: Number of cells (00 77 = 0x77 = 119 decimal)
    uint8_t cell_count = rx_frame.data.u8[7];  // Just use byte 7 (0x77 = 119)
    // Only update if we got a valid non-zero count
    if (cell_count > 0 && cell_count <= MAX_CELLS) {
      actual_cell_count = cell_count;
      datalayer_battery->info.number_of_cells = actual_cell_count;
    }
    return;
  }

  if (rx_frame.ID == EMUS_SOC_PARAMS_ID) {
    // EMUS Base+0x05: State of Charge parameters
    // Data0-1: current (0.1A, signed)
    // Data2-3: estimated remaining charge (0.1Ah)
    // Data6: SOC (%)
    // Data7: SOH (%)
    current_dA = (int16_t)((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
    est_charge_0p1Ah = (uint16_t)((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
    est_charge_valid = (est_charge_0p1Ah != 0xFFFF) && (est_charge_0p1Ah > 0);
    est_charge_last_ms = millis();
    SOC = rx_frame.data.u8[6];
    SOH = rx_frame.data.u8[7];
    return;
  }

  if (rx_frame.ID == EMUS_ENERGY_PARAMS_ID) {
    // EMUS Base+0x06: Energy parameters
    // Data2-3: estimated energy left (10Wh)
    est_energy_10Wh = (uint16_t)((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
    est_energy_valid = (est_energy_10Wh != 0xFFFF) && (est_energy_10Wh > 0);
    return;
  }

  switch (rx_frame.ID) {
    case 0x7310:
    case 0x7311:
      ensemble_info_ack = true;
      // This message contains software/hardware version info. No interest to us
      break;
    case 0x7320:
    case 0x7321:
      ensemble_info_ack = true;
      battery_module_quantity = rx_frame.data.u8[0];
      battery_modules_in_series = rx_frame.data.u8[2];
      cell_quantity_in_module = rx_frame.data.u8[3];
      voltage_level = rx_frame.data.u8[4];
      ah_number = rx_frame.data.u8[6];
      break;
    case 0x4210:
    case 0x4211:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 30000;
      SOC = rx_frame.data.u8[6];
      SOH = rx_frame.data.u8[7];
      break;
    case 0x4220:
    case 0x4221:
      charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      discharge_cutoff_voltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      max_charge_current = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * 0.1) - 3000;
      max_discharge_current = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) * 0.1) - 3000;
      break;
    case 0x4230:
    case 0x4231:
      cellvoltage_max_mV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      cellvoltage_min_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      break;
    case 0x4240:
    case 0x4241:
      celltemperature_max_dC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 1000;
      celltemperature_min_dC = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 1000;
      break;
    case 0x4250:
    case 0x4251:
      //Byte0 Basic Status
      //Byte1-2 Cycle Period
      //Byte3 Error
      //Byte4-5 Alarm
      //Byte6-7 Protection
      break;
    case 0x4260:
    case 0x4261:
      //Byte0-1 Module Max Voltage
      //Byte2-3 Module Min Voltage
      //Byte4-5 Module Max. Voltage Number
      //Byte6-7 Module Min. Voltage Number
      break;
    case 0x4270:
    case 0x4271:
      //Byte0-1 Module Max. Temperature
      //Byte2-3 Module Min. Temperature
      //Byte4-5 Module Max. Temperature Number
      //Byte6-7 Module Min. Temperature Number
      break;
    case 0x4280:
    case 0x4281:
      charge_forbidden = rx_frame.data.u8[0];
      discharge_forbidden = rx_frame.data.u8[1];
      break;
    case 0x4290:
    case 0x4291:
      break;
    default:
      // Handle EMUS individual cell voltage messages (0x10B50100-0x10B5011F)
      // Each message contains 8 cells (1 byte per cell)
      if (rx_frame.ID >= CELL_VOLTAGE_BASE_ID && rx_frame.ID < (CELL_VOLTAGE_BASE_ID + 32)) {
        uint8_t group = rx_frame.ID - CELL_VOLTAGE_BASE_ID;
        uint8_t cell_start = group * 8;  // 8 cells per message

        for (uint8_t i = 0; i < 8; i++) {
          uint8_t cell_index = cell_start + i;
          // Only process cells up to the actual cell count (if known)
          if (cell_index < MAX_CELLS && (actual_cell_count == 0 || cell_index < actual_cell_count)) {
            // Cell voltage: 2000mV base + (byte value × 10mV)
            // e.g., 0x7B (123) = 2000 + 123 × 10 = 3230mV
            uint16_t cell_voltage = 2000 + (rx_frame.data.u8[i] * 10);
            // Only update if voltage is in valid LiFePO4 range (2500-4200mV)
            if (cell_voltage >= 2500 && cell_voltage <= 4200) {
              uint16_t current_voltage = datalayer_battery->status.cell_voltages_mV[cell_index];
              // Reject sudden large changes (>1000mV) as likely data corruption
              // Using 1000mV threshold since EMUS updates every 5-6 seconds
              if (current_voltage == 0 || abs((int)cell_voltage - (int)current_voltage) <= 1000) {
                datalayer_battery->status.cell_voltages_mV[cell_index] = cell_voltage;
              }
            }
          }
        }
      }
      // Handle EMUS individual cell balancing status messages (0x10B50300-0x10B5031F)
      // Each message contains 8 cells (1 byte per cell)
      else if (rx_frame.ID >= CELL_BALANCING_BASE_ID && rx_frame.ID < (CELL_BALANCING_BASE_ID + 32)) {
        uint8_t group = rx_frame.ID - CELL_BALANCING_BASE_ID;
        uint8_t cell_start = group * 8;  // 8 cells per message

        for (uint8_t i = 0; i < 8; i++) {
          uint8_t cell_index = cell_start + i;
          if (cell_index < MAX_CELLS && (actual_cell_count == 0 || cell_index < actual_cell_count)) {
            // Balancing status: non-zero value = balancing active
            datalayer_battery->status.cell_balancing_status[cell_index] = (rx_frame.data.u8[i] > 0);
          }
        }
      }
      break;
  }
}

void EmusBms::transmit_can(unsigned long currentMillis) {
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    transmit_can_frame(&PYLON_3010);  // Heartbeat
    transmit_can_frame(&PYLON_4200);  // Ensemble OR System equipment info, depends on frame0
    transmit_can_frame(&PYLON_8200);  // Control device quit sleep status
    transmit_can_frame(&PYLON_8210);  // Charge command

    if (ensemble_info_ack) {
      PYLON_4200.data.u8[0] = 0x00;  //Request system equipment info
    }
  }

  // EMUS BMS auto-broadcasts cell data - no polling needed
}

void EmusBms::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "EMUS BMS (Pylon 250k)", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 2;  // Will be updated dynamically based on received data
  datalayer_battery->info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer_battery->info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer_battery->info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer_battery->info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // Initialize all cell voltages to a safe mid-range value to prevent false low voltage alarms
  for (uint16_t i = 0; i < MAX_CELLS; i++) {
    datalayer_battery->status.cell_voltages_mV[i] = 3300;  // Safe default value
  }

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
