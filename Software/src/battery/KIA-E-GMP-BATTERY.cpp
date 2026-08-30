#include "KIA-E-GMP-BATTERY.h"
#include <Arduino.h>
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "../system_settings.h"

// Function to estimate SOC based on cell voltage
uint16_t KiaEGmpBattery::estimateSOCFromCell(uint16_t cellVoltage) {
  if (cellVoltage >= voltage[0]) {
    return SOC[0];
  }
  if (cellVoltage <= voltage[numPoints - 1]) {
    return SOC[numPoints - 1];
  }

  for (int i = 1; i < numPoints; ++i) {
    if (cellVoltage >= voltage[i]) {
      // Cast to float for proper division
      float t = (float)(cellVoltage - voltage[i]) / (float)(voltage[i - 1] - voltage[i]);

      // Calculate interpolated SOC value
      uint16_t socDiff = SOC[i - 1] - SOC[i];
      uint16_t interpolatedValue = SOC[i] + (uint16_t)(t * socDiff);

      return interpolatedValue;
    }
  }
  return 0;  // Default return for safety, should never reach here
}

// Simplified version of the pack-based SOC estimation with compensation
uint16_t KiaEGmpBattery::estimateSOC(uint16_t packVoltage, uint16_t cellCount, int16_t currentAmps) {
  // If cell count is still the default 192 but we haven't confirmed it yet
  if (!set_voltage_limits && cellCount == 192) {
    // Fall back to BMS-reported SOC while cell count is uncertain
    return (SOC_Display * 10);
  }

  if (cellCount == 0)
    return 0;

  // Convert pack voltage (decivolts) to millivolts
  uint32_t packVoltageMv = packVoltage * 100;

  // Apply internal resistance compensation
  // Current is in deciamps (-150 = -15.0A, 150 = 15.0A)
  // Resistance is in milliohms
  int32_t voltageDrop = (currentAmps * PACK_INTERNAL_RESISTANCE_MOHM) / 10;

  // Compensate the pack voltage (add the voltage drop)
  uint32_t compensatedPackVoltageMv = packVoltageMv + voltageDrop;

  // Calculate average cell voltage in millivolts
  uint16_t avgCellVoltage = compensatedPackVoltageMv / cellCount;

  // Use the cell voltage lookup table to estimate SOC
  return estimateSOCFromCell(avgCellVoltage);
}

// Fix: Change parameter types to uint16_t to match SOC values
uint16_t KiaEGmpBattery::selectSOC(uint16_t SOC_low, uint16_t SOC_high) {
  if (SOC_low == 0 || SOC_high == 0) {
    return 0;  // If either value is 0, return 0
  }
  if (SOC_low == 10000 || SOC_high == 10000) {
    return 10000;  // If either value is 100%, return 100%
  }
  return (SOC_low < SOC_high) ? SOC_low : SOC_high;  // Otherwise, return the lowest value
}

void KiaEGmpBattery::set_cell_voltages(CAN_frame rx_frame, int start, int length, int startCell) {
  for (size_t i = 0; i < length; i++) {
    if ((rx_frame.data.u8[start + i] * 20) > 2600) {
      datalayer.battery.status.cell_voltages_mV[startCell + i] = (rx_frame.data.u8[start + i] * 20);
    }
  }
}

void KiaEGmpBattery::set_voltage_minmax_limits() {

  uint8_t valid_cell_count = 0;
  for (int i = 0; i < MAX_AMOUNT_CELLS; ++i) {
    if (datalayer.battery.status.cell_voltages_mV[i] > 0) {
      ++valid_cell_count;
    }
  }
  if (valid_cell_count == 144) {
    datalayer.battery.info.number_of_cells = valid_cell_count;
    datalayer.battery.info.max_design_voltage_dV = 6048;
    datalayer.battery.info.min_design_voltage_dV = 4320;
  } else if (valid_cell_count == 180) {
    datalayer.battery.info.number_of_cells = valid_cell_count;
    datalayer.battery.info.max_design_voltage_dV = 7560;
    datalayer.battery.info.min_design_voltage_dV = 5400;
  } else if (valid_cell_count == 192) {
    datalayer.battery.info.number_of_cells = valid_cell_count;
    datalayer.battery.info.max_design_voltage_dV = 8064;
    datalayer.battery.info.min_design_voltage_dV = 5760;
  } else {
    // We are still starting up? Not all cells available.
    set_voltage_limits = false;
  }
}

uint8_t KiaEGmpBattery::calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table_SAE_J1850_ZER0[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

void KiaEGmpBattery::update_values() {

  if (user_selected_use_estimated_SOC) {
    // Use the simplified pack-based SOC estimation with proper compensation
    datalayer.battery.status.real_soc =
        estimateSOC(batteryVoltage, datalayer.battery.info.number_of_cells, batteryAmps);

    // For comparison or fallback, we can still calculate from min/max cell voltages
    SOC_estimated_lowest = estimateSOCFromCell(CellVoltMin_mV);
    SOC_estimated_highest = estimateSOCFromCell(CellVoltMax_mV);
  } else {
    datalayer.battery.status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00
  }

  datalayer.battery.status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0)

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  //datalayer.battery.status.max_charge_power_W = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //The allowed charge power is not available. We use user set value for now
  datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;

  //datalayer.battery.status.max_discharge_power_W = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts
  //The allowed discharge power is not available. We use user set value for now
  datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;

  datalayer.battery.status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer.battery.status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  if ((millis64() > INTERVAL_60_S) && !set_voltage_limits) {  // millis64: plain millis() wraps after 49.7 days
    set_voltage_limits = true;
    set_voltage_minmax_limits();  // Count cells, and set voltage limits accordingly
  }

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String KiaEGmpBattery::get_uds_info_html() {
  String content;
  content.reserve(1600);

  // clang-format off
  content << "<h4>Cells: " << String(datalayer.battery.info.number_of_cells) << "</h4>"
              "<h4>12V voltage: " << String(leadAcidBatteryVoltage / 10.0f, 1) << "</h4>"
              "<h4>Waterleakage: " << String(waterleakageSensor)  << "</h4>"
              "<h4>Temperature, water inlet: " << String(temperature_water_inlet)  << "</h4>"
              "<h4>Batterymanagement mode: " << String(batteryManagementMode)  << "</h4>"
              "<h4>BMS ignition: " << String(BMS_ign)  << "</h4>";

  return content;
}

void KiaEGmpBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  // UDS frames (0x7EC PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  startedUp = true;
  switch (rx_frame.ID) {
    case 0x055:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x215:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x21A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x235:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x245:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x25A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x275:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2FA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x325:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x330:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x335:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x365:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3BA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:
      //Handled in UDS Superclass
      break;
    default:
      break;
  }
}

uint16_t KiaEGmpBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
      case POLL_GROUP_1:
      /*
            switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          poll_data_pid = rx_frame.data.u8[4];
          transmit_can_frame(&EGMP_7E4_ack);  //Send ack to BMS
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
            allowedChargePower = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]);
            allowedDischargePower = ((rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6]);
            SOC_BMS = rx_frame.data.u8[2] * 5;  //100% = 200 ( 200 * 5 = 1000 )

          } else if (poll_data_pid == 2) {
            // set cell voltages data, start bite, data length from start, start cell
            set_cell_voltages(rx_frame, 2, 6, 0);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 2, 6, 32);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 2, 6, 64);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 2, 6, 96);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 2, 6, 128);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 2, 6, 160);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 1) {
            batteryVoltage = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
            batteryAmps = (rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2];
            temperatureMax = rx_frame.data.u8[5];
            temperatureMin = rx_frame.data.u8[6];
            // temp1 = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 6);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 38);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 70);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 102);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 134);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 166);
          } else if (poll_data_pid == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
            // temp2 = rx_frame.data.u8[1];
            // temp3 = rx_frame.data.u8[2];
            // temp4 = rx_frame.data.u8[3];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 13);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 45);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 77);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 109);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 141);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 173);
          } else if (poll_data_pid == 5) {
            // ac = rx_frame.data.u8[3];
            // Vdiff = rx_frame.data.u8[4];

            // airbag = rx_frame.data.u8[6];
            heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
            CellVminNo = rx_frame.data.u8[3];
            // fanMod = rx_frame.data.u8[4];
            // fanSpeed = rx_frame.data.u8[5];
            leadAcidBatteryVoltage = rx_frame.data.u8[6];  //12v Battery Volts
            //cumulative_charge_current[0] = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 7, 20);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 7, 52);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 7, 84);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 7, 116);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 7, 148);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 7, 180);
          } else if (poll_data_pid == 5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
            // maxDetCell = rx_frame.data.u8[4];
            // minDet = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6];
            // minDetCell = rx_frame.data.u8[7];
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_charge_current[1] = rx_frame.data.u8[1];
            //cumulative_charge_current[2] = rx_frame.data.u8[2];
            //cumulative_charge_current[3] = rx_frame.data.u8[3];
            //cumulative_discharge_current[0] = rx_frame.data.u8[4];
            //cumulative_discharge_current[1] = rx_frame.data.u8[5];
            //cumulative_discharge_current[2] = rx_frame.data.u8[6];
            //cumulative_discharge_current[3] = rx_frame.data.u8[7];
            //set_cumulative_charge_current();
            //set_cumulative_discharge_current();
          } else if (poll_data_pid == 2) {
            set_cell_voltages(rx_frame, 1, 5, 27);
          } else if (poll_data_pid == 3) {
            set_cell_voltages(rx_frame, 1, 5, 59);
          } else if (poll_data_pid == 4) {
            set_cell_voltages(rx_frame, 1, 5, 91);
          } else if (poll_data_pid == 0x0A) {
            set_cell_voltages(rx_frame, 1, 5, 123);
          } else if (poll_data_pid == 0x0B) {
            set_cell_voltages(rx_frame, 1, 5, 155);
          } else if (poll_data_pid == 0x0C) {
            set_cell_voltages(rx_frame, 1, 5, 187);
            //set_cell_count();
          } else if (poll_data_pid == 5) {
            // datalayer.battery.info.number_of_cells = 98;
            SOC_Display = rx_frame.data.u8[1] * 5;
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_charged[0] = rx_frame.data.u8[1];
            // cumulative_energy_charged[1] = rx_frame.data.u8[2];
            //cumulative_energy_charged[2] = rx_frame.data.u8[3];
            //cumulative_energy_charged[3] = rx_frame.data.u8[4];
            //cumulative_energy_discharged[0] = rx_frame.data.u8[5];
            //cumulative_energy_discharged[1] = rx_frame.data.u8[6];
            //cumulative_energy_discharged[2] = rx_frame.data.u8[7];
            // set_cumulative_energy_charged();
          }
          break;
        case 0x27:  //Seventh datarow in PID group
          if (poll_data_pid == 1) {
            //cumulative_energy_discharged[3] = rx_frame.data.u8[1];

            //opTimeBytes[0] = rx_frame.data.u8[2];
            //opTimeBytes[1] = rx_frame.data.u8[3];
            //opTimeBytes[2] = rx_frame.data.u8[4];
            //opTimeBytes[3] = rx_frame.data.u8[5];

            BMS_ign = rx_frame.data.u8[6];
            inverterVoltageFrameHigh = rx_frame.data.u8[7];  // BMS Capacitoir

            // set_cumulative_energy_discharged();
            // set_opTime();
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (poll_data_pid == 1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];  // BMS Capacitoir
          }
          break;
      }*/
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void KiaEGmpBattery::transmit_can(unsigned long currentMillis) {
  if (startedUp) {
    //Send Contactor closing message loop
    // Check if we still have messages to send
    if (messageIndex < sizeof(messageDelays) / sizeof(messageDelays[0])) {

      // Check if it's time to send the next message
      if (currentMillis - startMillis >= messageDelays[messageIndex]) {

        // Transmit the current message
        transmit_can_frame(messages[messageIndex]);

        // Move to the next message
        messageIndex++;
      }
    }

    if (messageIndex >= 63) {
      startMillis = currentMillis;  // Start over!
      messageIndex = 0;
    }

    // UDS PID polling and DTC handling
    transmit_uds_can(currentMillis);
  }
}

void KiaEGmpBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 192;  // TODO: will vary depending on battery
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
    // UDS: send requests to 0x7E4, accept replies from the BMS on 0x7EC.
  setup_uds(0x7E4, 0x7EC);
  static const uint16_t pid_scan_list[] = {
      POLL_GROUP_1,
      POLL_GROUP_2,
      POLL_GROUP_3,
      POLL_GROUP_4,
      POLL_GROUP_5,
      POLL_GROUP_6,
      POLL_GROUP_7,
      POLL_GROUP_8,
      POLL_GROUP_9,
      POLL_GROUP_A,
      POLL_GROUP_B,
      POLL_GROUP_C,
      POLL_GROUP_D,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
