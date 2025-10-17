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
    datalayer.battery.status.real_soc = battery_soc * 10;

    //datalayer.battery.status.soh_pptt; //TODO: Find SOH%

    datalayer.battery.status.voltage_dV = battery_voltage * 10;

    datalayer.battery.status.current_dA = -(battery_current * 10);

    datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
        ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

    datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
        (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

    datalayer.battery.status.max_charge_power_W = battery_AllowedMaxChargeCurrent * battery_voltage;

    datalayer.battery.status.max_discharge_power_W = battery_AllowedMaxDischargeCurrent * battery_voltage;

    datalayer.battery.status.temperature_min_dC = battery_lowestTemperature * 10;

    datalayer.battery.status.temperature_max_dC = battery_highestTemperature * 10;

    // Initialize min and max, lets find which cells are min and max!
    uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
    uint16_t max_cell_mv_value = 0;
    // Loop to find the min and max while ignoring zero values
    for (uint8_t i = 0; i < datalayer.battery.info.number_of_cells; ++i) {
      uint16_t voltage_mV = datalayer.battery.status.cell_voltages_mV[i];
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

    datalayer.battery.status.cell_min_voltage_mV = min_cell_mv_value;
    datalayer.battery.status.cell_max_voltage_mV = max_cell_mv_value;
  } else {  //Some variant of the 50/75kWh battery that is not using the eCMP CAN mappings.
    // For these batteries we need to use the OBD2 PID polled values

    if (pid_energy_capacity != NOT_SAMPLED_YET) {
      datalayer.battery.status.remaining_capacity_Wh = pid_energy_capacity;
      // calculate SOC based on datalayer.battery.info.total_capacity_Wh and remaining_capacity_Wh
      datalayer.battery.status.real_soc = (uint16_t)(((float)datalayer.battery.status.remaining_capacity_Wh /
                                                      datalayer.battery.info.total_capacity_Wh) *
                                                     10000);
    }

    //datalayer.battery.status.soh_pptt; //TODO: Find SOH%

    if (pid_pack_voltage != NOT_SAMPLED_YET) {
      datalayer.battery.status.voltage_dV = pid_pack_voltage + 800;
    }

    if (pid_current != NOT_SAMPLED_YET) {
      datalayer.battery.status.current_dA = (int16_t)(pid_current / 100);

      datalayer.battery.status.active_power_W =
          (uint16_t)((pid_current / 1000.0f) * (datalayer.battery.status.voltage_dV / 10.0f));
    }

    if (pid_max_charge_10s != NOT_SAMPLED_YET) {
      datalayer.battery.status.max_charge_power_W = pid_max_charge_10s;
    }

    if (pid_max_discharge_10s != NOT_SAMPLED_YET) {
      datalayer.battery.status.max_discharge_power_W = pid_max_discharge_10s;
    }

    if ((pid_highest_temperature != NOT_SAMPLED_YET) && (pid_lowest_temperature != NOT_SAMPLED_YET)) {
      datalayer.battery.status.temperature_max_dC = pid_highest_temperature * 10;
      datalayer.battery.status.temperature_min_dC = pid_lowest_temperature * 10;
    }

    if ((pid_high_cell_voltage != NOT_SAMPLED_YET) && (pid_low_cell_voltage != NOT_SAMPLED_YET)) {
      datalayer.battery.status.cell_max_voltage_mV = pid_high_cell_voltage;
      datalayer.battery.status.cell_min_voltage_mV = pid_low_cell_voltage;
    }

    datalayer.battery.info.number_of_cells = NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY;  //50/75kWh sends valid cellcount
  }

  // Update extended datalayer (More Battery Info page)
  datalayer_extended.stellantisECMP.MainConnectorState = battery_MainConnectorState;
  datalayer_extended.stellantisECMP.InsulationResistance = battery_insulationResistanceKOhm;
  datalayer_extended.stellantisECMP.InsulationDiag = battery_insulation_failure_diag;
  datalayer_extended.stellantisECMP.InterlockOpen = battery_InterlockOpen;
  datalayer_extended.stellantisECMP.pid_welding_detection = pid_welding_detection;
  datalayer_extended.stellantisECMP.pid_reason_open = pid_reason_open;
  datalayer_extended.stellantisECMP.pid_contactor_status = pid_contactor_status;
  datalayer_extended.stellantisECMP.pid_negative_contactor_control = pid_negative_contactor_control;
  datalayer_extended.stellantisECMP.pid_negative_contactor_status = pid_negative_contactor_status;
  datalayer_extended.stellantisECMP.pid_positive_contactor_control = pid_positive_contactor_control;
  datalayer_extended.stellantisECMP.pid_positive_contactor_status = pid_positive_contactor_status;
  datalayer_extended.stellantisECMP.pid_contactor_negative = pid_contactor_negative;
  datalayer_extended.stellantisECMP.pid_contactor_positive = pid_contactor_positive;
  datalayer_extended.stellantisECMP.pid_precharge_relay_control = pid_precharge_relay_control;
  datalayer_extended.stellantisECMP.pid_precharge_relay_status = pid_precharge_relay_status;
  datalayer_extended.stellantisECMP.pid_recharge_status = pid_recharge_status;
  datalayer_extended.stellantisECMP.pid_delta_temperature = pid_delta_temperature;
  datalayer_extended.stellantisECMP.pid_coldest_module = pid_coldest_module;
  datalayer_extended.stellantisECMP.pid_lowest_temperature = pid_lowest_temperature;
  datalayer_extended.stellantisECMP.pid_average_temperature = pid_average_temperature;
  datalayer_extended.stellantisECMP.pid_highest_temperature = pid_highest_temperature;
  datalayer_extended.stellantisECMP.pid_hottest_module = pid_hottest_module;
  datalayer_extended.stellantisECMP.pid_avg_cell_voltage = pid_avg_cell_voltage;
  datalayer_extended.stellantisECMP.pid_current = pid_current;
  datalayer_extended.stellantisECMP.pid_insulation_res_neg = pid_insulation_res_neg;
  datalayer_extended.stellantisECMP.pid_insulation_res_pos = pid_insulation_res_pos;
  datalayer_extended.stellantisECMP.pid_max_current_10s = pid_max_current_10s;
  datalayer_extended.stellantisECMP.pid_max_discharge_10s = pid_max_discharge_10s;
  datalayer_extended.stellantisECMP.pid_max_discharge_30s = pid_max_discharge_30s;
  datalayer_extended.stellantisECMP.pid_max_charge_10s = pid_max_charge_10s;
  datalayer_extended.stellantisECMP.pid_max_charge_30s = pid_max_charge_30s;
  datalayer_extended.stellantisECMP.pid_energy_capacity = pid_energy_capacity;
  datalayer_extended.stellantisECMP.pid_highest_cell_voltage_num = pid_highest_cell_voltage_num;
  datalayer_extended.stellantisECMP.pid_lowest_cell_voltage_num = pid_lowest_cell_voltage_num;
  datalayer_extended.stellantisECMP.pid_sum_of_cells = pid_sum_of_cells;
  datalayer_extended.stellantisECMP.pid_cell_min_capacity = pid_cell_min_capacity;
  datalayer_extended.stellantisECMP.pid_cell_voltage_measurement_status = pid_cell_voltage_measurement_status;
  datalayer_extended.stellantisECMP.pid_insulation_res = pid_insulation_res;
  datalayer_extended.stellantisECMP.pid_pack_voltage = pid_pack_voltage;
  datalayer_extended.stellantisECMP.pid_high_cell_voltage = pid_high_cell_voltage;
  datalayer_extended.stellantisECMP.pid_low_cell_voltage = pid_low_cell_voltage;
  datalayer_extended.stellantisECMP.pid_battery_energy = pid_battery_energy;
  datalayer_extended.stellantisECMP.pid_crash_counter = pid_crash_counter;
  datalayer_extended.stellantisECMP.pid_wire_crash = pid_wire_crash;
  datalayer_extended.stellantisECMP.pid_CAN_crash = pid_CAN_crash;
  datalayer_extended.stellantisECMP.pid_history_data = pid_history_data;
  datalayer_extended.stellantisECMP.pid_lowsoc_counter = pid_lowsoc_counter;
  datalayer_extended.stellantisECMP.pid_last_can_failure_detail = pid_last_can_failure_detail;
  datalayer_extended.stellantisECMP.pid_hw_version_num = pid_hw_version_num;
  datalayer_extended.stellantisECMP.pid_sw_version_num = pid_sw_version_num;
  datalayer_extended.stellantisECMP.pid_factory_mode_control = pid_factory_mode_control;
  memcpy(datalayer_extended.stellantisECMP.pid_battery_serial, pid_battery_serial, sizeof(pid_battery_serial));
  //uint8_t pid_battery_serial[13] = {0};
  datalayer_extended.stellantisECMP.pid_aux_fuse_state = pid_aux_fuse_state;
  datalayer_extended.stellantisECMP.pid_battery_state = pid_battery_state;
  datalayer_extended.stellantisECMP.pid_precharge_short_circuit = pid_precharge_short_circuit;
  datalayer_extended.stellantisECMP.pid_eservice_plug_state = pid_eservice_plug_state;
  datalayer_extended.stellantisECMP.pid_mainfuse_state = pid_mainfuse_state;
  datalayer_extended.stellantisECMP.pid_most_critical_fault = pid_most_critical_fault;
  datalayer_extended.stellantisECMP.pid_current_time = pid_current_time;
  datalayer_extended.stellantisECMP.pid_time_sent_by_car = pid_time_sent_by_car;
  datalayer_extended.stellantisECMP.pid_12v = pid_12v;
  datalayer_extended.stellantisECMP.pid_12v_abnormal = pid_12v_abnormal;
  datalayer_extended.stellantisECMP.pid_hvil_in_voltage = pid_hvil_in_voltage;
  datalayer_extended.stellantisECMP.pid_hvil_out_voltage = pid_hvil_out_voltage;
  datalayer_extended.stellantisECMP.pid_hvil_state = pid_hvil_state;
  datalayer_extended.stellantisECMP.pid_bms_state = pid_bms_state;
  datalayer_extended.stellantisECMP.pid_vehicle_speed = pid_vehicle_speed;
  datalayer_extended.stellantisECMP.pid_time_spent_over_55c = pid_time_spent_over_55c;
  datalayer_extended.stellantisECMP.pid_contactor_closing_counter = pid_contactor_closing_counter;
  datalayer_extended.stellantisECMP.pid_date_of_manufacture = pid_date_of_manufacture;
  datalayer_extended.stellantisECMP.pid_SOH_cell_1 = pid_SOH_cell_1;
  // Update extended datalayer for MysteryVan
  datalayer_extended.stellantisECMP.MysteryVan = MysteryVan;
  datalayer_extended.stellantisECMP.CONTACTORS_STATE = CONTACTORS_STATE;
  datalayer_extended.stellantisECMP.CrashMemorized = HV_BATT_CRASH_MEMORIZED;
  datalayer_extended.stellantisECMP.CONTACTOR_OPENING_REASON = CONTACTOR_OPENING_REASON;
  datalayer_extended.stellantisECMP.TBMU_FAULT_TYPE = TBMU_FAULT_TYPE;
  datalayer_extended.stellantisECMP.HV_BATT_FC_INSU_MINUS_RES = HV_BATT_FC_INSU_MINUS_RES;
  datalayer_extended.stellantisECMP.HV_BATT_FC_INSU_PLUS_RES = HV_BATT_FC_INSU_PLUS_RES;
  datalayer_extended.stellantisECMP.HV_BATT_FC_VHL_INSU_PLUS_RES = HV_BATT_FC_VHL_INSU_PLUS_RES;
  datalayer_extended.stellantisECMP.HV_BATT_ONLY_INSU_MINUS_RES = HV_BATT_ONLY_INSU_MINUS_RES;
  datalayer_extended.stellantisECMP.HV_BATT_ONLY_INSU_MINUS_RES = HV_BATT_ONLY_INSU_MINUS_RES;
  datalayer_extended.stellantisECMP.ALERT_CELL_POOR_CONSIST = ALERT_CELL_POOR_CONSIST;
  datalayer_extended.stellantisECMP.ALERT_OVERCHARGE = ALERT_OVERCHARGE;
  datalayer_extended.stellantisECMP.ALERT_BATT = ALERT_BATT;
  datalayer_extended.stellantisECMP.ALERT_LOW_SOC = ALERT_LOW_SOC;
  datalayer_extended.stellantisECMP.ALERT_HIGH_SOC = ALERT_HIGH_SOC;
  datalayer_extended.stellantisECMP.ALERT_SOC_JUMP = ALERT_SOC_JUMP;
  datalayer_extended.stellantisECMP.ALERT_TEMP_DIFF = ALERT_TEMP_DIFF;
  datalayer_extended.stellantisECMP.ALERT_HIGH_TEMP = ALERT_HIGH_TEMP;
  datalayer_extended.stellantisECMP.ALERT_OVERVOLTAGE = ALERT_OVERVOLTAGE;
  datalayer_extended.stellantisECMP.ALERT_CELL_OVERVOLTAGE = ALERT_CELL_OVERVOLTAGE;
  datalayer_extended.stellantisECMP.ALERT_CELL_UNDERVOLTAGE = ALERT_CELL_UNDERVOLTAGE;

  if (battery_InterlockOpen) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  if (pid_12v < 11000) {
    set_event(EVENT_12V_LOW, 11);
  } else {
    clear_event(EVENT_12V_LOW);
  }

  if (pid_reason_open == 7) {  //Invalid status
    set_event(EVENT_CONTACTOR_OPEN, 0);
  } else {
    clear_event(EVENT_CONTACTOR_OPEN);
  }
}

void EcmpBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x2D4:  //MysteryVan 50/75kWh platform (TBMU 100ms periodic)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      MysteryVan = true;
      SOE_MAX_CURRENT_TEMP = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];                       // (Wh, 0-200000)
      FRONT_MACHINE_POWER_LIMIT = (rx_frame.data.u8[4] << 6) | ((rx_frame.data.u8[5] & 0xFC) >> 2);  // (W 0-1000000)
      REAR_MACHINE_POWER_LIMIT = ((rx_frame.data.u8[5] & 0x03) << 12) | (rx_frame.data.u8[6] << 4) |
                                 ((rx_frame.data.u8[7] & 0xF0) >> 4);  // (W 0-1000000)
      break;
    case 0x3B4:  //MysteryVan 50/75kWh platform (TBMU 100ms periodic)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      COUNTER_3B4 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x2F4:  //MysteryVan 50/75kWh platform (Event triggered when charging)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //TBMU_EVSE_DC_MES_VOLTAGE = (rx_frame.data.u8[0] << 6) | (rx_frame.data.u8[1] >> 2);           //V 0-1000 //Fastcharger info, not needed for BE
      //TBMU_EVSE_DC_MIN_VOLTAGE = ((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[2];         //V 0-1000 //Fastcharger info, not needed for BE
      //TBMU_EVSE_DC_MES_CURRENT = (rx_frame.data.u8[3] << 4) | ((rx_frame.data.u8[4] & 0xF0) >> 4);  //A -2000 - 2000 //Fastcharger info, not needed for BE
      //TBMU_EVSE_CHRG_REQ = (rx_frame.data.u8[4] & 0x0C) >> 2;  //00 No request, 01 Stop request //Fastcharger info, not needed for BE
      //HV_STORAGE_MAX_I = ((rx_frame.data.u8[4] & 0x03) << 12) | (rx_frame.data.u8[5] << 2) | //Fastcharger info, not needed for BE
      //((rx_frame.data.u8[6] & 0xC0) >> 6);  //A -2000 - 2000
      //TBMU_EVSE_DC_MAX_POWER = ((rx_frame.data.u8[6] & 0x3F) << 8) | rx_frame.data.u8[7];  //W -1000000 - 0 //Fastcharger info, not needed for BE
      break;
    case 0x3F4:  //MysteryVan 50/75kWh platform (Temperature sensors)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_PEAK_DISCH_POWER_HD = (rx_frame.data.u8[1] << 6) | (rx_frame.data.u8[2] >> 2);  //0-1000000 W
      HV_BATT_PEAK_CH_POWER_HD = ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) |
                                 ((rx_frame.data.u8[4] & 0xF0) >> 4);  // -1000000 - 0 W
      HV_BATT_NOM_CH_POWER_HD = ((rx_frame.data.u8[4] & 0x0F) << 12) | (rx_frame.data.u8[5] << 6) |
                                ((rx_frame.data.u8[6] & 0xC0) >> 6);  // -1000000 - 0 W
      MAX_ALLOW_CHRG_CURRENT = ((rx_frame.data.u8[6] & 0x3F) << 8) | rx_frame.data.u8[7];
      CHECKSUM_FRAME_554 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0xE
      COUNTER_554 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x373:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      COUNTER_373 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x4F4:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      COUNTER_4F4 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x414:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_REAL_POWER_HD = (rx_frame.data.u8[1] << 7) | (rx_frame.data.u8[2] >> 1);
      MAX_ALLOW_CHRG_POWER =
          ((rx_frame.data.u8[2] & 0x01) << 13) | (rx_frame.data.u8[3] << 5) | ((rx_frame.data.u8[4] & 0xF8) >> 3);
      MAX_ALLOW_DISCHRG_POWER =
          ((rx_frame.data.u8[5] & 0x07) << 11) | (rx_frame.data.u8[6] << 3) | ((rx_frame.data.u8[7] & 0xE0) >> 5);
      CHECKSUM_FRAME_414 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0x9
      COUNTER_414 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x353:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_COP_VOLTAGE =
          (rx_frame.data.u8[1] << 5) | (rx_frame.data.u8[2] >> 3);  //Real voltage HV battery (dV, 0-5000)
      HV_BATT_COP_CURRENT =
          (rx_frame.data.u8[3] << 5) | (rx_frame.data.u8[4] >> 3);  //High resolution battery current (dA, -4000 - 4000)
      CHECKSUM_FRAME_353 = (rx_frame.data.u8[0] & 0xF0) >> 4;       //Frame checksum 0xB
      COUNTER_353 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x474:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      COUNTER_474 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x574:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_FC_INSU_MINUS_RES = (rx_frame.data.u8[0] << 5) | (rx_frame.data.u8[1] >> 3);  //kOhm (0-60000)
      HV_BATT_FC_VHL_INSU_PLUS_RES =
          ((rx_frame.data.u8[1] & 0x07) << 10) | (rx_frame.data.u8[2] << 2) | ((rx_frame.data.u8[3] & 0xC0) >> 6);
      HV_BATT_FC_INSU_PLUS_RES = (rx_frame.data.u8[5] << 4) | (rx_frame.data.u8[6] >> 4);
      HV_BATT_ONLY_INSU_MINUS_RES = ((rx_frame.data.u8[3] & 0x3F) << 7) | (rx_frame.data.u8[4] >> 1);
      break;
    case 0x583:  //MysteryVan 50/75kWh platform (CAN-FD also?)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      MIN_ALLOW_DISCHRG_VOLTAGE = (rx_frame.data.u8[1] << 3) | (rx_frame.data.u8[2] >> 5);  //V (0-1000)
      //EVSE_DC_MAX_CURRENT = ((rx_frame.data.u8[2] & 0x1F) << 5) | (rx_frame.data.u8[3] >> 3); //Fastcharger info, not needed for BE
      //TBMU_EVSE_DC_MAX_VOLTAGE //Fastcharger info, not needed for BE
      //TBMU_MAX_CHRG_SCKT_TEMP //Fastcharger info, not needed for BE
      //DC_CHARGE_MODE_AVAIL //Fastcharger info, not needed for BE
      //BIDIR_V2HG_MODE_AVAIL //Fastcharger info, not needed for BE
      //TBMU_CHRG_CONN_CONF //Fastcharger info, not needed for BE
      //EVSE_GRID_FAULT //Fastcharger info, not needed for BE
      CHECKSUM_FRAME_314 = (rx_frame.data.u8[0] & 0xF0) >> 4;  //Frame checksum 0x8
      COUNTER_314 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x254:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //HV_BATT_SOE_MAX_HR = frame6 & frame7 //Only on FD-CAN variant of the message. FD has length 7, non-fd 5
      HV_BATT_NOMINAL_DISCH_CURR_HD = (rx_frame.data.u8[0] << 7) | (rx_frame.data.u8[1] >> 1);  //dA (0-20000)
      HV_BATT_PEAK_DISCH_CURR_HD = (rx_frame.data.u8[2] << 7) | (rx_frame.data.u8[3] >> 1);     //dA (0-20000)
      HV_BATT_STABLE_DISCH_CURR_HD = (rx_frame.data.u8[4] << 7) | (rx_frame.data.u8[5] >> 1);   //dA (0-20000)
      break;
    case 0x2B4:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_NOMINAL_CHARGE_CURR_HD = (rx_frame.data.u8[0] << 7) | (rx_frame.data.u8[1] >> 1);
      HV_BATT_PEAK_CHARGE_CURR_HD = (rx_frame.data.u8[2] << 7) | (rx_frame.data.u8[3] >> 1);
      HV_BATT_STABLE_CHARGE_CURR_HD = (rx_frame.data.u8[4] << 7) | (rx_frame.data.u8[5] >> 1);
      break;
    case 0x4D4:  //MysteryVan 50/75kWh platform
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HV_BATT_STABLE_CHARGE_POWER_HD = (rx_frame.data.u8[0] << 6) | (rx_frame.data.u8[1] >> 2);
      HV_BATT_STABLE_DISCH_POWER_HD =
          ((rx_frame.data.u8[2] & 0x03) << 12) | (rx_frame.data.u8[3] << 4) | ((rx_frame.data.u8[4] & 0xF0) >> 4);
      HV_BATT_NOMINAL_DISCH_POWER_HD =
          ((rx_frame.data.u8[4] & 0x0F) << 10) | (rx_frame.data.u8[5] << 2) | ((rx_frame.data.u8[6] & 0xC0) >> 6);
      MAX_ALLOW_DISCHRG_CURRENT = ((rx_frame.data.u8[6] & 0x3F) << 5) | (rx_frame.data.u8[7] >> 3);
      RC01_PERM_SYNTH_TBMU = (rx_frame.data.u8[7] & 0x04) >> 2;  //TBMU Readiness Code synthesis
      CHECKSUM_FRAME_4D4 = (rx_frame.data.u8[0] & 0xF0) >> 4;    //Frame checksum 0x5
      COUNTER_4D4 = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x125:  //Common eCMP
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_soc = (rx_frame.data.u8[0] << 2) |
                    (rx_frame.data.u8[1] >> 6);  // Byte1, bit 7 length 10 (0x3FE when abnormal) (0-1000 ppt)
      battery_MainConnectorState = ((rx_frame.data.u8[2] & 0x18) >>
                                    3);  //Byte2 , bit 4, length 2 ((00 contactors open, 01 precharged, 11 invalid))
      battery_voltage =
          (rx_frame.data.u8[3] << 1) | (rx_frame.data.u8[4] >> 7);  //Byte 4, bit 7, length 9 (0x1FE if invalid)
      battery_current = (((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]) - 600;  // TODO: Test
      break;
    case 0x127:  //DFM specific
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_AllowedMaxChargeCurrent =
          (rx_frame.data.u8[0] << 2) |
          ((rx_frame.data.u8[1] & 0xC0) >> 6);  //Byte 1, bit 7, length 10 (0-600A) [0x3FF if invalid]
      battery_AllowedMaxDischargeCurrent =
          ((rx_frame.data.u8[2] & 0x3F) << 4) |
          (rx_frame.data.u8[3] >> 4);  //Byte 2, bit 5, length 10 (0-600A) [0x3FF if invalid]
      break;
    case 0x129:  //PSA specific
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x31B:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_InterlockOpen = ((rx_frame.data.u8[1] & 0x10) >> 4);  //Best guess, seems to work?
      //TODO: frame7 contains checksum, we can use this to check for CAN message corruption
      break;
    case 0x358:  //Common
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_insulation_failure_diag = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //Unsure if this is right position
      //byte pos 6, bit pos 7, signal lenth 3
      //0 = no failure, 1 = symmetric failure, 4 = invalid value , forbidden value 5-7
      break;
    case 0x6D0:  //Common
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_insulationResistanceKOhm =
          (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];  //Byte 2, bit 7, length 16 (0-60000 kOhm)
      break;
    case 0x6D1:
      break;
    case 0x6D2:
      break;
    case 0x6D3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
    case 0x6E0:
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
    case 0x6E3:
      break;
    case 0x6E4:
      break;
    case 0x6E5:
      break;
    case 0x6E6:
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
      //Not available on e-C4
      break;
    case 0x6ED:
      cellvoltages[32] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[33] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[34] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[35] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EE:
      cellvoltages[36] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[37] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[38] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[39] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6EF:
      cellvoltages[40] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[41] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[42] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[43] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F0:
      cellvoltages[44] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[45] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[46] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[47] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F1:
      cellvoltages[48] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[49] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[50] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[51] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F2:
      cellvoltages[52] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[53] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[54] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[55] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F3:
      cellvoltages[56] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[57] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[58] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[59] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F4:
      cellvoltages[60] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[61] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[62] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[63] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F5:
      cellvoltages[64] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[65] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[66] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[67] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F6:
      cellvoltages[68] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[69] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[70] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[71] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F7:
      cellvoltages[72] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[73] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[74] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[75] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F8:
      cellvoltages[76] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[77] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[78] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[79] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6F9:
      cellvoltages[80] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[81] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[82] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[83] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FA:
      cellvoltages[84] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[85] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[86] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[87] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FB:
      cellvoltages[88] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[89] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[90] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[91] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FC:
      cellvoltages[92] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[93] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[94] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[95] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FD:
      cellvoltages[96] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[97] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[98] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[99] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FE:
      cellvoltages[100] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[101] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[102] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[103] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x6FF:
      cellvoltages[104] = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      cellvoltages[105] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      cellvoltages[106] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      cellvoltages[107] = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, 108 * sizeof(uint16_t));
      break;
    case 0x694:  // Poll reply
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      // Handle user requested functionality first if ongoing
      if (datalayer_extended.stellantisECMP.UserRequestDisableIsoMonitoring) {
        if ((rx_frame.data.u8[0] == 0x06) && (rx_frame.data.u8[1] == 0x50) && (rx_frame.data.u8[2] == 0x03)) {
          //06,50,03,00,C8,00,14,00,
          DisableIsoMonitoringStatemachine = 2;  //Send ECMP_ACK_MESSAGE (02 3e 00)
        }
        if ((rx_frame.data.u8[0] == 0x02) && (rx_frame.data.u8[1] == 0x7E) && (rx_frame.data.u8[2] == 0x00)) {
          //Expected 02,7E,00
          DisableIsoMonitoringStatemachine = 4;  //Send ECMP_FACTORY_MODE_ACTIVATION next loop
        }
        if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x6E) && (rx_frame.data.u8[2] == 0xD9)) {
          //Factory mode ENTRY: 2E.D9.00.01
          DisableIsoMonitoringStatemachine = 6;  //Send ECMP_DISABLE_ISOLATION_REQ next loop
        }
        if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x7F) && (rx_frame.data.u8[2] == 0x2E)) {
          //Factory mode fails to enter with 7F
          set_event(EVENT_PID_FAILED, rx_frame.data.u8[2]);
          DisableIsoMonitoringStatemachine =
              6;  //Send ECMP_DISABLE_ISOLATION_REQ next loop (pointless, since it will fail)
        }
        if ((rx_frame.data.u8[0] == 0x04) && (rx_frame.data.u8[1] == 0x31) && (rx_frame.data.u8[2] == 0x02)) {
          //Disable isolation successful 04 31 02 df e1
          DisableIsoMonitoringStatemachine = COMPLETED_STATE;
          datalayer_extended.stellantisECMP.UserRequestDisableIsoMonitoring = false;
          timeSpentDisableIsoMonitoring = COMPLETED_STATE;
        }
        if ((rx_frame.data.u8[0] == 0x03) && (rx_frame.data.u8[1] == 0x7F) && (rx_frame.data.u8[2] == 0x31)) {
          //Disable Isolation fails to enter with 7F
          set_event(EVENT_PID_FAILED, rx_frame.data.u8[2]);
          DisableIsoMonitoringStatemachine = COMPLETED_STATE;
          datalayer_extended.stellantisECMP.UserRequestDisableIsoMonitoring = false;
          timeSpentDisableIsoMonitoring = COMPLETED_STATE;
        }

      } else if (datalayer_extended.stellantisECMP.UserRequestContactorReset) {
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
          datalayer_extended.stellantisECMP.UserRequestContactorReset = false;
          timeSpentContactorReset = COMPLETED_STATE;
        }

      } else if (datalayer_extended.stellantisECMP.UserRequestCollisionReset) {
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
            datalayer_extended.stellantisECMP.UserRequestCollisionReset = false;
            timeSpentCollisionReset = COMPLETED_STATE;
          }
        }

      } else if (datalayer_extended.stellantisECMP.UserRequestIsolationReset) {
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
            datalayer_extended.stellantisECMP.UserRequestIsolationReset = false;
            timeSpentIsolationReset = COMPLETED_STATE;
          }
        }

      } else {  //Normal PID polling ongoing

        if (rx_frame.data.u8[0] == 0x10) {  //Multiframe response, send ACK
          transmit_can_frame(&ECMP_ACK);
          //Multiframe has the poll reply slightly different location
          incoming_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        }

        if (rx_frame.data.u8[0] == 0x11) {  //One line response, with special handling
          incoming_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];

          switch (incoming_poll) {  //One line responses, special
            case PID_HISTORY_DATA:
              pid_history_data = ((rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            default:
              break;
          }
        }

        if (rx_frame.data.u8[0] < 0x10) {  //One line responses
          incoming_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

          switch (incoming_poll) {  //One line responses
            case PID_WELD_CHECK:
              pid_welding_detection = (rx_frame.data.u8[4]);
              break;
            case PID_CONT_REASON_OPEN:
              pid_reason_open = (rx_frame.data.u8[4]);
              break;
            case PID_CONTACTOR_STATUS:
              pid_contactor_status = (rx_frame.data.u8[4]);
              break;
            case PID_NEG_CONT_CONTROL:
              pid_negative_contactor_control = (rx_frame.data.u8[4]);
              break;
            case PID_NEG_CONT_STATUS:
              pid_negative_contactor_status = (rx_frame.data.u8[4]);
              break;
            case PID_POS_CONT_CONTROL:
              pid_positive_contactor_control = (rx_frame.data.u8[4]);
              break;
            case PID_POS_CONT_STATUS:
              pid_positive_contactor_status = (rx_frame.data.u8[4]);
              break;
            case PID_CONTACTOR_NEGATIVE:
              pid_contactor_negative = (rx_frame.data.u8[4]);
              break;
            case PID_CONTACTOR_POSITIVE:
              pid_contactor_positive = (rx_frame.data.u8[4]);
              break;
            case PID_PRECHARGE_RELAY_CONTROL:
              pid_precharge_relay_control = (rx_frame.data.u8[4]);
              break;
            case PID_PRECHARGE_RELAY_STATUS:
              pid_precharge_relay_status = (rx_frame.data.u8[4]);
              break;
            case PID_RECHARGE_STATUS:
              pid_recharge_status = (rx_frame.data.u8[4]);
              break;
            case PID_DELTA_TEMPERATURE:
              pid_delta_temperature = (rx_frame.data.u8[4]);
              break;
            case PID_COLDEST_MODULE:
              pid_coldest_module = (rx_frame.data.u8[4]);
              break;
            case PID_LOWEST_TEMPERATURE:
              pid_lowest_temperature = (rx_frame.data.u8[4] - 40);
              break;
            case PID_AVERAGE_TEMPERATURE:
              pid_average_temperature = (rx_frame.data.u8[4] - 40);
              break;
            case PID_HIGHEST_TEMPERATURE:
              pid_highest_temperature = (rx_frame.data.u8[4] - 40);
              break;
            case PID_HOTTEST_MODULE:
              pid_hottest_module = (rx_frame.data.u8[4]);
              break;
            case PID_AVG_CELL_VOLTAGE:
              pid_avg_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_CURRENT:
              pid_current = -(((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) |
                               rx_frame.data.u8[7]) -
                              76800) *
                            20;
              break;
            case PID_INSULATION_NEG:
              pid_insulation_res_neg = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                        (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_INSULATION_POS:
              pid_insulation_res_pos = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                        (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_MAX_CURRENT_10S:
              pid_max_current_10s = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                     (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_MAX_DISCHARGE_10S:
              pid_max_discharge_10s = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                       (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_MAX_DISCHARGE_30S:
              pid_max_discharge_30s = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                       (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_MAX_CHARGE_10S:
              pid_max_charge_10s = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                    (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_MAX_CHARGE_30S:
              pid_max_charge_30s = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                    (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_ENERGY_CAPACITY:
              pid_energy_capacity = (rx_frame.data.u8[4] << 16) | (rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6]);
              break;
            case PID_HIGH_CELL_NUM:
              pid_highest_cell_voltage_num = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_LOW_CELL_NUM:
              pid_lowest_cell_voltage_num = (rx_frame.data.u8[4]);
              break;
            case PID_SUM_OF_CELLS:
              pid_sum_of_cells = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 2;
              break;
            case PID_CELL_MIN_CAPACITY:
              pid_cell_min_capacity = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_CELL_VOLTAGE_MEAS_STATUS:
              pid_cell_voltage_measurement_status = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                                     (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_INSULATION_RES:
              pid_insulation_res = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                    (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_PACK_VOLTAGE:
              pid_pack_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 2;
              break;
            case PID_HIGH_CELL_VOLTAGE:
              pid_high_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_ALL_CELL_VOLTAGES:
              //Multi frame, handled in other function
              break;
            case PID_LOW_CELL_VOLTAGE:
              pid_low_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_BATTERY_ENERGY:
              pid_battery_energy = (rx_frame.data.u8[4]);
              break;
            case PID_CELLBALANCE_STATUS:
              //Multi frame?, handled in other function
              break;
            case PID_CELLBALANCE_HWERR_MASK:
              //Multi frame, handled in other function
              break;
            case PID_CRASH_COUNTER:
              pid_crash_counter = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                   (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_WIRE_CRASH:
              pid_wire_crash = (rx_frame.data.u8[4]);
              break;
            case PID_CAN_CRASH:
              pid_CAN_crash = (rx_frame.data.u8[4]);
              break;
            case PID_HISTORY_DATA:
              //handled in 0x11 handler
              break;
            case PID_LOWSOC_COUNTER:
              pid_lowsoc_counter = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_LAST_CAN_FAILURE_DETAIL:
              pid_last_can_failure_detail = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                             (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_HW_VERSION_NUM:
              //pid_hw_version_num = rx_frame.data.u8[4]; Not available on all batteries
              break;
            case PID_SW_VERSION_NUM:
              //pid_sw_version_num = rx_frame.data.u8[4]; Not available on all batteries
              break;
            case PID_FACTORY_MODE_CONTROL:
              pid_factory_mode_control = rx_frame.data.u8[4];
              break;
            case PID_BATTERY_SERIAL:
              // Multiframe, handled separately down below
              break;
            case PID_AUX_FUSE_STATE:
              pid_aux_fuse_state = rx_frame.data.u8[4];
              break;
            case PID_BATTERY_STATE:
              pid_battery_state = rx_frame.data.u8[4];
              break;
            case PID_PRECHARGE_SHORT_CIRCUIT:
              pid_precharge_short_circuit = rx_frame.data.u8[4];
              break;
            case PID_ESERVICE_PLUG_STATE:
              pid_eservice_plug_state = rx_frame.data.u8[4];
              break;
            case PID_MAINFUSE_STATE:
              pid_mainfuse_state = rx_frame.data.u8[4];
              break;
            case PID_MOST_CRITICAL_FAULT:
              pid_most_critical_fault = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
              break;
            case PID_CURRENT_TIME:
              //Multiframe, handled separately down below
              break;
            case PID_TIME_SENT_BY_CAR:
              pid_time_sent_by_car = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                      (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_12V:
              pid_12v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_12V_ABNORMAL:
              pid_12v_abnormal = rx_frame.data.u8[4];
              break;
            case PID_HVIL_IN_VOLTAGE:
              pid_hvil_in_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_HVIL_OUT_VOLTAGE:
              pid_hvil_out_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
              break;
            case PID_HVIL_STATE:
              pid_hvil_state = rx_frame.data.u8[4];
              break;
            case PID_BMS_STATE:
              pid_bms_state = rx_frame.data.u8[4];
              break;
            case PID_VEHICLE_SPEED:
              pid_vehicle_speed = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                   (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_TIME_SPENT_OVER_55C:
              pid_time_spent_over_55c = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                         (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_CONTACTOR_CLOSING_COUNTER:
              pid_contactor_closing_counter = ((rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) |
                                               (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
              break;
            case PID_DATE_OF_MANUFACTURE:
              pid_date_of_manufacture =
                  ((rx_frame.data.u8[4] << 16) | (rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6]));
              break;
            default:
              break;
          }
        }

        switch (incoming_poll)  //Multiframe responses
        {
          case PID_ALL_CELL_SOH:
            switch (rx_frame.data.u8[0]) {
              case 0x10:
                pid_SOH_cell_1 = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
                break;
              case 0x21:
                break;
              case 0x22:
                break;
              case 0x23:
                break;
              case 0x24:
                break;
              case 0x25:
                break;
              case 0x26:
                break;
              case 0x27:
                break;
              case 0x28:
                break;
              case 0x29:
                break;
              default:
                break;
            }
            break;
          case PID_ALL_CELL_VOLTAGES:
            switch (rx_frame.data.u8[0]) {
              case 0x10:
                break;
              case 0x21:
                break;
              case 0x22:
                break;
              case 0x23:
                break;
              case 0x24:
                break;
              case 0x25:
                break;
              case 0x26:
                break;
              case 0x27:
                break;
              case 0x28:
                break;
              case 0x29:
                break;
              default:
                break;
            }
            break;
          case PID_CELLBALANCE_STATUS:
            switch (rx_frame.data.u8[0]) {
              case 0x10:
                break;
              case 0x21:
                break;
              case 0x22:
                break;
              case 0x23:
                break;
              default:
                break;
            }
            break;
          case PID_CELLBALANCE_HWERR_MASK:
            switch (rx_frame.data.u8[0]) {
              case 0x10:
                break;
              case 0x21:
                break;
              case 0x22:
                break;
              case 0x23:
                break;
              default:
                break;
            }
            break;
          case PID_CURRENT_TIME:
            switch (rx_frame.data.u8[0]) {
              case 0x10:
                pid_current_time = (pid_current_time | rx_frame.data.u8[7]);
                break;
              case 0x21:
                pid_current_time = (rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                   (rx_frame.data.u8[1] << 8) | pid_current_time;
                break;
              default:
                break;
            }
            break;
          case PID_BATTERY_SERIAL:
            switch (rx_frame.data.u8[0]) {
              case 0x10:  //10,11,62,D9,01,33,34,41,
                pid_battery_serial[0] = rx_frame.data.u8[5];
                pid_battery_serial[1] = rx_frame.data.u8[6];
                pid_battery_serial[2] = rx_frame.data.u8[7];
                break;
              case 0x21:  //21,41,41,46,30,30,30,32,
                pid_battery_serial[3] = rx_frame.data.u8[1];
                pid_battery_serial[4] = rx_frame.data.u8[2];
                pid_battery_serial[5] = rx_frame.data.u8[3];
                pid_battery_serial[6] = rx_frame.data.u8[4];
                pid_battery_serial[7] = rx_frame.data.u8[5];
                pid_battery_serial[8] = rx_frame.data.u8[6];
                pid_battery_serial[9] = rx_frame.data.u8[7];
                break;
              case 0x22:  //22,32,33,31,00,00,00,00,
                pid_battery_serial[10] = rx_frame.data.u8[1];
                pid_battery_serial[11] = rx_frame.data.u8[2];
                pid_battery_serial[12] = rx_frame.data.u8[3];
                pid_battery_serial[13] = rx_frame.data.u8[4];
                break;
              default:
                break;
            }
            break;
          default:
            //Not a multiframe response, do nothing
            break;
        }
        break;
        default:
          break;
      }
  }
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

  // Send 250ms diagnostic CAN Messages
  if (currentMillis - previousMillis250 >= INTERVAL_250_MS) {
    previousMillis250 = currentMillis;

    //To be able to use the battery, isolation monitoring needs to be disabled
    //Failure to do this results in the contactors opening after 30 seconds with load
    if (datalayer_extended.stellantisECMP.UserRequestDisableIsoMonitoring) {
      if (DisableIsoMonitoringStatemachine == 0) {
        transmit_can_frame(&ECMP_DIAG_START);
        DisableIsoMonitoringStatemachine = 1;
      }
      if (DisableIsoMonitoringStatemachine == 2) {
        transmit_can_frame(&ECMP_ACK_MESSAGE);
        DisableIsoMonitoringStatemachine = 3;
      }
      if (DisableIsoMonitoringStatemachine == 4) {
        transmit_can_frame(&ECMP_FACTORY_MODE_ACTIVATION);
        DisableIsoMonitoringStatemachine = 5;
      }
      if (DisableIsoMonitoringStatemachine == 6) {
        transmit_can_frame(&ECMP_DISABLE_ISOLATION_REQ);
        DisableIsoMonitoringStatemachine = 7;
      }
      timeSpentDisableIsoMonitoring++;
      if (timeSpentDisableIsoMonitoring > 40) {  //Timeout, if command takes more than 10s to complete
        datalayer_extended.stellantisECMP.UserRequestDisableIsoMonitoring = false;
        DisableIsoMonitoringStatemachine = COMPLETED_STATE;
        timeSpentDisableIsoMonitoring = COMPLETED_STATE;
      }
    } else if (datalayer_extended.stellantisECMP.UserRequestContactorReset) {
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
        datalayer_extended.stellantisECMP.UserRequestContactorReset = false;
        ContactorResetStatemachine = COMPLETED_STATE;
        timeSpentContactorReset = COMPLETED_STATE;
      }

    } else if (datalayer_extended.stellantisECMP.UserRequestCollisionReset) {

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
        datalayer_extended.stellantisECMP.UserRequestCollisionReset = false;
        CollisionResetStatemachine = COMPLETED_STATE;
        timeSpentCollisionReset = COMPLETED_STATE;
      }

    } else if (datalayer_extended.stellantisECMP.UserRequestIsolationReset) {

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
          datalayer_extended.stellantisECMP.UserRequestIsolationReset = false;
          IsolationResetStatemachine = COMPLETED_STATE;
          timeSpentIsolationReset = COMPLETED_STATE;
          countIsolationReset = 0;
        }
      }

    } else {  //Normal PID polling goes here

      if (datalayer.battery.status.bms_status != FAULT) {  //Stop PID polling if battery is in FAULT mode

        switch (poll_state) {
          case PID_WELD_CHECK:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_WELD_CHECK & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_WELD_CHECK & 0x00FF);
            poll_state = PID_CONT_REASON_OPEN;
            break;
          case PID_CONT_REASON_OPEN:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CONT_REASON_OPEN & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CONT_REASON_OPEN & 0x00FF);
            poll_state = PID_CONTACTOR_STATUS;
            break;
          case PID_CONTACTOR_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CONTACTOR_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CONTACTOR_STATUS & 0x00FF);
            poll_state = PID_NEG_CONT_CONTROL;
            break;
          case PID_NEG_CONT_CONTROL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_NEG_CONT_CONTROL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_NEG_CONT_CONTROL & 0x00FF);
            poll_state = PID_NEG_CONT_STATUS;
            break;
          case PID_NEG_CONT_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_NEG_CONT_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_NEG_CONT_STATUS & 0x00FF);
            poll_state = PID_POS_CONT_CONTROL;
            break;
          case PID_POS_CONT_CONTROL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_POS_CONT_CONTROL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_POS_CONT_CONTROL & 0x00FF);
            poll_state = PID_POS_CONT_STATUS;
            break;
          case PID_POS_CONT_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_POS_CONT_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_POS_CONT_STATUS & 0x00FF);
            poll_state = PID_CONTACTOR_NEGATIVE;
            break;
          case PID_CONTACTOR_NEGATIVE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CONTACTOR_NEGATIVE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CONTACTOR_NEGATIVE & 0x00FF);
            poll_state = PID_CONTACTOR_POSITIVE;
            break;
          case PID_CONTACTOR_POSITIVE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CONTACTOR_POSITIVE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CONTACTOR_POSITIVE & 0x00FF);
            poll_state = PID_PRECHARGE_RELAY_CONTROL;
            break;
          case PID_PRECHARGE_RELAY_CONTROL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_PRECHARGE_RELAY_CONTROL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_PRECHARGE_RELAY_CONTROL & 0x00FF);
            poll_state = PID_PRECHARGE_RELAY_STATUS;
            break;
          case PID_PRECHARGE_RELAY_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_PRECHARGE_RELAY_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_PRECHARGE_RELAY_STATUS & 0x00FF);
            poll_state = PID_RECHARGE_STATUS;
            break;
          case PID_RECHARGE_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_RECHARGE_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_RECHARGE_STATUS & 0x00FF);
            poll_state = PID_DELTA_TEMPERATURE;
            break;
          case PID_DELTA_TEMPERATURE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_DELTA_TEMPERATURE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_DELTA_TEMPERATURE & 0x00FF);
            poll_state = PID_COLDEST_MODULE;
            break;
          case PID_COLDEST_MODULE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_COLDEST_MODULE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_COLDEST_MODULE & 0x00FF);
            poll_state = PID_LOWEST_TEMPERATURE;
            break;
          case PID_LOWEST_TEMPERATURE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_LOWEST_TEMPERATURE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_LOWEST_TEMPERATURE & 0x00FF);
            poll_state = PID_AVERAGE_TEMPERATURE;
            break;
          case PID_AVERAGE_TEMPERATURE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_AVERAGE_TEMPERATURE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_AVERAGE_TEMPERATURE & 0x00FF);
            poll_state = PID_HIGHEST_TEMPERATURE;
            break;
          case PID_HIGHEST_TEMPERATURE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HIGHEST_TEMPERATURE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HIGHEST_TEMPERATURE & 0x00FF);
            poll_state = PID_HOTTEST_MODULE;
            break;
          case PID_HOTTEST_MODULE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HOTTEST_MODULE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HOTTEST_MODULE & 0x00FF);
            poll_state = PID_AVG_CELL_VOLTAGE;
            break;
          case PID_AVG_CELL_VOLTAGE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_AVG_CELL_VOLTAGE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_AVG_CELL_VOLTAGE & 0x00FF);
            poll_state = PID_CURRENT;
            break;
          case PID_CURRENT:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CURRENT & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CURRENT & 0x00FF);
            poll_state = PID_INSULATION_NEG;
            break;
          case PID_INSULATION_NEG:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_INSULATION_NEG & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_INSULATION_NEG & 0x00FF);
            poll_state = PID_INSULATION_POS;
            break;
          case PID_INSULATION_POS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_INSULATION_POS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_INSULATION_POS & 0x00FF);
            poll_state = PID_MAX_CURRENT_10S;
            break;
          case PID_MAX_CURRENT_10S:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MAX_CURRENT_10S & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MAX_CURRENT_10S & 0x00FF);
            poll_state = PID_MAX_DISCHARGE_10S;
            break;
          case PID_MAX_DISCHARGE_10S:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MAX_DISCHARGE_10S & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MAX_DISCHARGE_10S & 0x00FF);
            poll_state = PID_MAX_DISCHARGE_30S;
            break;
          case PID_MAX_DISCHARGE_30S:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MAX_DISCHARGE_30S & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MAX_DISCHARGE_30S & 0x00FF);
            poll_state = PID_MAX_CHARGE_10S;
            break;
          case PID_MAX_CHARGE_10S:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MAX_CHARGE_10S & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MAX_CHARGE_10S & 0x00FF);
            poll_state = PID_MAX_CHARGE_30S;
            break;
          case PID_MAX_CHARGE_30S:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MAX_CHARGE_30S & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MAX_CHARGE_30S & 0x00FF);
            poll_state = PID_ENERGY_CAPACITY;
            break;
          case PID_ENERGY_CAPACITY:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_ENERGY_CAPACITY & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_ENERGY_CAPACITY & 0x00FF);
            poll_state = PID_HIGH_CELL_NUM;
            break;
          case PID_HIGH_CELL_NUM:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HIGH_CELL_NUM & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HIGH_CELL_NUM & 0x00FF);
            poll_state = PID_LOW_CELL_NUM;
            break;
          case PID_LOW_CELL_NUM:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_LOW_CELL_NUM & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_LOW_CELL_NUM & 0x00FF);
            poll_state = PID_SUM_OF_CELLS;
            break;
          case PID_SUM_OF_CELLS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_SUM_OF_CELLS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_SUM_OF_CELLS & 0x00FF);
            poll_state = PID_CELL_MIN_CAPACITY;
            break;
          case PID_CELL_MIN_CAPACITY:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CELL_MIN_CAPACITY & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CELL_MIN_CAPACITY & 0x00FF);
            poll_state = PID_CELL_VOLTAGE_MEAS_STATUS;
            break;
          case PID_CELL_VOLTAGE_MEAS_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CELL_VOLTAGE_MEAS_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CELL_VOLTAGE_MEAS_STATUS & 0x00FF);
            poll_state = PID_INSULATION_RES;
            break;
          case PID_INSULATION_RES:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_INSULATION_RES & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_INSULATION_RES & 0x00FF);
            poll_state = PID_PACK_VOLTAGE;
            break;
          case PID_PACK_VOLTAGE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_PACK_VOLTAGE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_PACK_VOLTAGE & 0x00FF);
            poll_state = PID_HIGH_CELL_VOLTAGE;
            break;
          case PID_HIGH_CELL_VOLTAGE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HIGH_CELL_VOLTAGE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HIGH_CELL_VOLTAGE & 0x00FF);
            poll_state = PID_ALL_CELL_VOLTAGES;
            break;
          case PID_ALL_CELL_VOLTAGES:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_ALL_CELL_VOLTAGES & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_ALL_CELL_VOLTAGES & 0x00FF);
            poll_state = PID_LOW_CELL_VOLTAGE;
            break;
          case PID_LOW_CELL_VOLTAGE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_LOW_CELL_VOLTAGE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_LOW_CELL_VOLTAGE & 0x00FF);
            poll_state = PID_BATTERY_ENERGY;
            break;
          case PID_BATTERY_ENERGY:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_BATTERY_ENERGY & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_BATTERY_ENERGY & 0x00FF);
            poll_state = PID_CELLBALANCE_STATUS;
            break;
          case PID_CELLBALANCE_STATUS:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CELLBALANCE_STATUS & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CELLBALANCE_STATUS & 0x00FF);
            poll_state = PID_CELLBALANCE_HWERR_MASK;
            break;
          case PID_CELLBALANCE_HWERR_MASK:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CELLBALANCE_HWERR_MASK & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CELLBALANCE_HWERR_MASK & 0x00FF);
            poll_state = PID_CRASH_COUNTER;
            break;
          case PID_CRASH_COUNTER:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CRASH_COUNTER & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CRASH_COUNTER & 0x00FF);
            poll_state = PID_WIRE_CRASH;
            break;
          case PID_WIRE_CRASH:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_WIRE_CRASH & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_WIRE_CRASH & 0x00FF);
            poll_state = PID_CAN_CRASH;
            break;
          case PID_CAN_CRASH:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CAN_CRASH & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CAN_CRASH & 0x00FF);
            poll_state = PID_HISTORY_DATA;
            break;
          case PID_HISTORY_DATA:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HISTORY_DATA & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HISTORY_DATA & 0x00FF);
            poll_state = PID_LOWSOC_COUNTER;
            break;
          case PID_LOWSOC_COUNTER:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_LOWSOC_COUNTER & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_LOWSOC_COUNTER & 0x00FF);
            poll_state = PID_LAST_CAN_FAILURE_DETAIL;
            break;
          case PID_LAST_CAN_FAILURE_DETAIL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_LAST_CAN_FAILURE_DETAIL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_LAST_CAN_FAILURE_DETAIL & 0x00FF);
            poll_state = PID_HW_VERSION_NUM;
            break;
          case PID_HW_VERSION_NUM:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HW_VERSION_NUM & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HW_VERSION_NUM & 0x00FF);
            poll_state = PID_SW_VERSION_NUM;
            break;
          case PID_SW_VERSION_NUM:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_SW_VERSION_NUM & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_SW_VERSION_NUM & 0x00FF);
            poll_state = PID_FACTORY_MODE_CONTROL;
            break;
          case PID_FACTORY_MODE_CONTROL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_FACTORY_MODE_CONTROL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_FACTORY_MODE_CONTROL & 0x00FF);
            poll_state = PID_BATTERY_SERIAL;
            break;
          case PID_BATTERY_SERIAL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_BATTERY_SERIAL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_BATTERY_SERIAL & 0x00FF);
            poll_state = PID_AUX_FUSE_STATE;
            break;
          case PID_AUX_FUSE_STATE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_AUX_FUSE_STATE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_AUX_FUSE_STATE & 0x00FF);
            poll_state = PID_BATTERY_STATE;
            break;
          case PID_BATTERY_STATE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_BATTERY_STATE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_BATTERY_STATE & 0x00FF);
            poll_state = PID_PRECHARGE_SHORT_CIRCUIT;
            break;
          case PID_PRECHARGE_SHORT_CIRCUIT:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_PRECHARGE_SHORT_CIRCUIT & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_PRECHARGE_SHORT_CIRCUIT & 0x00FF);
            poll_state = PID_ESERVICE_PLUG_STATE;
            break;
          case PID_ESERVICE_PLUG_STATE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_ESERVICE_PLUG_STATE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_ESERVICE_PLUG_STATE & 0x00FF);
            poll_state = PID_MAINFUSE_STATE;
            break;
          case PID_MAINFUSE_STATE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MAINFUSE_STATE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MAINFUSE_STATE & 0x00FF);
            poll_state = PID_MOST_CRITICAL_FAULT;
            break;
          case PID_MOST_CRITICAL_FAULT:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_MOST_CRITICAL_FAULT & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_MOST_CRITICAL_FAULT & 0x00FF);
            poll_state = PID_CURRENT_TIME;
            break;
          case PID_CURRENT_TIME:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CURRENT_TIME & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CURRENT_TIME & 0x00FF);
            poll_state = PID_TIME_SENT_BY_CAR;
            break;
          case PID_TIME_SENT_BY_CAR:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_TIME_SENT_BY_CAR & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_TIME_SENT_BY_CAR & 0x00FF);
            poll_state = PID_12V;
            break;
          case PID_12V:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_12V & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_12V & 0x00FF);
            poll_state = PID_12V_ABNORMAL;
            break;
          case PID_12V_ABNORMAL:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_12V_ABNORMAL & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_12V_ABNORMAL & 0x00FF);
            poll_state = PID_HVIL_IN_VOLTAGE;
            break;
          case PID_HVIL_IN_VOLTAGE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HVIL_IN_VOLTAGE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HVIL_IN_VOLTAGE & 0x00FF);
            poll_state = PID_HVIL_OUT_VOLTAGE;
            break;
          case PID_HVIL_OUT_VOLTAGE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HVIL_OUT_VOLTAGE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HVIL_OUT_VOLTAGE & 0x00FF);
            poll_state = PID_HVIL_STATE;
            break;
          case PID_HVIL_STATE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_HVIL_STATE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_HVIL_STATE & 0x00FF);
            poll_state = PID_BMS_STATE;
            break;
          case PID_BMS_STATE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_BMS_STATE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_BMS_STATE & 0x00FF);
            poll_state = PID_VEHICLE_SPEED;
            break;
          case PID_VEHICLE_SPEED:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_VEHICLE_SPEED & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_VEHICLE_SPEED & 0x00FF);
            poll_state = PID_TIME_SPENT_OVER_55C;
            break;
          case PID_TIME_SPENT_OVER_55C:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_TIME_SPENT_OVER_55C & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_TIME_SPENT_OVER_55C & 0x00FF);
            poll_state = PID_CONTACTOR_CLOSING_COUNTER;
            break;
          case PID_CONTACTOR_CLOSING_COUNTER:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_CONTACTOR_CLOSING_COUNTER & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_CONTACTOR_CLOSING_COUNTER & 0x00FF);
            poll_state = PID_DATE_OF_MANUFACTURE;
            break;
          case PID_DATE_OF_MANUFACTURE:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_DATE_OF_MANUFACTURE & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_DATE_OF_MANUFACTURE & 0x00FF);
            poll_state = PID_ALL_CELL_SOH;
            break;
          case PID_ALL_CELL_SOH:
            ECMP_POLL.data.u8[2] = (uint8_t)((PID_ALL_CELL_SOH & 0xFF00) >> 8);
            ECMP_POLL.data.u8[3] = (uint8_t)(PID_ALL_CELL_SOH & 0x00FF);
            poll_state = PID_WELD_CHECK;  // Loop back to beginning
            break;
          default:
            //We should not end up here. Reset poll_state to first poll
            poll_state = PID_WELD_CHECK;
            break;
        }
        transmit_can_frame(&ECMP_POLL);
      }
    }
  }

  // Send 10ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    counter_10ms = (counter_10ms + 1) % 16;

    if (datalayer.battery.status.bms_status == FAULT) {
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
    if (simulateEntireCar) {
      transmit_can_frame(&ECMP_111);
      transmit_can_frame(&ECMP_110);
      transmit_can_frame(&ECMP_114);
    }
  }

  // Send 20ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    if (datalayer.battery.status.bms_status == FAULT) {
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

    if (datalayer.battery.status.bms_status == FAULT) {
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

    if (datalayer.battery.status.bms_status == FAULT) {
      //Make vehicle appear as in idle HV state. Useful for clearing DTCs
      ECMP_31E.data.u8[0] = 0x48;
      ECMP_345.data = {0x45, 0x57, 0x00, 0x04, 0x00, 0x00, 0x06, 0x31};
      ECMP_351.data = {0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0E};
      ECMP_372.data = {0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      ECMP_383.data.u8[0] = 0x00;
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
      ECMP_31E.data.u8[0] = 0x50;
      ECMP_345.data = {0x45, 0x52, 0x00, 0x04, 0xDD, 0x00, 0x02, 0x30};
      ECMP_351.data = {0x00, 0x00, 0x00, 0x00, 0x0E, 0xA0, 0x00, 0xE0};
      ECMP_372.data = {0x9A, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      ECMP_383.data.u8[0] = 0x40;
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

    ECMP_31E.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_31E);
    ECMP_3A2.data.u8[6] = data_3A2_CRC[counter_100ms];
    ECMP_3A3.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_3A3);
    ECMP_010.data.u8[0] = data_010_CRC[counter_010];
    ECMP_345.data.u8[3] = (uint8_t)((data_345_content[counter_100ms] & 0XF0) | 0x4);
    ECMP_345.data.u8[7] = (uint8_t)(0x3 << 4 | (data_345_content[counter_100ms] & 0X0F));
    ECMP_351.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_351);
    ECMP_31D.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_31D);
    ECMP_3D0.data.u8[7] = counter_100ms << 4 | checksum_calc(counter_100ms, ECMP_3D0);

    transmit_can_frame(&ECMP_382);  //PSA Specific VCU (BSIInfo_382)
    transmit_can_frame(&ECMP_345);  //DC1_345
    transmit_can_frame(&ECMP_3A2);  //OBC2_3A2
    transmit_can_frame(&ECMP_3A3);  //OBC1_3A3
    transmit_can_frame(&ECMP_010);  //VCU_BCM_Crash
    if (simulateEntireCar) {
      transmit_can_frame(&ECMP_31E);
      transmit_can_frame(&ECMP_383);
      transmit_can_frame(&ECMP_0A6);  //Not in all logs
      transmit_can_frame(&ECMP_37F);  //Seems to be temperatures of some sort
      transmit_can_frame(&ECMP_372);
      transmit_can_frame(&ECMP_351);
      transmit_can_frame(&ECMP_31D);
    }
  }
  // Send 500ms periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;
    if (simulateEntireCar) {
      transmit_can_frame(&ECMP_0AE);
    }
  }
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    if (datalayer.battery.status.bms_status == FAULT) {
      //Make vehicle appear as in idle HV state. Useful for clearing DTCs
      ECMP_486.data.u8[0] = 0x80;
      ECMP_794.data.u8[0] = 0xB8;  //Not sure if needed, could be static?
    } else {
      //Normal operation for contactor closing
      ECMP_486.data.u8[0] = 0x00;
      ECMP_794.data.u8[0] = 0x38;  //Not sure if needed, could be static?
    }

    //552 seems to be tracking time in byte 0-3 , distance in km in byte 4-6, temporal reset counter in byte 7
    ticks_552 = (ticks_552 + 10);
    ECMP_552.data.u8[0] = ((ticks_552 & 0xFF000000) >> 24);
    ECMP_552.data.u8[1] = ((ticks_552 & 0x00FF0000) >> 16);
    ECMP_552.data.u8[2] = ((ticks_552 & 0x0000FF00) >> 8);
    ECMP_552.data.u8[3] = (ticks_552 & 0x000000FF);

    transmit_can_frame(&ECMP_439);  //OBC4
    transmit_can_frame(&ECMP_552);  //VCU_552 timetracking
    if (simulateEntireCar) {
      transmit_can_frame(&ECMP_486);  //Not in all logs
      transmit_can_frame(&ECMP_041);  //Not in all logs
      transmit_can_frame(&ECMP_786);  //Not in all logs
      transmit_can_frame(&ECMP_591);  //Not in all logs
      transmit_can_frame(&ECMP_794);  //Not in all logs
    }
  }
  // Send 5s periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
    if (simulateEntireCar) {
      transmit_can_frame(&ECMP_55F);
    }
  }
}

void EcmpBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
