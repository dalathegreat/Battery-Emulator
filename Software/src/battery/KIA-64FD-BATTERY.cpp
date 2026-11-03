#include "KIA-64FD-BATTERY.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

// Function to estimate SOC based on cell voltage
uint16_t Kia64FDBattery::estimateSOCFromCell(uint16_t cellVoltage) {
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
uint16_t Kia64FDBattery::estimateSOC(uint16_t packVoltage, uint16_t cellCount, int16_t currentAmps) {

  if (cellCount == 0) {
    return 0;
  }

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
uint16_t Kia64FDBattery::selectSOC(uint16_t SOC_low, uint16_t SOC_high) {
  if (SOC_low == 0 || SOC_high == 0) {
    return 0;  // If either value is 0, return 0
  }
  if (SOC_low == 10000 || SOC_high == 10000) {
    return 10000;  // If either value is 100%, return 100%
  }
  return (SOC_low < SOC_high) ? SOC_low : SOC_high;  // Otherwise, return the lowest value
}

// Sets a cell voltage that's already expressed in mV (e.g. from 0x215),
// as opposed to set_cell_voltages() which takes an 8-bit reading and applies *20.
void Kia64FDBattery::set_cell_voltage_mv(uint16_t voltage_mV, uint8_t cellNumber) {
  if (cellNumber >= MAX_AMOUNT_CELLS) {
    return;
  }
  datalayer.battery.status.cell_voltages_mV[cellNumber] = voltage_mV;
  cellVoltageReceived[cellNumber] = voltage_mV > 0;
  cellVoltageFrameSeen = true;
}

void Kia64FDBattery::handle_215_cell_voltages(const CAN_frame& rx_frame) {
  if (rx_frame.DLC < 7) {
    return;
  }
  const uint8_t mux = rx_frame.data.u8[3];
  if (mux != 0x01) {
    return;  // mux 0x02 carries a different, non-voltage signal - still unconfirmed, excluded
  }

  // byte[4] is the 1-based starting cell number for THIS frame's batch of 13 cells.
  // It rolls forward by 13 each frame as the BMS cycles through the whole pack -
  // do not hardcode this to 0.
  const uint8_t start_cell_1based = rx_frame.data.u8[4];
  if (start_cell_1based == 0) {
    return;  // defensive: avoid underflow below
  }
  const uint8_t base_cell = start_cell_1based - 1;  // convert to 0-based array index

  const uint8_t max_values = (rx_frame.DLC - 5) / 2;
  for (uint8_t i = 0; i < max_values; i++) {
    const uint8_t offset = 5 + (i * 2);
    if (offset + 1 >= rx_frame.DLC) {
      break;
    }
    const uint16_t mv = (static_cast<uint16_t>(rx_frame.data.u8[offset + 1]) << 8) | rx_frame.data.u8[offset];
    const uint16_t cellNumber = base_cell + i;
    if (cellNumber >= MAX_AMOUNT_CELLS) {
      break;
    }
    set_cell_voltage_mv(mv, static_cast<uint8_t>(cellNumber));
  }
}

void Kia64FDBattery::set_voltage_minmax_limits() {

  uint8_t valid_cell_count = 0;
  for (int i = 0; i < MAX_AMOUNT_CELLS; ++i) {
    if (datalayer.battery.status.cell_voltages_mV[i] > 0) {
      ++valid_cell_count;
    }
  }
  if (valid_cell_count == 96) {
    datalayer.battery.info.number_of_cells = valid_cell_count;
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
  } else if (valid_cell_count == 108) {
    datalayer.battery.info.number_of_cells = valid_cell_count;
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
  } else {
    // We are still starting up? Not all cells available.
    set_voltage_limits = false;
  }
}

void Kia64FDBattery::update_values() {

  if (user_selected_use_estimated_SOC) {
    // Use the simplified pack-based SOC estimation with proper compensation
    datalayer_battery->status.real_soc =
        estimateSOC(batteryVoltage, datalayer_battery->info.number_of_cells, batteryAmps);

    // For comparison or fallback, we can still calculate from min/max cell voltages
    SOC_estimated_lowest = estimateSOCFromCell(CellVoltMin_mV);
    SOC_estimated_highest = estimateSOCFromCell(CellVoltMax_mV);
  } else {
    //Use the SOC from the battery directly
    datalayer_battery->status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00
  }

  datalayer_battery->status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer_battery->status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer_battery->status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0)

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  //datalayer_battery->status.max_charge_power_W = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //datalayer_battery->status.max_discharge_power_W = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts

  //The allowed charge power is not available. We use user set value for now
  // Gets ramped down by inverter function on the webserver
  // The override values are only populated from NVM on the primary datalayer, so always read them from there.
  datalayer_battery->status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;

  //The allowed discharge power is not available. We use user set value for now
  // Gets ramped down by inverter function on the webserver
  datalayer_battery->status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;

  datalayer_battery->status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer_battery->status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  uint16_t actual_min_mV = UINT16_MAX;
  uint16_t actual_max_mV = 0;
  uint8_t valid_cell_count = 0;

  for (int i = 0; i < MAX_AMOUNT_CELLS; ++i) {
    uint16_t v = datalayer.battery.status.cell_voltages_mV[i];
    if (cellVoltageReceived[i] && v > 0) {
      ++valid_cell_count;
      if (v < actual_min_mV) {
        actual_min_mV = v;
      }
      if (v > actual_max_mV) {
        actual_max_mV = v;
      }
    }
  }

  if (valid_cell_count > 0) {
    datalayer.battery.status.cell_max_voltage_mV = actual_max_mV;
    datalayer.battery.status.cell_min_voltage_mV = actual_min_mV;
  } else {
    // No cell data populated yet - fall back to the coarser UDS-reported values
    datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;
    datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;
  }

  if ((millis64() > INTERVAL_60_S) && !set_voltage_limits) {  // millis64: plain millis() wraps after 49.7 days
    set_voltage_limits = true;
    set_voltage_minmax_limits();  // Count cells, and set voltage limits accordingly
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage, battery_index);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}
String Kia64FDBattery::get_uds_info_html() {
  String content;
  content.reserve(1600);

  // clang-format off
    content << "<h4>SOC (BMS): " + String(SOC_BMS / 10.0f, 1) + " %</h4>"
                "<h4>SOC (display): " + String(SOC_Display / 10.0f, 1) + " %</h4>"
                "<h4>SOC (estimated from lowest cell): " + String(SOC_estimated_lowest / 100.0f, 2) + " %</h4>"
                "<h4>SOC (estimated from highest cell): " + String(SOC_estimated_highest / 100.0f, 2) + " %</h4>"
                "<h4>SOH: " + String(batterySOH / 10.0f, 1) + " %</h4>"
                "<h4>Allowed charge power: " + String(allowedChargePower / 100.0f, 2) + " kW</h4>"
                "<h4>Allowed discharge power: " + String(allowedDischargePower / 100.0f, 2) + " kW</h4>"
                "<h4>Highest cell: no " + String(CellVmaxNo) + "</h4>"
                "<h4>Lowest cell: no " + String(CellVminNo) + "</h4>"
                "<h4>12V voltage: " + String(leadAcidBatteryVoltage / 10.0f, 1) + " V</h4>"
                "<h4>Inverter voltage: " + String(inverterVoltage) + " V</h4>"
                "<h4>Temperature, water inlet: " + String(temperature_water_inlet) + " &deg;C</h4>"
                "<h4>Temperature, heater: " + String(heatertemp) + " &deg;C</h4>"
                "<h4>Batterymanagement mode: " + String(batteryManagementMode) + "</h4>"
                "<h4>Cumulative Charge Energy: " + String(cumulativeChargeEnergy) + " Wh</h4>"
                "<h4>Cumulative Discharge Energy: " + String(cumulativeDischargeEnergy) + " Wh</h4>"
                "<h4>Operation Time: " + String(opTime) + " s</h4>"
                "<h4>BMS ignition: " + String(BMS_ign) + "</h4>"
                "<h4>BMS Main Relay: " + String(batteryRelay) + "</h4>";
  // clang-format on
  return content;
}

uint16_t Kia64FDBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case POLL_GROUP_1:  //59 bytes
      // Frame 10 (ef fb e7)
      //data[0-2] We are not sure what these are.

      // Frame 21 (ed 02 00 00 00 00 00) data3-9
      SOC_BMS = data[4] * 5;                               //56
      allowedChargePower = ((data[5] << 8) + data[6]);     //00 00 (apparently not working)
      allowedDischargePower = ((data[7] << 8) + data[8]);  //00 00 (apparently not working)

      //Frame 22 (ff d6 0e 26 0a 09 09) data10-16
      batteryAmps = (data[10] << 8) + data[11];
      batteryVoltage = (data[12] << 8) + data[13];
      temperatureMax = data[14];
      temperatureMin = data[15];
      //temperatureAvg = data[16]; Not required

      // Frame 23 (09 09 09 09 00 7f a8) data17-23
      temperature_water_inlet = data[22];
      CellVoltMax_mV = (data[23] * 20);

      // Frame 24 (21 a5 57 00 00 86 00) data24-30
      CellVmaxNo = data[24];
      CellVoltMin_mV = (data[25] * 20);
      CellVminNo = data[26];
      leadAcidBatteryVoltage = data[29];
      // Frame 25 (01 fe 98 00 01 fb c0) data31-37
      cumulativeChargeEnergy = data[31] << 16 | data[32] << 8 | data[33];
      cumulativeDischargeEnergy = data[35] << 16 | data[36] << 8 | data[37];

      //Frame 26 (00 00 da 40 00 00 d5) data38-44
      cumulativeChargeEnergy2 = data[39] << 16 | data[40] << 8 | data[41];
      cumulativeDischargeEnergy2 = data[43] << 16 | data[44] << 8 | data[45];  //Flow over

      //Frame 27 (73 01 84 e0 c2 00 02) data45-51
      opTime = data[46] << 24 | data[47] << 16 | data[48] << 8 | data[49];
      BMS_ign = data[50];
      inverterVoltage = (((data[51] << 8) + data[52]) / 2);  //Flow over
      batteryRelay = data[51] & 0x01;
      //Frame 28 (c9 00 00 00 00 0b b8) data52-58
      break;
    case POLL_GROUP_2:  //Cellvoltages (Cell 1-32)
      // process_cell_voltage_group(data, 0);
      break;
    case POLL_GROUP_3:  //Cellvoltages (Cell 33-64)
      // process_cell_voltage_group(data, 32);
      break;
    case POLL_GROUP_4:  //Cellvoltages (Cell 65-96)
      // process_cell_voltage_group(data, 64);
      break;
    case POLL_GROUP_A:  //Cellvoltages (Cell 97-128)
      // process_cell_voltage_group(data, 96);
      break;
    case POLL_GROUP_B:  //Cellvoltages (Cell 129-160)
      // process_cell_voltage_group(data, 128);
      break;
    case POLL_GROUP_C:  //Cellvoltages (Cell 161-192)
      // process_cell_voltage_group(data, 160);
      break;
    case POLL_GROUP_5:
      //Frame 0 (10 2e 62 01 05 ff fb 74) //data0-2
      //Frame21 0f 01 2c 01 01 2c 15 //data3-9
      //Frame22 15 15 15 15 15 15 6c //data10-16
      //Frame23 34 6c 34 00 00 64 1e //data17-23
      heatertemp = data[23];
      //Frame24 00 03 e8 39 38 c6 00 //data24-30
      batterySOH = (data[25] << 8) | data[26];
      //amountOfCells = data[29];
      //Frame25 53 00 00 00 00 00 00 //data31-37
      SOC_Display = data[31] * 5;
      //Frame26 00 15 15 15 16 aa aa //data38-44
      break;
    case POLL_GROUP_6:
      batteryManagementMode = data[14];
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void Kia64FDBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  // UDS frames (0x7EC PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  startedUp = true;
  switch (rx_frame.ID) {
    case 0x055:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1F5:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x215:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      handle_215_cell_voltages(rx_frame);
      break;
      break;
    case 0x21A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x235:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x245:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x25A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x275:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2FA:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x325:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x330:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x335:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x365:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3BA:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3F5:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:
      // handled in UDS superclass, so nothing to do here
      break;
    default:
      break;
  }
}

void Kia64FDBattery::transmit_can(unsigned long currentMillis) {
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

void Kia64FDBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 96;  //Start in 96 cell mode, will be updated if 108s cells are detected
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;  //Start wide, narrow down later
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // Main battery only. Battery 2's flag is driven by parallel_safety.cpp once
  // the packs are voltage matched, so the pointer is null there.
  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }

  // UDS: send requests to 0x7E4, accept replies from the BMS on 0x7EC. Also passing true to isFD
  setup_uds(0x7E4, 0x7EC, true);
  static const uint16_t pid_scan_list[] = {
      POLL_GROUP_1, POLL_GROUP_2, POLL_GROUP_3, POLL_GROUP_4, POLL_GROUP_5, POLL_GROUP_6,
      POLL_GROUP_7, POLL_GROUP_8, POLL_GROUP_A, POLL_GROUP_B, POLL_GROUP_C,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
