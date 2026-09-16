#include "ECMP-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For More Battery Info page
#include "../devboard/utils/events.h"

/* TODO:
This integration is still ongoing. The same integration can be used on multiple variants of the Stellantis platforms
- eCMP: Disable the isolation resistance requirement that opens contactors after 30s under load. Factory mode?

- MysteryVan: Map more values from constantly transmitted instead of PID
- ADD CAN sending towards the battery (CAN-logs of full vehicle wanted!)
  - Following CAN messages need to be sent towards it:
  - VCU: 4C9 , 565 , 398, 448, 458, 4F1 , 342,  3E2 , 402 , 422 , 482 4D1
  - CMM: 478 , 558, 1A8, 4B8 1F8 498 4E8
  - OBC: 531 441 541 551 3C1
  - BSIInfo_382
  - VCU_BSI_Wakeup_27A
  - V2_BSI_552
  - CRASH_4C8
  - EVSE plug in (optional): 108, 109, 119
  - CRASH_4C8
  - CRNT_SENS_095
  - MCU 526
  - JDD 55F NEW

- STLA medium: Everything missing
- ADD CAN sending towards the battery (CAN-logs of full vehicle wanted!)
*/

/* Do not change code below unless you are sure what you are doing */
void EcmpBattery::update_values() {

  if (!MysteryVan) {  //Normal eCMP platform
    datalayer_battery->status.real_soc = battery_soc * 10;

    //datalayer_battery->status.soh_pptt; //TODO: Find SOH%

    datalayer_battery->status.voltage_dV = battery_voltage * 10;

    // If High Precision Curent is avilable, use it
    if (pid_current != NOT_SAMPLED_YET && datalayer.system.status.system_status != FAULT) {
      datalayer_battery->status.current_dA = (int16_t)(pid_current / 100);
    } else {  //Low precision
      datalayer_battery->status.current_dA = -(battery_current * 10);
    }

    datalayer_battery->status.active_power_W =  //Power in watts, Negative = charging batt
        ((datalayer_battery->status.voltage_dV * datalayer_battery->status.current_dA) / 100);

    datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
        (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

    datalayer_battery->status.max_charge_power_W = battery_AllowedMaxChargeCurrent * battery_voltage;

    datalayer_battery->status.max_discharge_power_W = battery_AllowedMaxDischargeCurrent * battery_voltage;

    datalayer_battery->status.temperature_min_dC = battery_lowestTemperature * 10;

    datalayer_battery->status.temperature_max_dC = battery_highestTemperature * 10;

    // Initialize min and max, lets find which cells are min and max!
    uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
    uint16_t max_cell_mv_value = 0;
    // Loop to find the min and max while ignoring zero values
    for (uint8_t i = 0; i < datalayer_battery->info.number_of_cells; ++i) {
      uint16_t voltage_mV = datalayer_battery->status.cell_voltages_mV[i];
      if (voltage_mV != 0) {  // Skip unread values (0)
        min_cell_mv_value = std::min(min_cell_mv_value, voltage_mV);
        max_cell_mv_value = std::max(max_cell_mv_value, voltage_mV);
      }
    }
    // If all array values are 0, reset min/max to 3700
    if (min_cell_mv_value == std::numeric_limits<uint16_t>::max()) {
      min_cell_mv_value = 3700;
      max_cell_mv_value = 3700;
    }

    datalayer_battery->status.cell_min_voltage_mV = min_cell_mv_value;
    datalayer_battery->status.cell_max_voltage_mV = max_cell_mv_value;
  } else {  //Some variant of the 50/75kWh battery that is not using the eCMP CAN mappings.
    // For these batteries we need to use the OBD2 PID polled values

    if (pid_energy_capacity != NOT_SAMPLED_YET) {
      datalayer_battery->status.remaining_capacity_Wh = pid_energy_capacity;
      // calculate SOC based on datalayer.battery.info.total_capacity_Wh and remaining_capacity_Wh
      datalayer_battery->status.real_soc = (uint16_t)(((float)datalayer_battery->status.remaining_capacity_Wh /
                                                       datalayer_battery->info.total_capacity_Wh) *
                                                      10000);
    }

    //datalayer.battery.status.soh_pptt; //TODO: Find SOH%

    if (pid_pack_voltage != NOT_SAMPLED_YET) {
      datalayer_battery->status.voltage_dV = pid_pack_voltage + 800;
    }

    if (pid_max_charge_10s != NOT_SAMPLED_YET) {
      datalayer_battery->status.max_charge_power_W = pid_max_charge_10s;
    }

    if (pid_max_discharge_10s != NOT_SAMPLED_YET) {
      datalayer_battery->status.max_discharge_power_W = pid_max_discharge_10s;
    }

    if ((pid_highest_temperature != 127) && (pid_lowest_temperature != 127)) {
      datalayer_battery->status.temperature_max_dC = pid_highest_temperature * 10;
      datalayer_battery->status.temperature_min_dC = pid_lowest_temperature * 10;
    }

    if ((pid_high_cell_voltage != NOT_SAMPLED_YET) && (pid_low_cell_voltage != NOT_SAMPLED_YET)) {
      datalayer_battery->status.cell_max_voltage_mV = pid_high_cell_voltage;
      datalayer_battery->status.cell_min_voltage_mV = pid_low_cell_voltage;
    }

    datalayer_battery->info.number_of_cells = NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY;  //50/75kWh sends valid cellcount
  }

  if (battery_InterlockOpen) {
    set_event(EVENT_HVIL_FAILURE, 0, battery_index);
  } else {
    clear_event(EVENT_HVIL_FAILURE, battery_index);
  }

  if (pid_12v < 11000) {
    set_event(EVENT_12V_LOW, 11, battery_index);
  } else {
    clear_event(EVENT_12V_LOW, battery_index);
  }

  if (pid_reason_open == 7) {  //Invalid status
    set_event(EVENT_CONTACTOR_OPEN, 0, battery_index);
  } else {
    clear_event(EVENT_CONTACTOR_OPEN, battery_index);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}
String EcmpBattery::get_uds_info_html() {
  String content;
  content.reserve(8000);

  // Helper macros to reduce repetition for common patterns
#define H4_COND(label, val, na_val, unit) \
  content << "<h4>" << label << ": " << (val == na_val ? "N/A" : String(val)) << unit << "</h4>"

#define H4_STR(label, val) content << "<h4>" << label << ": " << val << "</h4>"

#define H4_BOOL(label, flag) content << "<h4>" << label << ": " << (flag ? "Yes" : "No") << "</h4>"

#define H4_ARRAY(label, arr) content << "<h4>" << label << ": " << String((const char*)arr) << "</h4>"

  // clang-format off
 // Main Connector State
    content << "<h4>Main Connector State: ";
    switch (battery_MainConnectorState) {
      case 0:   content << "Contactors open"; break;
      case 0x01: content << "Precharged"; break;
      default:  content << "Invalid";
    }
    content << "</h4>";

    // Interlock
    content << "<h4>Interlock:  " 
            << (battery_InterlockOpen ? "BROKEN!" : "Seated OK")
            << "</h4>";

    // Insulation Diag
    H4_COND("Insulation Resistance", 
            battery_insulationResistanceKOhm, 255, "kOhm");
    content << "<h4>Insulation Diag: ";
    switch (battery_insulation_failure_diag) {
      case 0: content << "No failure"; break;
      case 1: content << "Symmetric failure"; break;
      default: content << "N/A";
    }
    content << "</h4>";

     // Contactor weld check
    content << "<h4>Contactor weld check: ";
    switch (pid_welding_detection) {
      case 0:   content << "OK"; break;
      case 255: content << "N/A"; break;
      default:  content << "WELDED!" << String(pid_welding_detection);
    }
    content << "</h4>";

    // Contactor opening reason
    content << "<h4>Contactor opening reason: ";
    switch (pid_reason_open) {
      case 7:   content << "Invalid Status"; break;
      case 255: content << "N/A"; break;
      default:  content << "Unknown" << String(pid_reason_open);
    }
    content << "</h4>";

    // Contactor/Switch states
    H4_COND("Status of power switch", 
            pid_contactor_status, 255, "");
    H4_COND("Negative power switch control", 
            pid_negative_contactor_control, 255, "");
    H4_COND("Negative power switch status", 
            pid_negative_contactor_status, 255, "");
    H4_COND("Positive power switch control", 
            pid_positive_contactor_control, 255, "");
    H4_COND("Positive power switch status", 
            pid_positive_contactor_status, 255, "");
    H4_COND("Contactor negative", 
            pid_contactor_negative, 255, "");
    H4_COND("Contactor positive", 
            pid_contactor_positive, 255, "");
    H4_COND("Precharge control", 
            pid_precharge_relay_control, 255, "");
    H4_COND("Precharge status", 
            pid_precharge_relay_status, 255, "");
    H4_COND("Recharge Status", 
            pid_recharge_status, 255, "");

    // Temperature readings
    H4_COND("Delta temperature", 
            pid_delta_temperature, 127, "&deg;C");
    H4_COND("Lowest temperature", 
            pid_lowest_temperature, 127, "&deg;C");
    H4_COND("Average temperature", 
            pid_average_temperature, 127, "&deg;C");
    H4_COND("Highest temperature", 
            pid_highest_temperature, 127, "&deg;C");
    H4_COND("Coldest module", 
            pid_coldest_module, 255, "");
    H4_COND("Hottest module", 
            pid_hottest_module, 255, "");

    // Voltage and current
    H4_COND("Average cell voltage", 
            pid_avg_cell_voltage, 255, " mV");
    H4_COND("High precision current", 
            pid_current, 255, " mA");

    // Insulation resistance
    H4_COND("Insulation resistance neg-gnd", 
            pid_insulation_res_neg, 255, " kOhm");
    H4_COND("Insulation resistance pos-gnd", 
            pid_insulation_res_pos, 255, " kOhm");

    // Power limits
    H4_COND("Max current 10s", 
            pid_max_current_10s, 255, "");
    H4_COND("Max discharge power 10s", 
            pid_max_discharge_10s, 255, "");
    H4_COND("Max discharge power 30s", 
            pid_max_discharge_30s, 255, "");
    H4_COND("Max charge power 10s", 
            pid_max_charge_10s, 255, "");
    H4_COND("Max charge power 30s", 
            pid_max_charge_30s, 255, "");

    // Energy and capacity info
    H4_COND("Energy capacity", 
            pid_energy_capacity, 255, "");
    H4_COND("Highest cell number", 
            pid_highest_cell_voltage_num, 255, "");
    H4_COND("Lowest cell voltage number", 
            pid_lowest_cell_voltage_num, 255, "");
    H4_COND("Sum of all cell voltages", 
            pid_sum_of_cells, 255, " dV");
    H4_COND("Cell min capacity", 
            pid_cell_min_capacity, 255, "");
    H4_COND("Cell voltage measurement status", 
            pid_cell_voltage_measurement_status, 255, "");

    // Battery resistance and voltage
    H4_COND("Battery Insulation Resistance", 
            pid_insulation_res, 255, " kOhm");
    H4_COND("Pack voltage", 
            pid_pack_voltage, 255, " dV");
    H4_COND("Highest cell voltage", 
            pid_high_cell_voltage, 255, " mV");
    H4_COND("Lowest cell voltage", 
            pid_low_cell_voltage, 255, " mV");

    // Battery energy and crash data
    H4_COND("Battery Energy", 
            pid_battery_energy, 255, "");
    H4_COND("Collision information Counter", 
            pid_crash_counter, 255, "");
    H4_COND("Collision Counter received by Wire", 
            pid_wire_crash, 255, "");
    H4_COND("Collision data sent from car to battery", 
            pid_CAN_crash, 255, "");

    // History and counters
    H4_COND("Low SOC counter", 
            pid_lowsoc_counter, 255, "");
    H4_COND("Last CAN failure detail", 
            pid_last_can_failure_detail, 255, "");

    // Version and configuration
    H4_ARRAY("HW version number", pid_hw_version_num);
    H4_ARRAY("SW version number", pid_sw_version_num);
    H4_COND("Factory mode", 
            pid_factory_mode_control, 255, "");

    // Serial number and date
    {
      char readableSerialNumber[14];  // One extra space for null terminator
      memcpy(readableSerialNumber, pid_battery_serial,
             sizeof(pid_battery_serial));
      readableSerialNumber[13] = '\0';  // Null terminate the string
      H4_STR("Battery serial", String(readableSerialNumber));
    }

    // Date of manufacture
    {
      uint8_t day = (pid_date_of_manufacture >> 16) & 0xFF;
      uint8_t month = (pid_date_of_manufacture >> 8) & 0xFF;
      uint8_t year = pid_date_of_manufacture & 0xFF;
      content << "<h4>Date of manufacture: " << String(day) << "/" 
              << String(month) << "/" << String(year) << "</h4>";
    }

    // Fuses and safety states
    H4_COND("Aux fuse state", 
            pid_aux_fuse_state, 255, "");
    H4_COND("Battery state", 
            pid_battery_state, 255, "");
    H4_COND("Precharge short circuit", 
            pid_precharge_short_circuit, 255, "");
    H4_COND("Service plug state", 
            pid_eservice_plug_state, 255, "");
    H4_COND("Main fuse state", 
            pid_mainfuse_state, 255, "");
    H4_COND("Most critical fault", 
            pid_most_critical_fault, 255, "");

    // Timing
    H4_COND("Current time", 
            pid_current_time, 255, " ticks");
    H4_COND("Time sent by car", 
            pid_time_sent_by_car, 255, " ticks");

    // Supply voltage
    H4_COND("12V", 
            pid_12v, 255, "");

    // 12V abnormal status
    content << "<h4>12V abnormal: ";
    if (pid_12v_abnormal == 255) {
      content << "N/A";
    } else if (pid_12v_abnormal == 0) {
      content << "No";
    } else {
      content << "Yes";
    }
    content << "</h4>";

    // HVIL voltages
    H4_COND("HVIL IN Voltage", 
            pid_hvil_in_voltage, 255, "mV");
    H4_COND("HVIL output voltage", 
            pid_hvil_out_voltage, 255, "mV");

    // HVIL State
    content << "<h4>HVIL State: ";
    if (pid_hvil_state == 255) {
      content << "N/A";
    } else if (pid_hvil_state == 0) {
      content << "OK";
    } else {
      content << String(pid_hvil_state);
    }
    content << "</h4>";

    // BMS State
    content << "<h4>BMS State: ";
    if (pid_bms_state == 255) {
      content << "N/A";
    } else if (pid_bms_state == 0) {
      content << "OK";
    } else {
      content << String(pid_bms_state);
    }
    content << "</h4>";

    // Vehicle and operational data
    H4_COND("Vehicle speed", 
            pid_vehicle_speed, 255, " km/h");
    H4_COND("Time spent over 55c", 
            pid_time_spent_over_55c, 255, " minutes");
    H4_COND("Contactor lifetime closing counter", 
            pid_contactor_closing_counter, 255, " cycles");
    H4_COND("State of Health Cell-1", 
            pid_SOH_cell_1, 255, "");

    // ============================================================================
    // MysteryVan platform section (All parameters in ALLCAPS)
    // ============================================================================
    if (MysteryVan) {
      content << "<h3>MysteryVan platform detected!</h3>";

      // Contactor State
      content << "<h4>Contactor State: ";
      switch (CONTACTORS_STATE) {
        case 0: content << "Open"; break;
        case 1: content << "Precharge"; break;
        case 2: content << "Closed"; break;
        default: content << "Unknown";
      }
      content << "</h4>";

      // Crash Memorized
      H4_BOOL("Crash Memorized", HV_BATT_CRASH_MEMORIZED);

      // Contactor Opening Reason
      content << "<h4>Contactor Opening Reason: ";
      switch (CONTACTOR_OPENING_REASON) {
        case 0: content << "No error"; break;
        case 1: content << "Crash!"; break;
        case 2: content << "12V supply source undervoltage"; break;
        case 3: content << "12V supply source overvoltage"; break;
        case 4: content << "Battery temperature"; break;
        case 5: content << "Interlock line open"; break;
        case 6: content << "e-Service plug disconnected"; break;
        default: content << "Unknown";
      }
      content << "</h4>";

      // Battery fault type
      content << "<h4>Battery fault type: ";
      switch (TBMU_FAULT_TYPE) {
        case 0: content << "No fault"; break;
        case 1: content << "FirstLevelFault: Warning Lamp"; break;
        case 2: content << "SecondLevelFault: Stop Lamp"; break;
        case 3: content << "ThirdLevelFault: Stop Lamp + contactor opening (EPS shutdown)"; break;
        case 4: content << "FourthLevelFault: Stop Lamp + Active Discharge"; break;
        case 5: content << "Inhibition of powertrain activation"; break;
        case 6: content << "Reserved"; break;
        default: content << "Unknown";
      }
      content << "</h4>";

      // FC insulation resistance values
      H4_COND("FC insulation minus resistance", 
              HV_BATT_FC_INSU_MINUS_RES, 0, " kOhm");
      H4_COND("FC insulation plus resistance", 
              HV_BATT_FC_INSU_PLUS_RES, 0, " kOhm");
      H4_COND("FC vehicle insulation plus resistance", 
              HV_BATT_FC_VHL_INSU_PLUS_RES, 0, " kOhm");
      H4_COND("FC vehicle insulation minus resistance", 
              HV_BATT_ONLY_INSU_MINUS_RES, 0, " kOhm");
    }

    // ============================================================================
    // Alert flags section
    // ============================================================================
    H4_BOOL("Alert Battery", ALERT_BATT);
    H4_BOOL("Alert Low SOC", ALERT_LOW_SOC);
    H4_BOOL("Alert High SOC", ALERT_HIGH_SOC);
    H4_BOOL("Alert SOC Jump", ALERT_SOC_JUMP);
    H4_BOOL("Alert Overcharge", ALERT_OVERCHARGE);
    H4_BOOL("Alert Temp Diff", ALERT_TEMP_DIFF);
    H4_BOOL("Alert Temp High", ALERT_HIGH_TEMP);
    H4_BOOL("Alert Overvoltage", ALERT_OVERVOLTAGE);
    H4_BOOL("Alert Cell Overvoltage", ALERT_CELL_OVERVOLTAGE);
    H4_BOOL("Alert Cell Undervoltage", ALERT_CELL_UNDERVOLTAGE);
    H4_BOOL("Alert Cell Poor Consistency", ALERT_CELL_POOR_CONSIST);

    // Footer message
    content << "<h4>Remember to press Open Contactors from main menu before running the diagnostic commands below:</h4>";

    // Clean up macros
    #undef H4_COND
    #undef H4_STR
    #undef H4_BOOL

    return content;
}

void EcmpBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  // UDS frames (0x7EC PID/DTC replies) are handled by the superclass.
  // Only exception is if userRequested functionality is requested, then we need to handle the UDS frame ourselves and not let the superclass handle it.
  if (UserRequestContactorReset || UserRequestCollisionReset || UserRequestIsolationReset) {

  } else {
    if (handle_incoming_uds_can_frame(rx_frame)) {
      return;
    }
  }

  switch (rx_frame.ID) {
    case 0x2D4:  //MysteryVan 50/75kWh platform (TBMU 100ms periodic)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      MysteryVan = true;
      SOE_MAX_CURRENT_TEMP = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];                       // (Wh, 0-200000)
      FRONT_MACHINE_POWER_LIMIT = (rx_frame.data.u8[4] << 6) | ((rx_frame.data.u8[5] & 0xFC) >> 2);  // (W 0-1000000)
      REAR_MACHINE_POWER_LIMIT = ((rx_frame.data.u8[5] & 0x03) << 12) | (rx_frame.data.u8[6] << 4) |
                                 ((rx_frame.data.u8[7] & 0xF0) >> 4);  // (W 0-1000000)
      break;
    case 0x3B4:  //MysteryVan 50/75kWh platform (TBMU 100ms periodic)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      EVSE_INSTANT_DC_HV_CURRENT =
          ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 2) | ((rx_frame.data.u8[4] & 0xC0) >> 6);
      EVSE_STATE = ((rx_frame.data.u8[4] & 0x38) >> 3); /*Enumeration below
      000: NOT CONNECTED 
      001: CONNECTED 
      010: INITIALISATION 
      011: READY 
      100: PRECHARGE IN PROGRESS 
      101: TRANSFER IN PROGRESS 
      110: NOT READY
      111: Reserved */
      HV_BATT_SOE_HD = ((rx_frame.data.u8[4] & 0x03) << 12) | (rx_frame.data.u8[5] << 4) |
                       ((rx_frame.data.u8[6] & 0xF0) >> 4);                         // (Wh, 0-200000)
      HV_BATT_SOE_MAX = ((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7];  // (Wh, 0-200000)
      CHECKSUM_FRAME_3B4 = (rx_frame.data.u8[0] & 0xF0) >> 4;
      //COUNTER_3B4 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x2F4:  //MysteryVan 50/75kWh platform (Event triggered when charging)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //TBMU_EVSE_DC_MES_VOLTAGE = (rx_frame.data.u8[0] << 6) | (rx_frame.data.u8[1] >> 2);           //V 0-1000 //Fastcharger info, not needed for BE
      //TBMU_EVSE_DC_MIN_VOLTAGE = ((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[2];         //V 0-1000 //Fastcharger info, not needed for BE
      //TBMU_EVSE_DC_MES_CURRENT = (rx_frame.data.u8[3] << 4) | ((rx_frame.data.u8[4] & 0xF0) >> 4);  //A -2000 - 2000 //Fastcharger info, not needed for BE
      //TBMU_EVSE_CHRG_REQ = (rx_frame.data.u8[4] & 0x0C) >> 2;  //00 No request, 01 Stop request //Fastcharger info, not needed for BE
      //HV_STORAGE_MAX_I = ((rx_frame.data.u8[4] & 0x03) << 12) | (rx_frame.data.u8[5] << 2) | //Fastcharger info, not needed for BE
      //((rx_frame.data.u8[6] & 0xC0) >> 6);  //A -2000 - 2000
      //TBMU_EVSE_DC_MAX_POWER = ((rx_frame.data.u8[6] & 0x3F) << 8) | rx_frame.data.u8[7];  //W -1000000 - 0 //Fastcharger info, not needed for BE
      break;
    case 0x3F4:  //MysteryVan 50/75kWh platform (Temperature sensors)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      switch (((rx_frame.data.u8[0] & 0xE0) >> 5))  //Mux resides in top 3 bits of frame0
      {
        case 0:
          BMS_PROBETEMP[0] = (rx_frame.data.u8[1] - 40);
          BMS_PROBETEMP[1] = (rx_frame.data.u8[2] - 40);
          BMS_PROBETEMP[2] = (rx_frame.data.u8[3] - 40);
          BMS_PROBETEMP[3] = (rx_frame.data.u8[4] - 40);
          BMS_PROBETEMP[4] = (rx_frame.data.u8[5] - 40);
          BMS_PROBETEMP[5] = (rx_frame.data.u8[6] - 40);
          BMS_PROBETEMP[6] = (rx_frame.data.u8[7] - 40);
          break;
        default:  //There are in total 64 temperature measurements in the BMS. We do not need to sample them all.
          break;  //For future, we could read them all if we want to.
      }
      break;
    case 0x554:  //MysteryVan 50/75kWh platform (Discharge/Charge limits)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_PEAK_DISCH_POWER_HD = (rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[2] >> 2);  //0-1000000 W
      HV_BATT_PEAK_CH_POWER_HD = ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) |
                                 ((rx_frame.data.u8[4] & 0xF0) >> 4);  // -1000000 - 0 W
      HV_BATT_NOM_CH_POWER_HD = ((rx_frame.data.u8[4] & 0x0F) << 12) | (rx_frame.data.u8[5] << 6) |
                                ((rx_frame.data.u8[6] & 0xC0) >> 6);  // -1000000 - 0 W
      MAX_ALLOW_CHRG_CURRENT = ((rx_frame.data.u8[6] & 0x3F) << 8) | rx_frame.data.u8[7];
      CHECKSUM_FRAME_554 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0xE
      //COUNTER_554 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x373:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      REQ_CLEAR_DTC_TBMU = ((rx_frame.data.u8[3] & 0x40) >> 7);
      TBCU_48V_WAKEUP = (rx_frame.data.u8[3] >> 7);
      HV_BATT_MAX_REAL_CURR = (rx_frame.data.u8[5] << 7) | (rx_frame.data.u8[6] >> 1);  //A	-2000 -	2000	0.1 scaling
      TBMU_FAULT_TYPE = (rx_frame.data.u8[7] & 0xE0) >> 5;
      /*000: No fault
        001: FirstLevelFault: Warning Lamp
        010: SecondLevelFault: Stop Lamp
        011: ThirdLevelFault: Stop Lamp + contactor opening (EPS shutdown) 
        100: FourthLevelFault: Stop Lamp + Active Discharge
        101: Inhibition of powertrain activation
        110: Reserved 
        111: Invalid*/
      HV_BATT_REAL_VOLT_HD = ((rx_frame.data.u8[3] & 0x3F) << 8) | (rx_frame.data.u8[4]);  //V 0-1000 * 0.1  scaling
      HV_BATT_REAL_CURR_HD = (rx_frame.data.u8[1] << 8) | (rx_frame.data.u8[2]);           //A	-2000 -	2000	0.1 scaling
      CHECKSUM_FRAME_373 = (rx_frame.data.u8[0] & 0xF0) >> 4;                              //Frame checksum 0xD
      //COUNTER_373 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x4F4:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_CRASH_MEMORIZED = ((rx_frame.data.u8[2] & 0x08) >> 3);
      HV_BATT_COLD_CRANK_ACK = ((rx_frame.data.u8[2] & 0x04) >> 2);
      HV_BATT_CHARGE_NEEDED_STATE = ((rx_frame.data.u8[2] & 0x02) >> 1);
      HV_BATT_NOM_CH_VOLTAGE = ((rx_frame.data.u8[2] & 0x01) << 8) | (rx_frame.data.u8[3]);   //V 0 - 500
      HV_BATT_NOM_CH_CURRENT = rx_frame.data.u8[4];                                           // -120 - 0	 0.5scaling
      HV_BATT_GENERATED_HEAT_RATE = (rx_frame.data.u8[5] << 1) | (rx_frame.data.u8[6] >> 7);  //W 0-50000
      REQ_MIL_LAMP_CONTINOUS = (rx_frame.data.u8[7] & 0x04) >> 2;
      REQ_BLINK_STOP_AND_SERVICE_LAMP = (rx_frame.data.u8[7] & 0x02) >> 1;
      CMD_RESET_MIL = (rx_frame.data.u8[7] & 0x01);
      HV_BATT_SOC = (rx_frame.data.u8[1] << 2) | (rx_frame.data.u8[2] >> 6);
      CONTACTORS_STATE =
          (rx_frame.data.u8[2] & 0x30) >> 4;  //00 : contactor open 01 : pre-load contactor 10 : contactor close
      HV_BATT_DISCONT_WARNING_OPEN = (rx_frame.data.u8[7] & 0x08) >> 3;
      CHECKSUM_FRAME_4F4 = (rx_frame.data.u8[0] & 0xF0) >> 4;
      //COUNTER_4F4 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x414:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_REAL_POWER_HD = (rx_frame.data.u8[1] << 7) | (rx_frame.data.u8[2] >> 1);
      MAX_ALLOW_CHRG_POWER =
          ((rx_frame.data.u8[2] & 0x01) << 13) | (rx_frame.data.u8[3] << 5) | ((rx_frame.data.u8[4] & 0xF8) >> 3);
      MAX_ALLOW_DISCHRG_POWER =
          ((rx_frame.data.u8[5] & 0x07) << 11) | (rx_frame.data.u8[6] << 3) | ((rx_frame.data.u8[7] & 0xE0) >> 5);
      CHECKSUM_FRAME_414 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0x9
      //COUNTER_414 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x353:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_COP_VOLTAGE =
          (rx_frame.data.u8[1] << 5) | (rx_frame.data.u8[2] >> 3);  //Real voltage HV battery (dV, 0-5000)
      HV_BATT_COP_CURRENT =
          (rx_frame.data.u8[3] << 5) | (rx_frame.data.u8[4] >> 3);  //High resolution battery current (dA, -4000 - 4000)
      CHECKSUM_FRAME_353 = (rx_frame.data.u8[0] & 0xF0) >> 4;       //Frame checksum 0xB
      //COUNTER_353 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x474:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMS_DC_RELAY_MES_EVSE_VOLTAGE = (rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[2] >> 2);  //V 0-1000
      FAST_CHARGE_CONTACTOR_STATE = (rx_frame.data.u8[2] & 0x03);
      /*00: Contactors Opened 
        01: Contactors Closed 
        10: No Request 
        11: WELDING TEST*/
      BMS_FASTCHARGE_STATUS = (rx_frame.data.u8[4] & 0x03);
      /*00 : not charging 
        01 : charging
        10 : charging fault
        11 : charging finished*/
      CHECKSUM_FRAME_474 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0xF
      //COUNTER_474 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x574:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_FC_INSU_MINUS_RES = (rx_frame.data.u8[0] << 5) | (rx_frame.data.u8[1] >> 3);  //kOhm (0-60000)
      HV_BATT_FC_VHL_INSU_PLUS_RES =
          ((rx_frame.data.u8[1] & 0x07) << 10) | (rx_frame.data.u8[2] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6);
      HV_BATT_FC_INSU_PLUS_RES = (rx_frame.data.u8[5] << 4) | (rx_frame.data.u8[6] >> 4);
      HV_BATT_ONLY_INSU_MINUS_RES = ((rx_frame.data.u8[3] & 0x3F) << 7) | (rx_frame.data.u8[4] >> 1);
      break;
    case 0x583:  //MysteryVan 50/75kWh platform (CAN-FD also?)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      ALERT_OVERCHARGE = (rx_frame.data.u8[4] & 0x20) >> 5;
      NUMBER_PROBE_TEMP_MAX = rx_frame.data.u8[0];
      NUMBER_PROBE_TEMP_MIN = rx_frame.data.u8[1];
      TEMPERATURE_MINIMUM_C = rx_frame.data.u8[2] - 40;
      ALERT_BATT = (rx_frame.data.u8[3] & 0x80) >> 7;
      ALERT_TEMP_DIFF = (rx_frame.data.u8[3] & 0x40) >> 6;
      ALERT_HIGH_TEMP = (rx_frame.data.u8[3] & 0x20) >> 5;
      ALERT_OVERVOLTAGE = (rx_frame.data.u8[3] & 0x10) >> 4;
      ALERT_LOW_SOC = (rx_frame.data.u8[3] & 0x08) >> 3;
      ALERT_HIGH_SOC = (rx_frame.data.u8[3] & 0x04) >> 2;
      ALERT_CELL_OVERVOLTAGE = (rx_frame.data.u8[3] & 0x02) >> 1;
      ALERT_CELL_UNDERVOLTAGE = (rx_frame.data.u8[3] & 0x01);
      ALERT_SOC_JUMP = (rx_frame.data.u8[4] & 0x80) >> 7;
      ALERT_CELL_POOR_CONSIST = (rx_frame.data.u8[4] & 0x40) >> 6;
      CONTACTOR_OPENING_REASON = (rx_frame.data.u8[4] & 0x1C) >> 2;
      /*
      000 : Not error
      001 : Crash
      010 : 12V supply source undervoltage
      011 : 12V supply source overvoltage
      100 : Battery temperature
      101 : interlock line open
      110 : e-Service plug disconnected
      111 : Not valid
      */
      NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY = rx_frame.data.u8[5];
      NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY = rx_frame.data.u8[6];

      break;
    case 0x314:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      MIN_ALLOW_DISCHRG_VOLTAGE = (rx_frame.data.u8[1] << 3) | (rx_frame.data.u8[2] >> 5);  //V (0-1000)
      //EVSE_DC_MAX_CURRENT = ((rx_frame.data.u8[2] & 0x1F) << 5) | (rx_frame.data.u8[3] >> 3); //Fastcharger info, not needed for BE
      //TBMU_EVSE_DC_MAX_VOLTAGE //Fastcharger info, not needed for BE
      //TBMU_MAX_CHRG_SCKT_TEMP //Fastcharger info, not needed for BE
      //DC_CHARGE_MODE_AVAIL //Fastcharger info, not needed for BE
      //BIDIR_V2HG_MODE_AVAIL //Fastcharger info, not needed for BE
      //TBMU_CHRG_CONN_CONF //Fastcharger info, not needed for BE
      //EVSE_GRID_FAULT //Fastcharger info, not needed for BE
      CHECKSUM_FRAME_314 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0x8
      //COUNTER_314 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x254:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HV_BATT_SOE_MAX_HR = frame6 & frame7 //Only on FD-CAN variant of the message. FD has length 7, non-fd 5
      HV_BATT_NOMINAL_DISCH_CURR_HD = (rx_frame.data.u8[0] << 7) | (rx_frame.data.u8[1] >> 1);  //dA (0-20000)
      HV_BATT_PEAK_DISCH_CURR_HD = (rx_frame.data.u8[2] << 7) | (rx_frame.data.u8[3] >> 1);     //dA (0-20000)
      HV_BATT_STABLE_DISCH_CURR_HD = (rx_frame.data.u8[4] << 7) | (rx_frame.data.u8[5] >> 1);   //dA (0-20000)
      break;
    case 0x2B4:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_NOMINAL_CHARGE_CURR_HD = (rx_frame.data.u8[0] << 7) | (rx_frame.data.u8[1] >> 1);
      HV_BATT_PEAK_CHARGE_CURR_HD = (rx_frame.data.u8[2] << 7) | (rx_frame.data.u8[3] >> 1);
      HV_BATT_STABLE_CHARGE_CURR_HD = (rx_frame.data.u8[4] << 7) | (rx_frame.data.u8[5] >> 1);
      break;
    case 0x4D4:  //MysteryVan 50/75kWh platform
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_STABLE_CHARGE_POWER_HD = (rx_frame.data.u8[0] << 6) | (rx_frame.data.u8[1] >> 2);
      HV_BATT_STABLE_DISCH_POWER_HD =
          ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) | ((rx_frame.data.u8[4] & 0xF0) >> 4);
      HV_BATT_NOMINAL_DISCH_POWER_HD =
          ((rx_frame.data.u8[4] & 0x0F) << 10) | (rx_frame.data.u8[5] << 2) | ((rx_frame.data.u8[6] & 0xC0) >> 6);
      MAX_ALLOW_DISCHRG_CURRENT = ((rx_frame.data.u8[6] & 0x3F) << 5) | (rx_frame.data.u8[7] >> 3);
      RC01_PERM_SYNTH_TBMU = (rx_frame.data.u8[7] & 0x04) >> 2;  //TBMU Readiness Code synthesis
      CHECKSUM_FRAME_4D4 = (rx_frame.data.u8[0] & 0xF0) >> 4;    //Frame checksum 0x5
      //COUNTER_4D4 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x125:  //Common eCMP
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_soc = (rx_frame.data.u8[0] << 2) |
                    (rx_frame.data.u8[1] >> 6);  // Byte1, bit 7 length 10 (0x3FE when abnormal) (0-1000 ppt)
      battery_MainConnectorState = ((rx_frame.data.u8[2] & 0x18) >>
                                    3);  //Byte2 , bit 4, length 2 ((00 contactors open, 01 precharged, 11 invalid))
      battery_voltage =
          (rx_frame.data.u8[3] << 1) | (rx_frame.data.u8[4] >> 7);  //Byte 4, bit 7, length 9 (0x1FE if invalid)
      battery_current = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]) - 600;  // TODO: Test
      break;
    case 0x127:  //DFM specific
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_AllowedMaxChargeCurrent =
          (rx_frame.data.u8[0] << 2) |
          ((rx_frame.data.u8[1] & 0xC0) >> 6);  //Byte 1, bit 7, length 10 (0-600A) [0x3FF if invalid]
      battery_AllowedMaxDischargeCurrent =
          ((rx_frame.data.u8[2] & 0x3F) << 4) |
          (rx_frame.data.u8[3] >> 4);  //Byte 2, bit 5, length 10 (0-600A) [0x3FF if invalid]
      break;
    case 0x129:  //PSA specific
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x31B:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_InterlockOpen = ((rx_frame.data.u8[1] & 0x10) >> 4);  //Best guess, seems to work?
      //TODO: frame7 contains checksum, we can use this to check for CAN message corruption
      break;
    case 0x358:  //Common
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_highestTemperature = rx_frame.data.u8[6] - 40;
      battery_lowestTemperature = rx_frame.data.u8[7] - 40;
      break;
    case 0x359:
      break;
    case 0x361:  //BMS6_361 , removed on batteries newer than 5/7/2022
      break;
    case 0x362:
      break;
    case 0x454:
      break;
    case 0x494:
      break;
    case 0x594:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_insulation_failure_diag = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //Unsure if this is right position
      //byte pos 6, bit pos 7, signal lenth 3
      //0 = no failure, 1 = symmetric failure, 4 = invalid value , forbidden value 5-7
      break;
    case 0x6D0:  //Common
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_insulationResistanceKOhm =
          (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];  //Byte 2, bit 7, length 16 (0-60000 kOhm)
      datalayer_battery->status.insulation_resistance_kOhm = battery_insulationResistanceKOhm;
      datalayer_battery->status.insulation_resistance_available = true;
      break;
    case 0x6D1:  //Temperatures? (39 39 39 39 39 39 39 39)
      break;
    case 0x6D2:  //Temperatures? (39 39 39 39 39 39 39 39)
      break;
    case 0x6D3:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellvoltages[0] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[1] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[2] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[3] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6D4:
      cellvoltages[4] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[5] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[6] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[7] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E0:  //Temperatures? (39 39 39 39 39 39 00 00)
      break;
    case 0x6E1:
      cellvoltages[8] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[9] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[10] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[11] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E2:
      cellvoltages[12] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[13] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[14] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[15] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E3:  //Temperatures? (39 3a 39 39 39 39 39 39)
      break;
    case 0x6E4:  //Temperatures? (3a 3a 3a 39 39 39 39 39)
      break;
    case 0x6E5:  //Temperatures? (3a 39 39 3a 39 39 39 39)
      break;
    case 0x6E6:  //Temperatures? (3a 39 39 3a 39 3b 3d 3a)
      break;
    case 0x6E7:
      cellvoltages[16] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[17] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[18] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[19] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E8:
      cellvoltages[20] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[21] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[22] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[23] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6E9:
      cellvoltages[24] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[25] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[26] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[27] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EB:
      cellvoltages[28] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[29] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[30] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[31] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EC:
      //Not available on e-C4, neither on Opel CorsaE 50kWh, neither on Vivaro 75kWh
      break;
    case 0x6ED:
    case 0x6EE:
    case 0x6EF:
    case 0x6F0:
    case 0x6F1:
    case 0x6F2:
    case 0x6F3:
    case 0x6F4:
    case 0x6F5:
    case 0x6F6:
    case 0x6F7:
    case 0x6F8:
    case 0x6F9:
    case 0x6FA:
    case 0x6FB:
    case 0x6FC:
    case 0x6FD:
    case 0x6FE: {  //Cellvoltages between 32 - 103
      // Calculate starting index: each ID increments by 1, each group is 4 cells
      int start_index = 32 + (rx_frame.ID - 0x6ED) * 4;

      cellvoltages[start_index + 0] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[start_index + 1] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[start_index + 2] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[start_index + 3] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    }
    case 0x6FF:
      cellvoltages[104] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[105] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[106] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[107] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      memcpy(datalayer_battery->status.cell_voltages_mV, cellvoltages, 108 * sizeof(uint16_t));
      break;
    case 0x694:  // Poll reply
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      // Handle user requested functionality first if ongoing
      if (UserRequestContactorReset) {
        if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x50) && (rx_frame.data.u8[2] == 0x03)) {
          //06,50,03,00,C8,00,14,00,
          ContactorResetStatemachine = 2;  //Send ECMP_CONTACTOR_RESET_START next loop
        }
        if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x71) && (rx_frame.data.u8[2] == 0x01)) {
          //05,71,01,DD,35,01,00,00,
          ContactorResetStatemachine = 4;  //Send ECMP_CONTACTOR_RESET_PROGRESS next loop
        }
        if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x71) && (rx_frame.data.u8[2] == 0x03)) {
          //05,71,03,DD,35,02,00,00,
          ContactorResetStatemachine = COMPLETED_STATE;
          UserRequestContactorReset = false;
          timeSpentContactorReset = COMPLETED_STATE;
        }

      } else if (UserRequestCollisionReset) {
        if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x50) && (rx_frame.data.u8[2] == 0x03)) {
          //06,50,03,00,C8,00,14,00,
          CollisionResetStatemachine = 2;  //Send ECMP_COLLISION_RESET_START next loop
        }
        if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x71) && (rx_frame.data.u8[2] == 0x01)) {
          //05,71,01,DF,60,01,00,00,
          CollisionResetStatemachine = 4;  //Send ECMP_COLLISION_RESET_PROGRESS next loop
        }
        if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x71) && (rx_frame.data.u8[2] == 0x03)) {
          if (rx_frame.data.u8[5] == 0x01) {
            //05,71,03,DF,60,01,00,00,
            CollisionResetStatemachine = 4;  //Send ECMP_COLLISION_RESET_PROGRESS next loop
          }
          if (rx_frame.data.u8[5] == 0x02) {
            //05,71,03,DF,60,02,00,00,
            CollisionResetStatemachine = COMPLETED_STATE;
            UserRequestCollisionReset = false;
            timeSpentCollisionReset = COMPLETED_STATE;
          }
        }

      } else if (UserRequestIsolationReset) {
        if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x50) && (rx_frame.data.u8[2] == 0x03)) {
          //06,50,03,00,C8,00,14,00,
          IsolationResetStatemachine = 2;  //Send ECMP_ISOLATION_RESET_START next loop
        }
        if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x71) && (rx_frame.data.u8[2] == 0x01)) {
          //05,71,01,DF,46,01,00,00,
          IsolationResetStatemachine = 4;  //Send ECMP_ISOLATION_RESET_PROGRESS next loop
        }
        if ((rx_frame.data.u8[0] == 0x05) && (rx_frame.data.u8[1] == 0x71) && (rx_frame.data.u8[2] == 0x03)) {
          if (rx_frame.data.u8[5] == 0x01) {
            //05,71,03,DF,46,01,00,00,
            IsolationResetStatemachine = 4;  //Send ECMP_ISOLATION_RESET_PROGRESS next loop
          }
          if (rx_frame.data.u8[5] == 0x02) {
            //05,71,03,DF,46,02,00,00,
            IsolationResetStatemachine = COMPLETED_STATE;
            UserRequestIsolationReset = false;
            timeSpentIsolationReset = COMPLETED_STATE;
          }
        }
      }
  }
}

uint16_t EcmpBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_WELD_CHECK:
      pid_welding_detection = value;  //00 all good
      break;
    case PID_CONT_REASON_OPEN:
      pid_reason_open = value;
      break;
    case PID_CONTACTOR_STATUS:
      pid_contactor_status = value;
      break;
    case PID_NEG_CONT_CONTROL:
      pid_negative_contactor_control = value;
      break;
    case PID_NEG_CONT_STATUS:
      pid_negative_contactor_status = value;
      break;
    case PID_POS_CONT_CONTROL:
      pid_positive_contactor_control = value;
      break;
    case PID_POS_CONT_STATUS:
      pid_positive_contactor_status = value;
      break;
    case PID_CONTACTOR_NEGATIVE:
      pid_contactor_negative = value;
      break;
    case PID_CONTACTOR_POSITIVE:
      pid_contactor_positive = value;
      break;
    case PID_PRECHARGE_RELAY_CONTROL:
      pid_precharge_relay_control = value;
      break;
    case PID_PRECHARGE_RELAY_STATUS:
      pid_precharge_relay_status = value;
      break;
    case PID_RECHARGE_STATUS:
      pid_recharge_status = value;
      break;
    case PID_DELTA_TEMPERATURE:
      pid_delta_temperature = value;
      break;
    case PID_COLDEST_MODULE:
      pid_coldest_module = value;
      break;
    case PID_LOWEST_TEMPERATURE:
      pid_lowest_temperature = value;
      break;
    case PID_AVERAGE_TEMPERATURE:
      pid_average_temperature = value;
      break;
    case PID_HIGHEST_TEMPERATURE:
      pid_highest_temperature = value;
      break;
    case PID_HOTTEST_MODULE:
      pid_hottest_module = value;
      break;
    case PID_AVG_CELL_VOLTAGE:
      pid_avg_cell_voltage = value;
      break;
    case PID_CURRENT:
      pid_current = -(((value)-76800) * 155) / 10;
      break;
    case PID_INSULATION_NEG:
      pid_insulation_res_neg = value;
      break;
    case PID_INSULATION_POS:
      pid_insulation_res_pos = value;
      break;
    case PID_MAX_CURRENT_10S:
      pid_max_current_10s = value;
      break;
    case PID_MAX_DISCHARGE_10S:
      pid_max_discharge_10s = value;
      break;
    case PID_MAX_DISCHARGE_30S:
      pid_max_discharge_30s = value;
      break;
    case PID_MAX_CHARGE_10S:
      pid_max_charge_10s = value;
      break;
    case PID_MAX_CHARGE_30S:
      pid_max_charge_30s = value;
      break;
    case PID_ENERGY_CAPACITY:
      pid_energy_capacity = value;
      break;
    case PID_HIGH_CELL_NUM:
      pid_highest_cell_voltage_num = value;
      break;
    case PID_LOW_CELL_NUM:
      pid_lowest_cell_voltage_num = value;
      break;
    case PID_SUM_OF_CELLS:
      pid_sum_of_cells = value / 2;
      break;
    case PID_CELL_MIN_CAPACITY:
      pid_cell_min_capacity = value;
      break;
    case PID_CELL_VOLTAGE_MEAS_STATUS:
      pid_cell_voltage_measurement_status = value;
      break;
    case PID_INSULATION_RES:
      pid_insulation_res = value;
      break;
    case PID_PACK_VOLTAGE:
      pid_pack_voltage = value;
      break;
    case PID_HIGH_CELL_VOLTAGE:
      pid_high_cell_voltage = value;
      break;
    case PID_ALL_CELL_VOLTAGES:  //Multiframe (No need to poll this, we can get it from constantly sent CAN)
      break;
    case PID_LOW_CELL_VOLTAGE:
      pid_low_cell_voltage = value;
      break;
    case PID_BATTERY_ENERGY:
      pid_battery_energy = value;
      break;
    case PID_CELLBALANCE_STATUS:  //Multiframe 20 bytes
      // All values appear 0x00 in every single log
      break;
    case PID_CELLBALANCE_HWERR_MASK:  //Multiframe
      // All values appear 0x00 in every single log
      break;
    case PID_CRASH_COUNTER:
      pid_crash_counter = value;
      break;
    case PID_WIRE_CRASH:
      pid_wire_crash = value;
      break;
    case PID_CAN_CRASH:
      //pid_can_crash = value;
      break;
    case PID_HISTORY_DATA:  //Multiframe
                            //Extremely long reply. Not worth it for us to store this data
      break;
    case PID_LOWSOC_COUNTER:
      pid_lowsoc_counter = value;
      break;
    case PID_LAST_CAN_FAILURE_DETAIL:
      pid_last_can_failure_detail = value;
      break;
    case PID_HW_VERSION_NUM:  //Not available on all batteries - Multiframe
      if (length >= 17) {
        memcpy(pid_hw_version_num, data, 17);
      }
      break;
    case PID_SW_VERSION_NUM:  //Not available on all batteries - Multiframe
      if (length >= 17) {
        memcpy(pid_sw_version_num, data, 17);
      }
      break;
    case PID_FACTORY_MODE_CONTROL:
      pid_factory_mode_control = value;
      break;
    case PID_BATTERY_SERIAL:  //Multiframe
      if (length >= 14) {
        memcpy(pid_battery_serial, data, 14);
      }
      break;
    case PID_ALL_CELL_SOH:  //Multiframe
      pid_SOH_cell_1 = data[0] << 8 | data[1];
      //No need for us to read all 108 cells, we can just read the first one and assume the rest are similar
      break;
    case PID_AUX_FUSE_STATE:
      pid_aux_fuse_state = value;
      break;
    case PID_BATTERY_STATE:
      pid_battery_state = value;
      break;
    case PID_PRECHARGE_SHORT_CIRCUIT:
      pid_precharge_short_circuit = value;
      break;
    case PID_ESERVICE_PLUG_STATE:
      pid_eservice_plug_state = value;
      break;
    case PID_MAINFUSE_STATE:
      pid_mainfuse_state = value;
      break;
    case PID_MOST_CRITICAL_FAULT:
      pid_most_critical_fault = value;
      break;
    case PID_CURRENT_TIME:  //Multiframe
                            // 6 bytes long (10 01 01 00 1A 2C) Unclear how to map this
      //pid_current_time = (((data[0]) << 38) | ((data[1]) << 32) | ((data[2]) << 24) | ((data[3]) << 16) | ((data[4]) << 8) | (data[5]);
      break;
    case PID_TIME_SENT_BY_CAR:  //(0b c8 d3 2c)
      pid_time_sent_by_car = value;
      break;
    case PID_12V:
      pid_12v = value;
      break;
    case PID_12V_ABNORMAL:
      pid_12v_abnormal = value;
      break;
    case PID_HVIL_IN_VOLTAGE:
      pid_hvil_in_voltage = value;
      break;
    case PID_HVIL_OUT_VOLTAGE:
      pid_hvil_out_voltage = value;
      break;
    case PID_HVIL_STATE:
      pid_hvil_state = value;
      break;
    case PID_BMS_STATE:
      pid_bms_state = value;
      break;
    case PID_VEHICLE_SPEED:
      pid_vehicle_speed = value;
      break;
    case PID_TIME_SPENT_OVER_55C:
      pid_time_spent_over_55c = value;
      break;
    case PID_CONTACTOR_CLOSING_COUNTER:
      pid_contactor_closing_counter = value;
      break;
    case PID_DATE_OF_MANUFACTURE:  //Raw hex value is day/month/year (e.g. 09 08 22)
      pid_date_of_manufacture = value;
      break;
    default:
      break;
  }
  return 0;  // Continue scanning
}

uint8_t checksum_calc(uint8_t counter, CAN_frame rx_frame) {
  // Confirmed working on IDs 0F0,0F2,17B,31B,31D,31E,3A2(special),3A3,112,351
  // Sum of frame ID nibbles + Sum all nibbles of data bytes (frames 0–6 and high nibble of frame7)
  int sum = ((rx_frame.ID >> 8) & 0xF) + ((rx_frame.ID >> 4) & 0xF) + (rx_frame.ID & 0xF);
  sum += (rx_frame.data.u8[0] >> 4) + (rx_frame.data.u8[0] & 0xF);
  sum += (rx_frame.data.u8[1] >> 4) + (rx_frame.data.u8[1] & 0xF);
  sum += (rx_frame.data.u8[2] >> 4) + (rx_frame.data.u8[2] & 0xF);
  sum += (rx_frame.data.u8[3] >> 4) + (rx_frame.data.u8[3] & 0xF);
  sum += (rx_frame.data.u8[4] >> 4) + (rx_frame.data.u8[4] & 0xF);
  sum += (rx_frame.data.u8[5] >> 4) + (rx_frame.data.u8[5] & 0xF);
  sum += (rx_frame.data.u8[6] >> 4) + (rx_frame.data.u8[6] & 0xF);
  sum += (counter);  //high nibble of frame7

  // Compute: (0xF - sum) % 16
  return (0xF - sum) & 0xF;  // Masking with & 0xF ensures modulo 16
}

void EcmpBattery::transmit_can(unsigned long currentMillis) {

  // UDS PID polling and DTC handling
  // Only exception is if userRequested functionality is requested, then we need to handle the UDS frame ourselves and not let the superclass handle it.
  if (UserRequestContactorReset || UserRequestCollisionReset || UserRequestIsolationReset) {
    //Do nothing
  } else {
    transmit_uds_can(currentMillis);
  }

  // Send 250ms diagnostic CAN Messages
  if (currentMillis - previousMillis250 >= INTERVAL_250_MS) {
    previousMillis250 = currentMillis;

    //To be able to use the battery, isolation monitoring needs to be disabled
    //Failure to do this results in the contactors opening after 30 seconds with load
    if (UserRequestContactorReset) {
      if (ContactorResetStatemachine == 0) {
        transmit_can_frame(&ECMP_DIAG_START);
        ContactorResetStatemachine = 1;
      }
      if (ContactorResetStatemachine == 2) {
        transmit_can_frame(&ECMP_CONTACTOR_RESET_START);
        ContactorResetStatemachine = 3;
      }
      if (ContactorResetStatemachine == 4) {
        transmit_can_frame(&ECMP_CONTACTOR_RESET_PROGRESS);
        ContactorResetStatemachine = 5;
      }

      timeSpentContactorReset++;
      if (timeSpentContactorReset > 40) {  //Timeout, if command takes more than 10s to complete
        UserRequestContactorReset = false;
        ContactorResetStatemachine = COMPLETED_STATE;
        timeSpentContactorReset = COMPLETED_STATE;
      }

    } else if (UserRequestCollisionReset) {

      if (CollisionResetStatemachine == 0) {
        transmit_can_frame(&ECMP_DIAG_START);
        CollisionResetStatemachine = 1;
      }
      if (CollisionResetStatemachine == 2) {
        transmit_can_frame(&ECMP_COLLISION_RESET_START);
        CollisionResetStatemachine = 3;
      }
      if (CollisionResetStatemachine == 4) {
        transmit_can_frame(&ECMP_COLLISION_RESET_PROGRESS);
        CollisionResetStatemachine = 5;
      }

      timeSpentCollisionReset++;
      if (timeSpentCollisionReset > 40) {  //Timeout, if command takes more than 10s to complete
        UserRequestCollisionReset = false;
        CollisionResetStatemachine = COMPLETED_STATE;
        timeSpentCollisionReset = COMPLETED_STATE;
      }

    } else if (UserRequestIsolationReset) {

      if (IsolationResetStatemachine == 0) {
        transmit_can_frame(&ECMP_DIAG_START);
        IsolationResetStatemachine = 1;
      }
      if (IsolationResetStatemachine == 2) {
        transmit_can_frame(&ECMP_ISOLATION_RESET_START);
        IsolationResetStatemachine = 3;
      }
      if (IsolationResetStatemachine == 4) {
        transmit_can_frame(&ECMP_ISOLATION_RESET_PROGRESS);
        IsolationResetStatemachine = 5;
      }

      timeSpentIsolationReset++;
      if (timeSpentIsolationReset > 40) {  //Timeout, if command takes more than 10s to complete
        if (countIsolationReset < 4) {
          countIsolationReset++;
          IsolationResetStatemachine = 0;  //Reset state machine to start over
        } else {
          UserRequestIsolationReset = false;
          IsolationResetStatemachine = COMPLETED_STATE;
          timeSpentIsolationReset = COMPLETED_STATE;
          countIsolationReset = 0;
        }
      }
    }
  }

  // Send 10ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    counter_10ms = (counter_10ms + 1) % 16;

    if (datalayer.system.status.system_status == FAULT) {
      //Make vehicle appear as in idle HV state. Useful for clearing DTCs
      ECMP_0F2.data = {0x7D, 0x00, 0x4E, 0x20, 0x00, 0x00, 0x60, 0x0D};
      ECMP_17B.data = {0x00, 0x00, 0x00, 0x7E, 0x78, 0x00, 0x00, 0x0F};
      ECMP_110.data.u8[6] = 0x87;
      ECMP_110.data.u8[7] = 0x05;
    } else {
      //Normal operation for contactor closing
      ECMP_0F2.data = {0x7D, 0x00, 0x4E, 0x20, 0x00, 0x00, 0x90, 0x0D};
      ECMP_110.data.u8[6] = 0x4E;
      ECMP_110.data.u8[7] = 0x20;
      ECMP_17B.data = {0x00, 0x00, 0x00, 0x7F, 0x98, 0x00, 0x00, 0x0F};
    }

    ECMP_0F2.data.u8[7] = counter_10ms << 4 | checksum_calc(counter_10ms, ECMP_0F2);
    ECMP_17B.data.u8[7] = counter_10ms << 4 | checksum_calc(counter_10ms, ECMP_17B);
    ECMP_112.data.u8[7] = counter_10ms << 4 | checksum_calc(counter_10ms, ECMP_112);

    transmit_can_frame(&ECMP_112);  //MCU1_112
    transmit_can_frame(&ECMP_0C5);  //DC2_0C5
    transmit_can_frame(&ECMP_17B);  //VCU_PCANInfo_17B
    transmit_can_frame(&ECMP_0F2);  //CtrlMCU1_0F2
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
    transmit_can_frame(&ECMP_111);
    transmit_can_frame(&ECMP_110);
    transmit_can_frame(&ECMP_114);
#endif
  }

  // Send 20ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    if (datalayer.system.status.system_status == FAULT) {
      //Open contactors!
      ECMP_0F0.data.u8[1] = 0x00;
    } else {  // Not in faulted mode, Close contactors!
      ECMP_0F0.data.u8[1] = 0x20;
    }

    counter_20ms = (counter_20ms + 1) % 16;

    ECMP_0F0.data.u8[7] = counter_20ms << 4 | checksum_calc(counter_20ms, ECMP_0F0);

    transmit_can_frame(&ECMP_0F0);  //VCU2_0F0
  }
  // Send 50ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    if (datalayer.system.status.system_status == FAULT) {
      //Make vehicle appear as in idle HV state. Useful for clearing DTCs
      ECMP_27A.data = {0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    } else {
      //Normal operation for contactor closing
      ECMP_27A.data = {0x4F, 0x58, 0x00, 0x02, 0x24, 0x00, 0x00, 0x00};
    }
    transmit_can_frame(&ECMP_230);  //OBC3_230
    transmit_can_frame(&ECMP_27A);  //VCU_BSI_Wakeup_27A
  }
  // Send 100ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    counter_100ms = (counter_100ms + 1) % 16;
    counter_010 = (counter_010 + 1) % 8;

    if (datalayer.system.status.system_status == FAULT) {
      //Make vehicle appear as in idle HV state. Useful for clearing DTCs
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
      ECMP_31E.data.u8[0] = 0x48;
      ECMP_351.data = {0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0E};
      ECMP_372.data = {0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      ECMP_383.data.u8[0] = 0x00;
#endif
      ECMP_345.data = {0x45, 0x57, 0x00, 0x04, 0x00, 0x00, 0x06, 0x31};
      ECMP_3A2.data = {0x03, 0xE8, 0x00, 0x00, 0x81, 0x00, 0x08, 0x02};
      ECMP_3A3.data = {0x4A, 0x4A, 0x40, 0x00, 0x00, 0x08, 0x00, 0x0F};
      data_345_content[0] = 0x04;  // Allows for DTCs to clear
      data_345_content[1] = 0xF5;
      data_345_content[2] = 0xE6;
      data_345_content[3] = 0xD7;
      data_345_content[4] = 0xC8;
      data_345_content[5] = 0xB9;
      data_345_content[6] = 0xAA;
      data_345_content[7] = 0x9B;
      data_345_content[8] = 0x8C;
      data_345_content[9] = 0x7D;
      data_345_content[10] = 0x6E;
      data_345_content[11] = 0x5F;
      data_345_content[12] = 0x40;
      data_345_content[13] = 0x31;
      data_345_content[14] = 0x22;
      data_345_content[15] = 0x13;
      data_3A2_CRC[0] = 0x0C;
      data_3A2_CRC[1] = 0x1B;
      data_3A2_CRC[2] = 0x2A;
      data_3A2_CRC[3] = 0x39;
      data_3A2_CRC[4] = 0x48;
      data_3A2_CRC[5] = 0x57;
      data_3A2_CRC[6] = 0x66;
      data_3A2_CRC[7] = 0x75;
      data_3A2_CRC[8] = 0x84;
      data_3A2_CRC[9] = 0x93;
      data_3A2_CRC[10] = 0xA2;
      data_3A2_CRC[11] = 0xB1;
      data_3A2_CRC[12] = 0xC0;
      data_3A2_CRC[13] = 0xDF;
      data_3A2_CRC[14] = 0xEE;
      data_3A2_CRC[15] = 0xFD;
      transmit_can_frame(&ECMP_3D0);  //Not in logs, but makes speed go to 0km/h
    } else {
      //Normal operation for contactor closing
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
      ECMP_31E.data.u8[0] = 0x50;
      ECMP_351.data = {0x00, 0x00, 0x00, 0x00, 0x0E, 0xA0, 0x00, 0xE0};
      ECMP_372.data = {0x9A, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      ECMP_383.data.u8[0] = 0x40;
#endif
      ECMP_345.data = {0x45, 0x52, 0x00, 0x04, 0xDD, 0x00, 0x02, 0x30};
      ECMP_3A2.data = {0x01, 0x68, 0x00, 0x00, 0x81, 0x00, 0x08, 0x02};
      ECMP_3A3.data = {0x49, 0x49, 0x40, 0x00, 0xDD, 0x08, 0x00, 0x0F};
      data_345_content[0] = 0x00;  // Allows for contactor closing
      data_345_content[1] = 0xF1;
      data_345_content[2] = 0xE2;
      data_345_content[3] = 0xD3;
      data_345_content[4] = 0xC4;
      data_345_content[5] = 0xB5;
      data_345_content[6] = 0xA6;
      data_345_content[7] = 0x97;
      data_345_content[8] = 0x88;
      data_345_content[9] = 0x79;
      data_345_content[10] = 0x6A;
      data_345_content[11] = 0x5B;
      data_345_content[12] = 0x4C;
      data_345_content[13] = 0x3D;
      data_345_content[14] = 0x2E;
      data_345_content[15] = 0x1F;
      data_3A2_CRC[0] = 0x06;  // Allows for contactor closing
      data_3A2_CRC[1] = 0x15;
      data_3A2_CRC[2] = 0x24;
      data_3A2_CRC[3] = 0x33;
      data_3A2_CRC[4] = 0x42;
      data_3A2_CRC[5] = 0x51;
      data_3A2_CRC[6] = 0x60;
      data_3A2_CRC[7] = 0x7F;
      data_3A2_CRC[8] = 0x8E;
      data_3A2_CRC[9] = 0x9D;
      data_3A2_CRC[10] = 0xAC;
      data_3A2_CRC[11] = 0xBB;
      data_3A2_CRC[12] = 0xCA;
      data_3A2_CRC[13] = 0xD9;
      data_3A2_CRC[14] = 0xE8;
      data_3A2_CRC[15] = 0xF7;
    }

    ECMP_3A2.data.u8[6] = data_3A2_CRC[counter_100ms];
    ECMP_3A3.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_3A3);
    ECMP_010.data.u8[0] = data_010_CRC[counter_010];
    ECMP_345.data.u8[3] = (uint8_t)((data_345_content[counter_100ms] & 0XF0) | 0x4);
    ECMP_345.data.u8[7] = (uint8_t)(0x3 << 4 | (data_345_content[counter_100ms] & 0X0F));
    ECMP_3D0.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_3D0);

    transmit_can_frame(&ECMP_382);  //PSA Specific VCU (BSIInfo_382)
    transmit_can_frame(&ECMP_345);  //DC1_345
    transmit_can_frame(&ECMP_3A2);  //OBC2_3A2
    transmit_can_frame(&ECMP_3A3);  //OBC1_3A3
    transmit_can_frame(&ECMP_010);  //VCU_BCM_Crash
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
    ECMP_31E.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_31E);
    ECMP_351.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_351);
    ECMP_31D.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_31D);
    transmit_can_frame(&ECMP_31E);
    transmit_can_frame(&ECMP_383);
    transmit_can_frame(&ECMP_0A6);  //Not in all logs
    transmit_can_frame(&ECMP_37F);  //Seems to be temperatures of some sort
    transmit_can_frame(&ECMP_372);
    transmit_can_frame(&ECMP_351);
    transmit_can_frame(&ECMP_31D);
#endif
  }
  // Send 500ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
    transmit_can_frame(&ECMP_0AE);
#endif
  }
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //552 seems to be tracking time in byte 0-3 , distance in km in byte 4-6, temporal reset counter in byte 7
    ticks_552 = (ticks_552 + 10);
    ECMP_552.data.u8[0] = ((ticks_552 & 0xFF000000) >> 24);
    ECMP_552.data.u8[1] = ((ticks_552 & 0x00FF0000) >> 16);
    ECMP_552.data.u8[2] = ((ticks_552 & 0x0000FF00) >> 8);
    ECMP_552.data.u8[3] = (ticks_552 & 0x000000FF);

    transmit_can_frame(&ECMP_439);  //OBC4
    transmit_can_frame(&ECMP_552);  //VCU_552 timetracking
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
    if (datalayer.battery.status.bms_status == FAULT) {
      //Make vehicle appear as in idle HV state. Useful for clearing DTCs
      ECMP_486.data.u8[0] = 0x80;
      ECMP_794.data.u8[0] = 0xB8;  //Not sure if needed, could be static?
    } else {
      //Normal operation for contactor closing
      ECMP_486.data.u8[0] = 0x00;
      ECMP_794.data.u8[0] = 0x38;  //Not sure if needed, could be static?
    }
    transmit_can_frame(&ECMP_486);  //Not in all logs
    transmit_can_frame(&ECMP_041);  //Not in all logs
    transmit_can_frame(&ECMP_786);  //Not in all logs
    transmit_can_frame(&ECMP_591);  //Not in all logs
    transmit_can_frame(&ECMP_794);  //Not in all logs
#endif
  }
  // Send 5s periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
#ifdef SIMULATE_ENTIRE_VEHICLE_ECMP
    transmit_can_frame(&ECMP_55F);
#endif
  }
}

void EcmpBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 108;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  // UDS: send requests to 0x6B4, accept replies from the BMS on 0x694
  setup_uds(0x6B4, 0x694);
  static const uint16_t pid_scan_list[] = {
      //PID_CURRENT is sampled every other time, so we can get a fast update rate on it
      PID_WELD_CHECK,
      PID_CURRENT,
      PID_CONT_REASON_OPEN,
      PID_CURRENT,
      PID_CONTACTOR_STATUS,
      PID_CURRENT,
      PID_NEG_CONT_CONTROL,
      PID_CURRENT,
      PID_NEG_CONT_STATUS,
      PID_CURRENT,
      PID_POS_CONT_CONTROL,
      PID_CURRENT,
      PID_POS_CONT_STATUS,
      PID_CURRENT,
      PID_CONTACTOR_NEGATIVE,
      PID_CURRENT,
      PID_CONTACTOR_POSITIVE,
      PID_CURRENT,
      PID_PRECHARGE_RELAY_CONTROL,
      PID_CURRENT,
      PID_PRECHARGE_RELAY_STATUS,
      PID_CURRENT,
      PID_RECHARGE_STATUS,
      PID_CURRENT,
      PID_DELTA_TEMPERATURE,
      PID_CURRENT,
      PID_COLDEST_MODULE,
      PID_CURRENT,
      PID_LOWEST_TEMPERATURE,
      PID_CURRENT,
      PID_AVERAGE_TEMPERATURE,
      PID_CURRENT,
      PID_HIGHEST_TEMPERATURE,
      PID_CURRENT,
      PID_HOTTEST_MODULE,
      PID_CURRENT,
      PID_AVG_CELL_VOLTAGE,
      PID_CURRENT,
      PID_INSULATION_NEG,
      PID_CURRENT,
      PID_INSULATION_POS,
      PID_CURRENT,
      PID_MAX_CURRENT_10S,
      PID_CURRENT,
      PID_MAX_DISCHARGE_10S,
      PID_CURRENT,
      PID_MAX_DISCHARGE_30S,
      PID_CURRENT,
      PID_MAX_CHARGE_10S,
      PID_CURRENT,
      PID_MAX_CHARGE_30S,
      PID_CURRENT,
      PID_ENERGY_CAPACITY,
      PID_CURRENT,
      PID_HIGH_CELL_NUM,
      PID_CURRENT,
      PID_LOW_CELL_NUM,
      PID_CURRENT,
      PID_SUM_OF_CELLS,
      PID_CURRENT,
      PID_CELL_MIN_CAPACITY,
      PID_CURRENT,
      PID_CELL_VOLTAGE_MEAS_STATUS,
      PID_CURRENT,
      PID_INSULATION_RES,
      PID_CURRENT,
      PID_PACK_VOLTAGE,
      PID_CURRENT,
      PID_HIGH_CELL_VOLTAGE,
      PID_CURRENT,
      //PID_ALL_CELL_VOLTAGES, No need to poll this, we can get it from constantly sent CAN
      PID_CURRENT,
      PID_LOW_CELL_VOLTAGE,
      PID_CURRENT,
      PID_BATTERY_ENERGY,
      PID_CURRENT,
      PID_CELLBALANCE_STATUS,
      PID_CURRENT,
      PID_CELLBALANCE_HWERR_MASK,
      PID_CURRENT,
      PID_CRASH_COUNTER,
      PID_CURRENT,
      PID_WIRE_CRASH,
      PID_CURRENT,
      PID_CAN_CRASH,
      PID_CURRENT,
      //PID_HISTORY_DATA, Insanely long reply. Not worth it for us to store this data
      PID_CURRENT,
      PID_LOWSOC_COUNTER,
      PID_CURRENT,
      PID_LAST_CAN_FAILURE_DETAIL,
      PID_CURRENT,
      PID_HW_VERSION_NUM,
      PID_CURRENT,
      PID_SW_VERSION_NUM,
      PID_CURRENT,
      PID_FACTORY_MODE_CONTROL,
      PID_CURRENT,
      PID_BATTERY_SERIAL,
      PID_CURRENT,
      PID_ALL_CELL_SOH,
      PID_CURRENT,
      PID_AUX_FUSE_STATE,
      PID_CURRENT,
      PID_BATTERY_STATE,
      PID_CURRENT,
      PID_PRECHARGE_SHORT_CIRCUIT,
      PID_CURRENT,
      PID_ESERVICE_PLUG_STATE,
      PID_CURRENT,
      PID_MAINFUSE_STATE,
      PID_CURRENT,
      PID_MOST_CRITICAL_FAULT,
      PID_CURRENT,
      PID_CURRENT_TIME,
      PID_CURRENT,
      PID_TIME_SENT_BY_CAR,
      PID_CURRENT,
      PID_12V,
      PID_CURRENT,
      PID_12V_ABNORMAL,
      PID_CURRENT,
      PID_HVIL_IN_VOLTAGE,
      PID_CURRENT,
      PID_HVIL_OUT_VOLTAGE,
      PID_CURRENT,
      PID_HVIL_STATE,
      PID_CURRENT,
      PID_BMS_STATE,
      PID_CURRENT,
      PID_VEHICLE_SPEED,
      PID_CURRENT,
      PID_TIME_SPENT_OVER_55C,
      PID_CURRENT,
      PID_CONTACTOR_CLOSING_COUNTER,
      PID_CURRENT,
      PID_DATE_OF_MANUFACTURE,
      PID_CURRENT,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
