#include "ECMP-V2-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"

#ifndef SMALL_FLASH_DEVICE

// Store up to 16 unique live JDD fault codes broadcast from BMS on 0x774
static constexpr uint8_t MAX_JDD_DTCS = 16;
static uint32_t jdd_dtcs[MAX_JDD_DTCS] = {0};
static uint8_t jdd_dtc_count = 0;
static String reset_log = "Not run yet";

static void log_reset_step(const char* label, const CAN_frame& rx) {
  char buf[32];
  if (rx.data.u8[1] == 0x7F) {
    snprintf(buf, sizeof(buf), "%s:NRC(%02X) ", label, rx.data.u8[3]);
  } else if (rx.data.u8[1] == 0x71 && rx.data.u8[2] == 0x03) {
    snprintf(buf, sizeof(buf), "%s:OK(%02X) ", label, rx.data.u8[5]);
  } else {
    snprintf(buf, sizeof(buf), "%s:%02X ", label, rx.data.u8[1]);
  }
  reset_log += buf;
}

// Universal Stellantis eCMP v2 / STLA 4-bit PSA nibble-sum checksum
static void apply_psa_checksum(CAN_frame& frame, uint8_t byte_idx, uint8_t counter, bool cnt_in_high_nibble) {
  uint8_t cnt = counter & 0x0F;
  int sum = ((frame.ID >> 8) & 0xF) + ((frame.ID >> 4) & 0xF) + (frame.ID & 0xF) + cnt;
  for (uint8_t i = 0; i < frame.DLC; i++) {
    if (i == byte_idx) {
      continue;
    }
    sum += (frame.data.u8[i] >> 4) + (frame.data.u8[i] & 0xF);
  }
  uint8_t chk = static_cast<uint8_t>((0xF - (sum & 0xF)) & 0xF);
  frame.data.u8[byte_idx] =
      cnt_in_high_nibble ? static_cast<uint8_t>((cnt << 4) | chk) : static_cast<uint8_t>((chk << 4) | cnt);
}

void EcmpV2Battery::update_values() {
  // Auto-sizes 102S (54 kWh eCMP v2), 96S (82 kWh STLA Medium e-3008), or 108S from 0x583
  if (NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY >= 80 && NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY <= MAX_AMOUNT_CELLS) {
    datalayer_battery->info.number_of_cells = NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY;
    datalayer_battery->info.max_design_voltage_dV =
        (static_cast<uint32_t>(NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY) * MAX_CELL_VOLTAGE_MV) / 100;
    datalayer_battery->info.min_design_voltage_dV =
        (static_cast<uint32_t>(NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY) * MIN_CELL_VOLTAGE_MV) / 100;
  }

  if (HV_BATT_SOC <= 1000) {
    datalayer_battery->status.real_soc = HV_BATT_SOC * 10;
  }

  // Real SOH: prefer verified UDS 0xDF49 (0.1%), fallback to broadcast 0x5B5 second 12 bits (* 0.03%)
  if (pid_soh_pack > 0 && pid_soh_pack <= 1000) {
    datalayer_battery->status.soh_pptt = pid_soh_pack * 10;
  } else if (soh_5b5_pptt > 0 && soh_5b5_pptt <= 10000) {
    datalayer_battery->status.soh_pptt = soh_5b5_pptt;
  }

  if (HV_BATT_SOE_MAX > 100 && HV_BATT_SOE_MAX <= 2000) {
    datalayer_battery->info.total_capacity_Wh = static_cast<uint32_t>(HV_BATT_SOE_MAX) * 100;
  }
  if (HV_BATT_SOE_HD > 0 && HV_BATT_SOE_HD < 0x3FF0) {
    datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(HV_BATT_SOE_HD) * 10;
  } else {
    datalayer_battery->status.remaining_capacity_Wh =
        static_cast<uint32_t>((static_cast<double>(datalayer_battery->status.real_soc) / 10000.0) *
                              datalayer_battery->info.total_capacity_Wh);
  }

  uint8_t num_cells = datalayer_battery->info.number_of_cells;
  if (num_cells > 108) {
    num_cells = 108;
  }
  for (uint8_t i = num_cells; i < MAX_AMOUNT_CELLS; i++) {
    datalayer_battery->status.cell_voltages_mV[i] = 0;
  }

  uint32_t sum_cells_mV = 0;
  uint8_t valid_cell_count = 0;
  uint16_t min_cell_mv = 0xFFFF;
  uint16_t max_cell_mv = 0;

  for (uint8_t i = 0; i < num_cells; i++) {
    uint16_t mv = cellvoltages[i];
    if (mv > 3000 && mv <= 4500) {
      datalayer_battery->status.cell_voltages_mV[i] = mv;
      sum_cells_mV += mv;
      valid_cell_count++;
      if (mv < min_cell_mv)
        min_cell_mv = mv;
      if (mv > max_cell_mv)
        max_cell_mv = mv;
    } else {
      datalayer_battery->status.cell_voltages_mV[i] = 0;
    }
  }

  if (CELL_MIN_MV_5A3 > 3000 && CELL_MAX_MV_5A3 >= CELL_MIN_MV_5A3 && CELL_MAX_MV_5A3 <= 4500) {
    datalayer_battery->status.cell_min_voltage_mV = CELL_MIN_MV_5A3;
    datalayer_battery->status.cell_max_voltage_mV = CELL_MAX_MV_5A3;
  } else if (valid_cell_count > 0 && min_cell_mv != 0xFFFF) {
    datalayer_battery->status.cell_min_voltage_mV = min_cell_mv;
    datalayer_battery->status.cell_max_voltage_mV = max_cell_mv;
  } else {
    datalayer_battery->status.cell_min_voltage_mV = 3700;
    datalayer_battery->status.cell_max_voltage_mV = 3700;
  }

  if (CONTACTORS_STATE == 2 && HV_BATT_REAL_VOLT_HD > 3000) {
    datalayer_battery->status.voltage_dV = HV_BATT_REAL_VOLT_HD;
  } else if (valid_cell_count > 0 && sum_cells_mV > 0) {
    uint32_t est_pack_mV = (sum_cells_mV / valid_cell_count) * num_cells;
    datalayer_battery->status.voltage_dV = static_cast<uint16_t>(est_pack_mV / 100);
  } else if (CELL_MIN_MV_5A3 > 3000 && CELL_MAX_MV_5A3 > 3000) {
    uint32_t avg_mv = (static_cast<uint32_t>(CELL_MIN_MV_5A3) + CELL_MAX_MV_5A3) / 2;
    datalayer_battery->status.voltage_dV = static_cast<uint16_t>((avg_mv * num_cells) / 100);
  } else if (datalayer_battery->status.voltage_dV == 0) {
    datalayer_battery->status.voltage_dV = 3700;
  }

  datalayer_battery->status.current_dA = -HV_BATT_REAL_CURR_HD;
  datalayer_battery->status.active_power_W = (static_cast<int32_t>(datalayer_battery->status.voltage_dV) *
                                              static_cast<int32_t>(datalayer_battery->status.current_dA)) /
                                             100;

  uint32_t max_chg_W = 0;
  if (HV_BATT_STABLE_CH_POWER_HD < 10000) {
    max_chg_W = static_cast<uint32_t>(10000 - HV_BATT_STABLE_CH_POWER_HD) * 100;
  } else if (HV_BATT_NOM_CH_POWER_HD < 10000) {
    max_chg_W = static_cast<uint32_t>(10000 - HV_BATT_NOM_CH_POWER_HD) * 100;
  }
  uint32_t max_disch_W = static_cast<uint32_t>(HV_BATT_NOMINAL_DISCH_POWER_HD) * 100;
  if (max_disch_W == 0) {
    max_disch_W = static_cast<uint32_t>(HV_BATT_PEAK_DISCH_POWER_HD) * 100;
  }
  datalayer_battery->status.max_charge_power_W = max_chg_W;
  datalayer_battery->status.max_discharge_power_W = max_disch_W;

  uint8_t num_temps = NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY;
  if (num_temps == 0 || num_temps > 56) {
    num_temps = 56;
  }
  int8_t min_t = 127;
  int8_t max_t = -40;
  bool found_temp = false;
  for (uint8_t i = 0; i < num_temps; i++) {
    if (celltemp_valid[i]) {
      if (celltemps[i] == 25 && TEMPERATURE_MINIMUM_C < 18) {
        continue;
      }
      found_temp = true;
      if (celltemps[i] < min_t)
        min_t = celltemps[i];
      if (celltemps[i] > max_t)
        max_t = celltemps[i];
    }
  }
  if (found_temp) {
    datalayer_battery->status.temperature_min_dC = static_cast<int16_t>(min_t) * 10;
    datalayer_battery->status.temperature_max_dC = static_cast<int16_t>(max_t) * 10;
  } else {
    datalayer_battery->status.temperature_min_dC = static_cast<int16_t>(TEMPERATURE_MINIMUM_C) * 10;
    datalayer_battery->status.temperature_max_dC = static_cast<int16_t>(TEMPERATURE_MINIMUM_C) * 10;
  }

  if (HV_BATT_FC_INSU_MINUS_RES > 0 || HV_BATT_FC_INSU_PLUS_RES > 0) {
    uint16_t insu = HV_BATT_FC_INSU_MINUS_RES;
    if (HV_BATT_FC_INSU_PLUS_RES > 0 && (insu == 0 || HV_BATT_FC_INSU_PLUS_RES < insu)) {
      insu = HV_BATT_FC_INSU_PLUS_RES;
    }
    datalayer_battery->status.insulation_resistance_kOhm = insu;
    datalayer_battery->status.insulation_resistance_available = true;
  }

  if (CONTACTOR_OPENING_REASON == 5 || CONTACTOR_OPENING_REASON == 6) {
    set_event(EVENT_HVIL_FAILURE, CONTACTOR_OPENING_REASON, battery_index);
  } else {
    clear_event(EVENT_HVIL_FAILURE, battery_index);
  }

  if (pid_12v < 11000) {
    set_event(EVENT_12V_LOW, 11, battery_index);
  } else {
    clear_event(EVENT_12V_LOW, battery_index);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String EcmpV2Battery::get_uds_info_html() {
  String content;
  content.reserve(4000);

#define H4_COND(label, val, na_val, unit) \
  content << "<h4>" << label << ": " << (val == na_val ? "N/A" : String(val)) << unit << "</h4>"
#define H4_BOOL(label, flag) content << "<h4>" << label << ": " << (flag ? "Yes" : "No") << "</h4>"

  content << "<h3>Stellantis eCMP v2 / STLA (TBMU) Status</h3>";

  content << "<h4>Emulator Contactor Cmd State: ";
  switch (cmd_state) {
    case CMD_IDLE:
      content << "IDLE (Open)";
      break;
    case CMD_WAKE_EDGE:
      content << "WAKE EDGE (0x342/0x27A)";
      break;
    case CMD_PRECHARGE_REQ:
      content << "PRECHARGE REQ (0x448=0x60)";
      break;
    case CMD_CLOSED_HOLD:
      content << "CLOSED HOLD (0x448=0x20)";
      break;
  }
  content << "</h4>";

  content << "<h4>BMS Contactor State (0x4F4): ";
  switch (CONTACTORS_STATE) {
    case 0:
      content << "Open";
      break;
    case 1:
      content << "Precharge";
      break;
    case 2:
      content << "Closed";
      break;
    default:
      content << "Unknown (" << String(CONTACTORS_STATE) << ")";
  }
  content << "</h4>";

  content << "<h4>TBMU Fault Type (0x373): ";
  switch (TBMU_FAULT_TYPE) {
    case 0:
      content << "No fault";
      break;
    case 1:
      content << "Level 1: Warning Lamp";
      break;
    case 2:
      content << "Level 2: Stop Lamp";
      break;
    case 3:
      content << "Level 3: Stop Lamp + Contactor Opening";
      break;
    case 4:
      content << "Level 4: Stop Lamp + Active Discharge";
      break;
    case 5:
      content << "Level 5: Powertrain Inhibition";
      break;
    default:
      content << "Code " << String(TBMU_FAULT_TYPE);
  }
  content << "</h4>";

  content << "<h4>Live BMS Faults (0x774) [" << String(jdd_dtc_count) << "]: ";
  if (jdd_dtc_count > 0) {
    const char prefixes[4] = {'P', 'C', 'B', 'U'};
    char dtc_buf[16];
    for (uint8_t i = 0; i < jdd_dtc_count; i++) {
      uint8_t b0 = (jdd_dtcs[i] >> 16) & 0xFF;
      uint8_t b1 = (jdd_dtcs[i] >> 8) & 0xFF;
      uint8_t ftb = jdd_dtcs[i] & 0xFF;
      snprintf(dtc_buf, sizeof(dtc_buf), "%c%X%X%02X-%02X", prefixes[(b0 >> 6) & 0x03], (b0 >> 4) & 0x03, b0 & 0x0F, b1,
               ftb);
      if (i > 0)
        content << ", ";
      content << dtc_buf;
    }
  } else {
    content << "None";
  }
  content << "</h4>";

  content << "<h4>Last Reset Log: " << reset_log << "</h4>";

  content << "<h4>Contactor Opening Reason (0x583): ";
  switch (CONTACTOR_OPENING_REASON) {
    case 0:
      content << "No error";
      break;
    case 1:
      content << "Crash!";
      break;
    case 2:
      content << "12V undervoltage";
      break;
    case 3:
      content << "12V overvoltage";
      break;
    case 4:
      content << "Battery temperature";
      break;
    case 5:
      content << "Interlock line open";
      break;
    case 6:
      content << "e-Service plug disconnected";
      break;
    default:
      content << "Unknown (" << String(CONTACTOR_OPENING_REASON) << ")";
  }
  content << "</h4>";

  H4_BOOL("Crash Memorized (0x4F4)", HV_BATT_CRASH_MEMORIZED);
  content << "<h4>Real SOH — 0x5B5: " << String(soh_5b5_pptt / 100.0f, 2)
          << "% | UDS Pack (0xDF49): " << (pid_soh_pack ? String(pid_soh_pack / 10.0f, 1) + "%" : "N/A")
          << " | Min Cell (0xDF44): " << (pid_soh_min_cell ? String(pid_soh_min_cell / 10.0f, 1) + "%" : "N/A")
          << "</h4>";
  content << "<h4>HV Link Voltage (0x373 / 0x353): " << String(HV_BATT_REAL_VOLT_HD / 10.0f, 1) << " V / "
          << String(HV_BATT_COP_VOLTAGE / 10.0f, 1) << " V</h4>";
  content << "<h4>BMS Current (0x373): " << String(HV_BATT_REAL_CURR_HD / 10.0f, 1) << " A</h4>";
  content << "<h4>Cells / Temp Sensors (0x583): " << String(NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY) << "S / "
          << String(NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY) << " sensors</h4>";
  content << "<h4>Max/Min Cell (0x5A3): #" << String(CELL_MAX_IDX_5A3) << " " << String(CELL_MAX_MV_5A3) << " mV / #"
          << String(CELL_MIN_IDX_5A3) << " " << String(CELL_MIN_MV_5A3) << " mV</h4>";
  content << "<h4>SOE (0x3B4): " << String(HV_BATT_SOE_HD * 10) << " / " << String(HV_BATT_SOE_MAX * 100) << " Wh</h4>";
  content << "<h4>Insulation (0x574): - " << String(HV_BATT_FC_INSU_MINUS_RES) << " / + "
          << String(HV_BATT_FC_INSU_PLUS_RES) << "</h4>";

  H4_COND("UDS 12V Supply", pid_12v, 255, " mV");
  H4_COND("UDS Contactor Status", pid_contactor_status, 255, "");
  H4_COND("UDS Reason Open", pid_reason_open, 255, "");
  H4_COND("UDS Weld Check", pid_welding_detection, 255, "");
  H4_COND("UDS Wire Crash", pid_wire_crash, 255, "");
  H4_COND("UDS HVIL State", pid_hvil_state, 255, "");
  H4_COND("UDS BMS State", pid_bms_state, 255, "");

  H4_BOOL("Alert Battery", ALERT_BATT);
  H4_BOOL("Alert Low SOC", ALERT_LOW_SOC);
  H4_BOOL("Alert High SOC", ALERT_HIGH_SOC);
  H4_BOOL("Alert Overcharge", ALERT_OVERCHARGE);
  H4_BOOL("Alert Temp Diff", ALERT_TEMP_DIFF);
  H4_BOOL("Alert High Temp", ALERT_HIGH_TEMP);
  H4_BOOL("Alert Overvoltage", ALERT_OVERVOLTAGE);
  H4_BOOL("Alert Cell Overvoltage", ALERT_CELL_OVERVOLTAGE);
  H4_BOOL("Alert Cell Undervoltage", ALERT_CELL_UNDERVOLTAGE);
  H4_BOOL("Alert Cell Poor Consistency", ALERT_CELL_POOR_CONSIST);

  content << "<h4>Press Open Contactors before running diagnostic reset commands below:</h4>";

#undef H4_COND
#undef H4_BOOL
  return content;
}

void EcmpV2Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // Pause Car-TX for 50ms whenever 0x694 starts a multi-frame ISO-TP response (e.g. Read DTC 59 02!)
  // so 1ms Consecutive Frames (21, 22, 23...) never collide with the 5ms Car-TX loop!
  if (rx_frame.ID == 0x694 && rx_frame.DLC >= 2 && (rx_frame.data.u8[0] & 0xF0) == 0x10) {
    isotp_quiet_until_ms = millis() + 50;
  }

  if (!(UserRequestContactorReset || UserRequestCollisionReset || UserRequestIsolationReset || UserRequestBmsReset)) {
    if (handle_incoming_uds_can_frame(rx_frame)) {
      return;
    }
  }

  switch (rx_frame.ID) {
    case 0x334: {
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8 && HV_BATT_SOC <= 1000) {
        uint8_t mux = rx_frame.data.u8[0] >> 3;
        uint64_t val56 =
            (static_cast<uint64_t>(rx_frame.data.u8[1]) << 48) | (static_cast<uint64_t>(rx_frame.data.u8[2]) << 40) |
            (static_cast<uint64_t>(rx_frame.data.u8[3]) << 32) | (static_cast<uint64_t>(rx_frame.data.u8[4]) << 24) |
            (static_cast<uint64_t>(rx_frame.data.u8[5]) << 16) | (static_cast<uint64_t>(rx_frame.data.u8[6]) << 8) |
            (static_cast<uint64_t>(rx_frame.data.u8[7]));
        for (uint8_t k = 0; k < 4; k++) {
          uint16_t idx = mux * 4 + k;
          if (idx < NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY && idx < 108) {
            uint16_t mv = static_cast<uint16_t>((val56 >> (42 - 14 * k)) & 0x3FFF);
            if (mv > 3000 && mv <= 4500) {
              cellvoltages[idx] = mv;
            }
          }
        }
      }
      break;
    }

    case 0x3F4: {
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8 && HV_BATT_SOC <= 1000) {
        uint8_t mux = (rx_frame.data.u8[0] & 0xE0) >> 5;
        for (uint8_t k = 0; k < 7; k++) {
          uint8_t idx = mux * 7 + k;
          uint8_t raw = rx_frame.data.u8[1 + k];
          if (idx < NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY && idx < 56 && raw < 0xFA && raw != 0x28) {
            if (raw == 0x41 && TEMPERATURE_MINIMUM_C < 18) {
              continue;
            }
            celltemps[idx] = static_cast<int8_t>(static_cast<int16_t>(raw) - 40);
            celltemp_valid[idx] = true;
          }
        }
      }
      break;
    }

    case 0x353:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 5) {
        HV_BATT_COP_VOLTAGE = (rx_frame.data.u8[1] << 5) | (rx_frame.data.u8[2] >> 3);
        HV_BATT_COP_CURRENT = static_cast<int16_t>((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]) >> 3;
      }
      break;

    case 0x373:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8) {
        HV_BATT_REAL_CURR_HD = static_cast<int16_t>((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
        HV_BATT_REAL_VOLT_HD = ((rx_frame.data.u8[3] & 0x3F) << 8) | rx_frame.data.u8[4];
        TBMU_FAULT_TYPE = (rx_frame.data.u8[7] & 0xE0) >> 5;
      }
      break;

    case 0x4F4:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8) {
        HV_BATT_SOC = (rx_frame.data.u8[1] << 2) | (rx_frame.data.u8[2] >> 6);
        CONTACTORS_STATE = (rx_frame.data.u8[2] & 0x30) >> 4;
        HV_BATT_CRASH_MEMORIZED = ((rx_frame.data.u8[2] & 0x08) >> 3) != 0;
      }
      break;

    case 0x3B4:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8) {
        uint16_t soe_hd =
            ((rx_frame.data.u8[4] & 0x03) << 12) | (rx_frame.data.u8[5] << 4) | ((rx_frame.data.u8[6] & 0xF0) >> 4);
        uint16_t soe_max = ((rx_frame.data.u8[6] & 0x07) << 8) | rx_frame.data.u8[7];
        if (soe_hd > 0 && soe_hd < 0x3FF0) {
          HV_BATT_SOE_HD = soe_hd;
        }
        if (soe_max > 100 && soe_max <= 2000) {
          HV_BATT_SOE_MAX = soe_max;
        }
      }
      break;

    case 0x414:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 3) {
        HV_BATT_REAL_POWER_HD = static_cast<int16_t>((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]) >> 1;
      }
      break;

    case 0x554:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8) {
        HV_BATT_PEAK_DISCH_POWER_HD = (rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[2] >> 2);
        HV_BATT_PEAK_CH_POWER_HD =
            ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4);
        HV_BATT_NOM_CH_POWER_HD =
            ((rx_frame.data.u8[4] & 0x0F) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[6] >> 6);
      }
      break;

    case 0x4D4:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8) {
        HV_BATT_STABLE_CH_POWER_HD = (rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[2] >> 2);
        HV_BATT_STABLE_DISCH_POWER_HD =
            ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4);
        HV_BATT_NOMINAL_DISCH_POWER_HD =
            ((rx_frame.data.u8[4] & 0x0F) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[6] >> 6);
      }
      break;

    case 0x314:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 3) {
        MIN_ALLOW_DISCHRG_VOLTAGE = (rx_frame.data.u8[1] << 3) | (rx_frame.data.u8[2] >> 5);
      }
      break;

    case 0x574:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 7) {
        HV_BATT_FC_INSU_MINUS_RES = (rx_frame.data.u8[0] << 5) | (rx_frame.data.u8[1] >> 3);
        HV_BATT_FC_VHL_INSU_PLUS_RES =
            ((rx_frame.data.u8[1] & 0x07) << 10) | (rx_frame.data.u8[2] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6);
        HV_BATT_ONLY_INSU_MINUS_RES = ((rx_frame.data.u8[3] & 0x3F) << 7) | (rx_frame.data.u8[4] >> 1);
        HV_BATT_FC_INSU_PLUS_RES =
            ((rx_frame.data.u8[4] & 0x01) << 12) | (rx_frame.data.u8[5] << 4) | (rx_frame.data.u8[6] >> 4);
      }
      break;

    case 0x583:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 7) {
        if (rx_frame.data.u8[2] < 0xFA) {
          TEMPERATURE_MINIMUM_C = static_cast<int8_t>(static_cast<int16_t>(rx_frame.data.u8[2]) - 40);
        }
        ALERT_BATT = (rx_frame.data.u8[3] & 0x80) != 0;
        ALERT_TEMP_DIFF = (rx_frame.data.u8[3] & 0x40) != 0;
        ALERT_HIGH_TEMP = (rx_frame.data.u8[3] & 0x20) != 0;
        ALERT_OVERVOLTAGE = (rx_frame.data.u8[3] & 0x10) != 0;
        ALERT_LOW_SOC = (rx_frame.data.u8[3] & 0x08) != 0;
        ALERT_HIGH_SOC = (rx_frame.data.u8[3] & 0x04) != 0;
        ALERT_CELL_OVERVOLTAGE = (rx_frame.data.u8[3] & 0x02) != 0;
        ALERT_CELL_UNDERVOLTAGE = (rx_frame.data.u8[3] & 0x01) != 0;
        ALERT_SOC_JUMP = (rx_frame.data.u8[4] & 0x80) != 0;
        ALERT_CELL_POOR_CONSIST = (rx_frame.data.u8[4] & 0x40) != 0;
        ALERT_OVERCHARGE = (rx_frame.data.u8[4] & 0x20) != 0;
        CONTACTOR_OPENING_REASON = (rx_frame.data.u8[4] & 0x1C) >> 2;
        if (rx_frame.data.u8[5] > 0 && rx_frame.data.u8[5] <= 56) {
          NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY = rx_frame.data.u8[5];
          for (uint8_t i = NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY; i < 56; i++) {
            celltemp_valid[i] = false;
          }
        }
        if (rx_frame.data.u8[6] >= 80 && rx_frame.data.u8[6] <= 108) {
          NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY = rx_frame.data.u8[6];
          for (uint8_t i = NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY; i < 108; i++) {
            cellvoltages[i] = 0;
          }
        }
      }
      break;

    case 0x5A3:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC == 8) {
        CELL_MAX_IDX_5A3 = rx_frame.data.u8[2];
        CELL_MAX_MV_5A3 = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]) >> 2;
        CELL_MIN_IDX_5A3 = rx_frame.data.u8[5];
        CELL_MIN_MV_5A3 = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) >> 2;
      }
      break;

    case 0x5B5:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 3) {
        uint16_t raw_soh_c = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2];
        if (raw_soh_c > 0 && raw_soh_c < 0xFF0) {
          uint32_t pptt = static_cast<uint32_t>(raw_soh_c) * 3;
          soh_5b5_pptt = (pptt > 10000) ? 10000 : static_cast<uint16_t>(pptt);
        }
      }
      break;

    case 0x774: {
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC >= 4 && rx_frame.data.u8[0] == 0x99 && rx_frame.data.u8[1] != 0xFF) {
        uint32_t dtc24 = (static_cast<uint32_t>(rx_frame.data.u8[1]) << 16) |
                         (static_cast<uint32_t>(rx_frame.data.u8[2]) << 8) | rx_frame.data.u8[3];
        bool exists = false;
        for (uint8_t i = 0; i < jdd_dtc_count; i++) {
          if (jdd_dtcs[i] == dtc24) {
            exists = true;
            break;
          }
        }
        if (!exists && jdd_dtc_count < MAX_JDD_DTCS) {
          jdd_dtcs[jdd_dtc_count++] = dtc24;
        }
      }
      break;
    }

    case 0x694:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (UserRequestContactorReset) {
        uint8_t sid = rx_frame.data.u8[1];
        uint8_t sub = rx_frame.data.u8[2];
        if (sid == 0x7F && rx_frame.data.u8[3] == 0x78) {
          break;
        }
        bool is_nrc = (sid == 0x7F);

        if (ContactorResetStatemachine == 1 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1003", rx_frame);
          ContactorResetStatemachine = (sid == 0x50) ? 2 : 14;
        } else if (ContactorResetStatemachine == 3 && (sid == 0x71 || is_nrc)) {
          if (is_nrc)
            log_reset_step("DD35", rx_frame);
          ContactorResetStatemachine = (sid == 0x71) ? 4 : 6;
        } else if (ContactorResetStatemachine == 5 && (sid == 0x71 || is_nrc)) {
          if (sid == 0x71 && sub == 0x03 && rx_frame.data.u8[5] == 0x01) {
            ContactorResetStatemachine = 4;
          } else {
            log_reset_step("DD35", rx_frame);
            ContactorResetStatemachine = 6;
          }
        } else if (ContactorResetStatemachine == 7 && (sid == 0x71 || is_nrc)) {
          log_reset_step("DD0B", rx_frame);
          ContactorResetStatemachine = 8;
        } else if (ContactorResetStatemachine == 9 && (sid == 0x71 || is_nrc)) {
          if (is_nrc)
            log_reset_step("DFFC", rx_frame);
          ContactorResetStatemachine = (sid == 0x71) ? 10 : 12;
        } else if (ContactorResetStatemachine == 11 && (sid == 0x71 || is_nrc)) {
          if (sid == 0x71 && sub == 0x03 && rx_frame.data.u8[5] == 0x01) {
            ContactorResetStatemachine = 10;
          } else {
            log_reset_step("DFFC", rx_frame);
            ContactorResetStatemachine = 12;
          }
        } else if (ContactorResetStatemachine == 13 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1001", rx_frame);
          ContactorResetStatemachine = 14;
        } else if (ContactorResetStatemachine == 15 && (sid == 0x54 || is_nrc)) {
          log_reset_step("14FF", rx_frame);
          jdd_dtc_count = 0;
          UserRequestContactorReset = false;
          ContactorResetStatemachine = COMPLETED_STATE;
          timeSpentContactorReset = COMPLETED_STATE;
          cmd_state = CMD_IDLE;
          state_entered_ms = 0;
          closed_confirmed_ms = 0;
        }
      } else if (UserRequestBmsReset) {
        uint8_t sid = rx_frame.data.u8[1];
        if (sid == 0x7F && rx_frame.data.u8[3] == 0x78) {
          break;
        }
        bool is_nrc = (sid == 0x7F);

        if (BmsResetStatemachine == 1 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1003", rx_frame);
          BmsResetStatemachine = 2;
        } else if (BmsResetStatemachine == 3 && (sid == 0x51 || is_nrc)) {
          log_reset_step("1103", rx_frame);
          jdd_dtc_count = 0;
          UserRequestBmsReset = false;
          BmsResetStatemachine = COMPLETED_STATE;
          timeSpentBmsReset = COMPLETED_STATE;
          cmd_state = CMD_IDLE;
          state_entered_ms = 0;
          closed_confirmed_ms = 0;
        }
      } else if (UserRequestCollisionReset) {
        uint8_t sid = rx_frame.data.u8[1];
        uint8_t sub = rx_frame.data.u8[2];
        if (sid == 0x7F && rx_frame.data.u8[3] == 0x78) {
          break;
        }
        bool is_nrc = (sid == 0x7F);

        if (CollisionResetStatemachine == 1 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1003", rx_frame);
          CollisionResetStatemachine = (sid == 0x50) ? 2 : 6;
        } else if (CollisionResetStatemachine == 3 && (sid == 0x71 || is_nrc)) {
          if (is_nrc)
            log_reset_step("DF60", rx_frame);
          CollisionResetStatemachine = (sid == 0x71) ? 4 : 6;
        } else if (CollisionResetStatemachine == 5 && (sid == 0x71 || is_nrc)) {
          if (sid == 0x71 && sub == 0x03 && rx_frame.data.u8[5] == 0x01) {
            CollisionResetStatemachine = 4;
          } else {
            log_reset_step("DF60", rx_frame);
            CollisionResetStatemachine = 6;
          }
        } else if (CollisionResetStatemachine == 7 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1001", rx_frame);
          UserRequestCollisionReset = false;
          CollisionResetStatemachine = COMPLETED_STATE;
          timeSpentCollisionReset = COMPLETED_STATE;
          cmd_state = CMD_IDLE;
          state_entered_ms = 0;
          closed_confirmed_ms = 0;
        }
      } else if (UserRequestIsolationReset) {
        uint8_t sid = rx_frame.data.u8[1];
        uint8_t sub = rx_frame.data.u8[2];
        if (sid == 0x7F && rx_frame.data.u8[3] == 0x78) {
          break;
        }
        bool is_nrc = (sid == 0x7F);

        if (IsolationResetStatemachine == 1 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1003", rx_frame);
          IsolationResetStatemachine = (sid == 0x50) ? 2 : 6;
        } else if (IsolationResetStatemachine == 3 && (sid == 0x71 || is_nrc)) {
          if (is_nrc)
            log_reset_step("DF46", rx_frame);
          IsolationResetStatemachine = (sid == 0x71) ? 4 : 6;
        } else if (IsolationResetStatemachine == 5 && (sid == 0x71 || is_nrc)) {
          if (sid == 0x71 && sub == 0x03 && rx_frame.data.u8[5] == 0x01) {
            IsolationResetStatemachine = 4;
          } else {
            log_reset_step("DF46", rx_frame);
            IsolationResetStatemachine = 6;
          }
        } else if (IsolationResetStatemachine == 7 && (sid == 0x50 || is_nrc)) {
          log_reset_step("1001", rx_frame);
          UserRequestIsolationReset = false;
          IsolationResetStatemachine = COMPLETED_STATE;
          timeSpentIsolationReset = COMPLETED_STATE;
          cmd_state = CMD_IDLE;
          state_entered_ms = 0;
          closed_confirmed_ms = 0;
        }
      }
      break;

    default:
      break;
  }
}

uint16_t EcmpV2Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  switch (pid) {
    case PID_WELD_CHECK:
      pid_welding_detection = value;
      break;
    case PID_CONT_REASON_OPEN:
      pid_reason_open = value;
      break;
    case PID_CONTACTOR_STATUS:
      pid_contactor_status = value;
      break;
    case PID_INSULATION_NEG:
      pid_insulation_res_neg = value;
      break;
    case PID_INSULATION_POS:
      pid_insulation_res_pos = value;
      break;
    case PID_INSULATION_RES:
      pid_insulation_res = value;
      break;
    case PID_WIRE_CRASH:
      pid_wire_crash = value;
      break;
    case PID_12V:
      pid_12v = value;
      break;
    case PID_HVIL_STATE:
      pid_hvil_state = value;
      break;
    case PID_BMS_STATE:
      pid_bms_state = value;
      break;
    case PID_DATE_OF_MANUFACTURE:
      pid_date_of_manufacture = value;
      break;
    case PID_SOH_PACK:
      if (value > 0 && value <= 1000)
        pid_soh_pack = value;
      break;
    case PID_SOH_MIN_CELL:
      if (value > 0 && value <= 1000)
        pid_soh_min_cell = value;
      break;
    default:
      break;
  }
  return 0;
}

void EcmpV2Battery::transmit_can(unsigned long currentMillis) {
  static bool auto_weld_reset_done = false;
  static uint8_t slice_5ms = 0;  // 0..199 (1000ms cycle — exact match to working 247ce62)
  static uint8_t sec_counter = 0;
  static unsigned long first_rx_ms = 0;

  if (datalayer_battery->status.CAN_battery_still_alive == 0) {
    first_rx_ms = 0;
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;
      transmit_can_frame(&TX_786);
      transmit_can_frame(&TX_591);
    }
    return;
  }

  if (first_rx_ms == 0) {
    first_rx_ms = currentMillis;
  }
  if (currentMillis - first_rx_ms < 600) {
    return;
  }

  if (cmd_state == CMD_IDLE && CONTACTORS_STATE == 0 && HV_BATT_REAL_VOLT_HD < 200 && HV_BATT_COP_VOLTAGE < 200 &&
      pid_welding_detection != 0 && pid_welding_detection != NOT_SAMPLED_YET && !auto_weld_reset_done &&
      !UserRequestContactorReset) {
    UserRequestContactorReset = true;
    ContactorResetStatemachine = 0;
    timeSpentContactorReset = 0;
    auto_weld_reset_done = true;
  }

  bool reset_active =
      UserRequestContactorReset || UserRequestCollisionReset || UserRequestIsolationReset || UserRequestBmsReset;
  bool want_open = (datalayer.system.status.system_status == FAULT) || reset_active;

  if (want_open) {
    if (cmd_state != CMD_IDLE) {
      cmd_state = CMD_IDLE;
      state_entered_ms = currentMillis;
      closed_confirmed_ms = 0;
    }
  } else {
    if (state_entered_ms == 0) {
      state_entered_ms = currentMillis;
    }
    unsigned long elapsed = currentMillis - state_entered_ms;
    switch (cmd_state) {
      case CMD_IDLE:
        closed_confirmed_ms = 0;
        if (elapsed >= 3000 && datalayer_battery->status.CAN_battery_still_alive > 0 && HV_BATT_REAL_VOLT_HD < 200 &&
            HV_BATT_COP_VOLTAGE < 200) {
          cmd_state = CMD_WAKE_EDGE;
          state_entered_ms = currentMillis;
        }
        break;

      case CMD_WAKE_EDGE:
        if (elapsed >= 150) {
          cmd_state = CMD_PRECHARGE_REQ;
          state_entered_ms = currentMillis;
          closed_confirmed_ms = 0;
        }
        break;

      case CMD_PRECHARGE_REQ:
        if (CONTACTORS_STATE == 2) {
          if (closed_confirmed_ms == 0) {
            closed_confirmed_ms = currentMillis;
          } else if (currentMillis - closed_confirmed_ms >= 180) {
            cmd_state = CMD_CLOSED_HOLD;
            state_entered_ms = currentMillis;
          }
        } else {
          closed_confirmed_ms = 0;
          if (elapsed >= 3000) {
            cmd_state = CMD_IDLE;
            state_entered_ms = currentMillis;
          }
        }
        break;

      case CMD_CLOSED_HOLD:
        if (CONTACTORS_STATE == 0) {
          if (elapsed >= 500) {
            cmd_state = CMD_IDLE;
            state_entered_ms = currentMillis;
          }
        } else {
          state_entered_ms = currentMillis;
        }
        break;
    }
  }

  // Never assert hv_bus_active (0x186 B2=0x01) during PRECHARGE — only in CLOSED_HOLD or IDLE turn-off weld check!
  bool hv_bus_active = (cmd_state == CMD_CLOSED_HOLD) || (cmd_state == CMD_IDLE && HV_BATT_REAL_VOLT_HD > 200);

  if (!reset_active) {
    transmit_uds_can(currentMillis);
  }

  // 250 ms UDS reset state machines (Car-TX keeps running in clean CMD_IDLE!)
  if (currentMillis - previousMillis250 >= INTERVAL_250_MS) {
    previousMillis250 = currentMillis;
    if (UserRequestContactorReset) {
      if (ContactorResetStatemachine == 0) {
        if (timeSpentContactorReset == 0) {
          reset_log = "CONT: ";
        }
        if (timeSpentContactorReset >= 4) {
          transmit_can_frame(&ECMP_DIAG_START);
          ContactorResetStatemachine = 1;
        }
      } else if (ContactorResetStatemachine == 2) {
        transmit_can_frame(&ECMP_CONTACTOR_RESET_START);
        ContactorResetStatemachine = 3;
      } else if (ContactorResetStatemachine == 4) {
        transmit_can_frame(&ECMP_CONTACTOR_RESET_PROGRESS);
        ContactorResetStatemachine = 5;
      } else if (ContactorResetStatemachine == 6) {
        transmit_can_frame(&ECMP_BREAKER_RESET_START);
        ContactorResetStatemachine = 7;
      } else if (ContactorResetStatemachine == 8) {
        transmit_can_frame(&ECMP_UV_RESET_START);
        ContactorResetStatemachine = 9;
      } else if (ContactorResetStatemachine == 10) {
        transmit_can_frame(&ECMP_UV_RESET_PROGRESS);
        ContactorResetStatemachine = 11;
      } else if (ContactorResetStatemachine == 12) {
        transmit_can_frame(&ECMP_DIAG_STOP);
        ContactorResetStatemachine = 13;
      } else if (ContactorResetStatemachine == 14) {
        transmit_can_frame(&ECMP_CLEAR_DTCS);
        ContactorResetStatemachine = 15;
      }
      if (++timeSpentContactorReset > 80) {
        reset_log += "TIMEOUT";
        UserRequestContactorReset = false;
        ContactorResetStatemachine = COMPLETED_STATE;
        timeSpentContactorReset = COMPLETED_STATE;
        cmd_state = CMD_IDLE;
        state_entered_ms = 0;
      }
    } else if (UserRequestBmsReset) {
      if (BmsResetStatemachine == 0) {
        if (timeSpentBmsReset == 0) {
          reset_log = "BMSRST: ";
        }
        if (timeSpentBmsReset >= 4) {
          transmit_can_frame(&ECMP_DIAG_START);
          BmsResetStatemachine = 1;
        }
      } else if (BmsResetStatemachine == 2) {
        transmit_can_frame(&ECMP_ECU_RESET_SOFT);
        BmsResetStatemachine = 3;
      } else if (BmsResetStatemachine == 3) {
        reset_log += "1103:SENT ";
        jdd_dtc_count = 0;
        UserRequestBmsReset = false;
        BmsResetStatemachine = COMPLETED_STATE;
        timeSpentBmsReset = COMPLETED_STATE;
        cmd_state = CMD_IDLE;
        state_entered_ms = 0;
      }
      if (++timeSpentBmsReset > 40) {
        reset_log += "TIMEOUT";
        UserRequestBmsReset = false;
        BmsResetStatemachine = COMPLETED_STATE;
        timeSpentBmsReset = COMPLETED_STATE;
        cmd_state = CMD_IDLE;
        state_entered_ms = 0;
      }
    } else if (UserRequestCollisionReset) {
      if (CollisionResetStatemachine == 0) {
        if (timeSpentCollisionReset == 0) {
          reset_log = "CRASH: ";
        }
        if (timeSpentCollisionReset >= 4) {
          transmit_can_frame(&ECMP_DIAG_START);
          CollisionResetStatemachine = 1;
        }
      } else if (CollisionResetStatemachine == 2) {
        transmit_can_frame(&ECMP_COLLISION_RESET_START);
        CollisionResetStatemachine = 3;
      } else if (CollisionResetStatemachine == 4) {
        transmit_can_frame(&ECMP_COLLISION_RESET_PROGRESS);
        CollisionResetStatemachine = 5;
      } else if (CollisionResetStatemachine == 6) {
        transmit_can_frame(&ECMP_DIAG_STOP);
        CollisionResetStatemachine = 7;
      }
      if (++timeSpentCollisionReset > 60) {
        reset_log += "TIMEOUT";
        UserRequestCollisionReset = false;
        CollisionResetStatemachine = COMPLETED_STATE;
        timeSpentCollisionReset = COMPLETED_STATE;
        cmd_state = CMD_IDLE;
        state_entered_ms = 0;
      }
    } else if (UserRequestIsolationReset) {
      if (IsolationResetStatemachine == 0) {
        if (timeSpentIsolationReset == 0) {
          reset_log = "ISO: ";
        }
        if (timeSpentIsolationReset >= 4) {
          transmit_can_frame(&ECMP_DIAG_START);
          IsolationResetStatemachine = 1;
        }
      } else if (IsolationResetStatemachine == 2) {
        transmit_can_frame(&ECMP_ISOLATION_RESET_START);
        IsolationResetStatemachine = 3;
      } else if (IsolationResetStatemachine == 4) {
        transmit_can_frame(&ECMP_ISOLATION_RESET_PROGRESS);
        IsolationResetStatemachine = 5;
      } else if (IsolationResetStatemachine == 6) {
        transmit_can_frame(&ECMP_DIAG_STOP);
        IsolationResetStatemachine = 7;
      }
      if (++timeSpentIsolationReset > 60) {
        reset_log += "TIMEOUT";
        UserRequestIsolationReset = false;
        IsolationResetStatemachine = COMPLETED_STATE;
        timeSpentIsolationReset = COMPLETED_STATE;
        cmd_state = CMD_IDLE;
        state_entered_ms = 0;
      }
    }
  }

  // Keep bus quiet for 50ms whenever 0x694 is streaming a multi-frame ISO-TP response (e.g. Read DTC 59 02!)
  if (currentMillis < isotp_quiet_until_ms) {
    return;
  }

  // Exact working 247ce62 5 ms Micro-Slice Scheduler (max 3-4 frames per 5ms tick!)
  if (currentMillis - previousMillis10 >= 5) {
    previousMillis10 = currentMillis;
    slice_5ms = (slice_5ms + 1) % 200;

    if ((slice_5ms & 1) == 0) {
      counter_10ms = (counter_10ms + 1) & 0x0F;
      if (hv_bus_active) {
        TX_186.data = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x80};
        TX_1F8.data = {0x00, 0x00, 0xA0, 0x14, 0x7F, 0xFC, 0x01, 0x00};
      } else if (cmd_state == CMD_PRECHARGE_REQ &&
                 (CONTACTORS_STATE >= 1 || (currentMillis - state_entered_ms) >= 80)) {
        TX_186.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43, 0x80};
        TX_1F8.data = {0x00, 0xA2, 0x94, 0x50, 0x7F, 0xFC, 0x01, 0x00};
      } else {
        TX_186.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        TX_1F8.data = {0x00, 0xA2, 0x94, 0x50, 0x7F, 0xFC, 0x01, 0x00};
      }
      apply_psa_checksum(TX_1F8, 0, counter_10ms, false);
      apply_psa_checksum(TX_186, 0, counter_10ms, false);
      transmit_can_frame(&TX_1F8);
      transmit_can_frame(&TX_186);

      if ((slice_5ms % 4) == 2) {
        counter_20ms = (counter_20ms + 1) & 0x0F;
        apply_psa_checksum(TX_12D, 0, counter_20ms, true);
        transmit_can_frame(&TX_12D);
        transmit_can_frame(&TX_2E2);
      } else if ((slice_5ms % 8) == 0) {
        counter_40ms = (counter_40ms + 1) & 0x0F;
        if (cmd_state == CMD_IDLE) {
          TX_342.data = {0x00, 0x00, 0x01, 0x28, 0x00, 0x00, 0x00, 0x80};
        } else if (cmd_state == CMD_WAKE_EDGE || cmd_state == CMD_PRECHARGE_REQ) {
          TX_342.data = {0x00, 0x00, 0x41, 0x28, 0x00, 0x00, 0x00, 0x80};
        } else {
          TX_342.data = {0x00, 0x64, 0x41, 0x28, 0x00, 0x00, 0x00, 0x80};
        }
        apply_psa_checksum(TX_342, 0, counter_40ms, false);
        transmit_can_frame(&TX_342);
      } else if ((slice_5ms % 8) == 4) {
        apply_psa_checksum(TX_428, 0, counter_40ms, false);
        transmit_can_frame(&TX_428);
      }
    } else {
      apply_psa_checksum(TX_17B, 7, counter_10ms, true);
      transmit_can_frame(&TX_17B);

      if ((slice_5ms % 10) == 5) {
        if (cmd_state == CMD_IDLE) {
          TX_27A.data = {0x4E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        } else if (cmd_state == CMD_WAKE_EDGE) {
          if ((currentMillis - state_entered_ms) < 75) {
            TX_27A.data = {0x8E, 0x14, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00};
          } else {
            TX_27A.data = {0x8E, 0x34, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00};
          }
        } else if (cmd_state == CMD_PRECHARGE_REQ) {
          TX_27A.data = {0x8E, 0x34, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00};
        } else {
          TX_27A.data = {0x4F, 0x58, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00};
        }
        transmit_can_frame(&TX_27A);
      }

      uint8_t odd_slot = (slice_5ms / 2) % 10;
      switch (odd_slot) {
        case 0:
          counter_100ms = (counter_100ms + 1) & 0x0F;
          if (cmd_state == CMD_IDLE || cmd_state == CMD_WAKE_EDGE) {
            TX_448.data.u8[1] = 0x00;
          } else if (cmd_state == CMD_PRECHARGE_REQ) {
            TX_448.data.u8[1] = 0x60;
          } else {
            TX_448.data.u8[1] = 0x20;
          }
          apply_psa_checksum(TX_448, 0, counter_100ms, false);
          transmit_can_frame(&TX_448);
          transmit_can_frame(&TX_0A6);
          break;
        case 1:
          TX_398.data.u8[1] = hv_bus_active ? 0x1E : 0x00;
          if (cmd_state == CMD_CLOSED_HOLD) {
            TX_281.data = {0x00, 0x5B, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00};
          } else if (cmd_state == CMD_PRECHARGE_REQ) {
            TX_281.data = {0x00, 0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          } else {
            TX_281.data = {0x00, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          }
          apply_psa_checksum(TX_398, 0, counter_100ms, false);
          apply_psa_checksum(TX_281, 0, counter_100ms, false);
          transmit_can_frame(&TX_398);
          transmit_can_frame(&TX_281);
          break;
        case 2:
          TX_422.data.u8[5] = (cmd_state == CMD_CLOSED_HOLD) ? 0x10 : 0x00;
          apply_psa_checksum(TX_422, 0, counter_100ms, false);
          apply_psa_checksum(TX_31D, 7, counter_100ms, true);
          transmit_can_frame(&TX_422);
          transmit_can_frame(&TX_31D);
          break;
        case 3:
          apply_psa_checksum(TX_441, 0, counter_100ms, false);
          apply_psa_checksum(TX_458, 0, counter_100ms, false);
          transmit_can_frame(&TX_441);
          transmit_can_frame(&TX_458);
          break;
        case 4:
          TX_4F1.data.u8[5] = hv_bus_active ? 0x20 : 0x00;
          apply_psa_checksum(TX_4D1, 0, counter_100ms, false);
          apply_psa_checksum(TX_4F1, 0, counter_100ms, false);
          transmit_can_frame(&TX_4D1);
          transmit_can_frame(&TX_4F1);
          break;
        case 5:
          TX_526.data.u8[3] = hv_bus_active ? 0x06 : 0x00;
          transmit_can_frame(&TX_526);
          transmit_can_frame(&TX_382);
          break;
        case 6:
          if (cmd_state == CMD_CLOSED_HOLD) {
            TX_541.data = {0x00, 0x39, 0x9C, 0x07, 0xD8, 0x9C, 0x40, 0x00};
          } else {
            TX_541.data = {0x00, 0x00, 0x54, 0x00, 0x10, 0x9C, 0x40, 0x00};
          }
          apply_psa_checksum(TX_541, 0, counter_100ms, false);
          apply_psa_checksum(TX_31E, 7, counter_100ms, true);
          transmit_can_frame(&TX_541);
          transmit_can_frame(&TX_31E);
          break;
        case 7:
          TX_3E2.data.u8[4] = (cmd_state == CMD_IDLE) ? 0x01 : 0x24;
          transmit_can_frame(&TX_3E2);
          transmit_can_frame(&TX_482);
          break;
        case 8:
          if (cmd_state == CMD_CLOSED_HOLD) {
            TX_501.data = {0x00, 0x19, 0xE0, 0x4C, 0xFA, 0x00, 0x00, 0x41};
          } else {
            TX_501.data = {0x00, 0x00, 0x10, 0x00, 0xFA, 0x00, 0x00, 0x41};
          }
          TX_586.data.u8[5] = hv_bus_active ? 0x80 : 0x00;
          apply_psa_checksum(TX_501, 0, counter_100ms, false);
          apply_psa_checksum(TX_586, 0, counter_100ms, false);
          transmit_can_frame(&TX_501);
          transmit_can_frame(&TX_586);
          break;
        case 9:
          if (slice_5ms == 19 || slice_5ms == 119) {
            counter_500ms = (counter_500ms + 1) & 0x0F;
            apply_psa_checksum(TX_531, 0, counter_500ms, false);
            transmit_can_frame(&TX_531);
            transmit_can_frame(&TX_0AE);
          } else if (slice_5ms == 59) {
            counter_1000ms = (counter_1000ms + 1) & 0x0F;
            TX_402.data.u8[4] = 0xE6;
            TX_402.data.u8[6] = (cmd_state == CMD_CLOSED_HOLD) ? 0x22 : 0x00;
            transmit_can_frame(&TX_402);
            transmit_can_frame(&TX_551);
          } else if (slice_5ms == 99) {
            apply_psa_checksum(TX_5A6, 3, counter_1000ms, false);
            transmit_can_frame(&TX_591);
            transmit_can_frame(&TX_5A6);
          } else if (slice_5ms == 159) {
            ticks_552 += 10;
            TX_552.data.u8[0] = (ticks_552 >> 24) & 0xFF;
            TX_552.data.u8[1] = (ticks_552 >> 16) & 0xFF;
            TX_552.data.u8[2] = (ticks_552 >> 8) & 0xFF;
            TX_552.data.u8[3] = ticks_552 & 0xFF;
            transmit_can_frame(&TX_552);
            transmit_can_frame(&TX_5C6);
          } else if (slice_5ms == 199) {
            transmit_can_frame(&TX_786);
            if (++sec_counter >= 5) {
              sec_counter = 0;
              transmit_can_frame(&TX_55F);
            }
          }
          break;
      }
    }
  }
}

void EcmpV2Battery::setup(void) {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = DEFAULT_NUMBER_OF_CELLS;
  datalayer_battery->info.total_capacity_Wh = 54000;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  setup_uds(0x6B4, 0x694);
  static const uint16_t pid_scan_list[] = {
      PID_WELD_CHECK,     PID_CONT_REASON_OPEN, PID_CONTACTOR_STATUS,    PID_INSULATION_NEG,
      PID_INSULATION_POS, PID_INSULATION_RES,   PID_WIRE_CRASH,          PID_12V,
      PID_HVIL_STATE,     PID_BMS_STATE,        PID_DATE_OF_MANUFACTURE, PID_SOH_PACK,
      PID_SOH_MIN_CELL,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}

#endif
