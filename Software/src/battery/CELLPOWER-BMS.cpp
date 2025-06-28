#include "CELLPOWER-BMS.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "../include.h"

void CellPowerBms::update_values() {

  /* Update values from CAN */

  datalayer.battery.status.real_soc = battery_SOC_percentage * 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = battery_SOH_percentage * 100;

  datalayer.battery.status.voltage_dV = battery_pack_voltage_dV;

  datalayer.battery.status.current_dA = battery_pack_current_dA;

  datalayer.battery.status.max_charge_power_W = 5000;  //TODO, is this available via CAN?

  datalayer.battery.status.max_discharge_power_W = 5000;  //TODO, is this available via CAN?

  datalayer.battery.status.temperature_min_dC = (int16_t)(pack_temperature_low_C * 10);

  datalayer.battery.status.temperature_max_dC = (int16_t)(pack_temperature_high_C * 10);

  datalayer.battery.status.cell_max_voltage_mV = cell_voltage_max_mV;

  datalayer.battery.status.cell_min_voltage_mV = cell_voltage_min_mV;

  /* Update webserver datalayer */
  datalayer_extended.cellpower.system_state_discharge = system_state_discharge;
  datalayer_extended.cellpower.system_state_charge = system_state_charge;
  datalayer_extended.cellpower.system_state_cellbalancing = system_state_cellbalancing;
  datalayer_extended.cellpower.system_state_tricklecharge = system_state_tricklecharge;
  datalayer_extended.cellpower.system_state_idle = system_state_idle;
  datalayer_extended.cellpower.system_state_chargecompleted = system_state_chargecompleted;
  datalayer_extended.cellpower.system_state_maintenancecharge = system_state_maintenancecharge;
  datalayer_extended.cellpower.IO_state_main_positive_relay = IO_state_main_positive_relay;
  datalayer_extended.cellpower.IO_state_main_negative_relay = IO_state_main_negative_relay;
  datalayer_extended.cellpower.IO_state_charge_enable = IO_state_charge_enable;
  datalayer_extended.cellpower.IO_state_precharge_relay = IO_state_precharge_relay;
  datalayer_extended.cellpower.IO_state_discharge_enable = IO_state_discharge_enable;
  datalayer_extended.cellpower.IO_state_IO_6 = IO_state_IO_6;
  datalayer_extended.cellpower.IO_state_IO_7 = IO_state_IO_7;
  datalayer_extended.cellpower.IO_state_IO_8 = IO_state_IO_8;
  datalayer_extended.cellpower.error_Cell_overvoltage = error_Cell_overvoltage;
  datalayer_extended.cellpower.error_Cell_undervoltage = error_Cell_undervoltage;
  datalayer_extended.cellpower.error_Cell_end_of_life_voltage = error_Cell_end_of_life_voltage;
  datalayer_extended.cellpower.error_Cell_voltage_misread = error_Cell_voltage_misread;
  datalayer_extended.cellpower.error_Cell_over_temperature = error_Cell_over_temperature;
  datalayer_extended.cellpower.error_Cell_under_temperature = error_Cell_under_temperature;
  datalayer_extended.cellpower.error_Cell_unmanaged = error_Cell_unmanaged;
  datalayer_extended.cellpower.error_LMU_over_temperature = error_LMU_over_temperature;
  datalayer_extended.cellpower.error_LMU_under_temperature = error_LMU_under_temperature;
  datalayer_extended.cellpower.error_Temp_sensor_open_circuit = error_Temp_sensor_open_circuit;
  datalayer_extended.cellpower.error_Temp_sensor_short_circuit = error_Temp_sensor_short_circuit;
  datalayer_extended.cellpower.error_SUB_communication = error_SUB_communication;
  datalayer_extended.cellpower.error_LMU_communication = error_LMU_communication;
  datalayer_extended.cellpower.error_Over_current_IN = error_Over_current_IN;
  datalayer_extended.cellpower.error_Over_current_OUT = error_Over_current_OUT;
  datalayer_extended.cellpower.error_Short_circuit = error_Short_circuit;
  datalayer_extended.cellpower.error_Leak_detected = error_Leak_detected;
  datalayer_extended.cellpower.error_Leak_detection_failed = error_Leak_detection_failed;
  datalayer_extended.cellpower.error_Voltage_difference = error_Voltage_difference;
  datalayer_extended.cellpower.error_BMCU_supply_over_voltage = error_BMCU_supply_over_voltage;
  datalayer_extended.cellpower.error_BMCU_supply_under_voltage = error_BMCU_supply_under_voltage;
  datalayer_extended.cellpower.error_Main_positive_contactor = error_Main_positive_contactor;
  datalayer_extended.cellpower.error_Main_negative_contactor = error_Main_negative_contactor;
  datalayer_extended.cellpower.error_Precharge_contactor = error_Precharge_contactor;
  datalayer_extended.cellpower.error_Midpack_contactor = error_Midpack_contactor;
  datalayer_extended.cellpower.error_Precharge_timeout = error_Precharge_timeout;
  datalayer_extended.cellpower.error_Emergency_connector_override = error_Emergency_connector_override;
  datalayer_extended.cellpower.warning_High_cell_voltage = warning_High_cell_voltage;
  datalayer_extended.cellpower.warning_Low_cell_voltage = warning_Low_cell_voltage;
  datalayer_extended.cellpower.warning_High_cell_temperature = warning_High_cell_temperature;
  datalayer_extended.cellpower.warning_Low_cell_temperature = warning_Low_cell_temperature;
  datalayer_extended.cellpower.warning_High_LMU_temperature = warning_High_LMU_temperature;
  datalayer_extended.cellpower.warning_Low_LMU_temperature = warning_Low_LMU_temperature;
  datalayer_extended.cellpower.warning_SUB_communication_interfered = warning_SUB_communication_interfered;
  datalayer_extended.cellpower.warning_LMU_communication_interfered = warning_LMU_communication_interfered;
  datalayer_extended.cellpower.warning_High_current_IN = warning_High_current_IN;
  datalayer_extended.cellpower.warning_High_current_OUT = warning_High_current_OUT;
  datalayer_extended.cellpower.warning_Pack_resistance_difference = warning_Pack_resistance_difference;
  datalayer_extended.cellpower.warning_High_pack_resistance = warning_High_pack_resistance;
  datalayer_extended.cellpower.warning_Cell_resistance_difference = warning_Cell_resistance_difference;
  datalayer_extended.cellpower.warning_High_cell_resistance = warning_High_cell_resistance;
  datalayer_extended.cellpower.warning_High_BMCU_supply_voltage = warning_High_BMCU_supply_voltage;
  datalayer_extended.cellpower.warning_Low_BMCU_supply_voltage = warning_Low_BMCU_supply_voltage;
  datalayer_extended.cellpower.warning_Low_SOC = warning_Low_SOC;
  datalayer_extended.cellpower.warning_Balancing_required_OCV_model = warning_Balancing_required_OCV_model;
  datalayer_extended.cellpower.warning_Charger_not_responding = warning_Charger_not_responding;

  /* Peform safety checks */
  if (system_state_chargecompleted) {
    //TODO, shall we set max_charge_power_W to 0 incase this is true?
  }
  if (IO_state_charge_enable) {
    //TODO, shall we react on this?
  }
  if (IO_state_discharge_enable) {
    //TODO, shall we react on this?
  }
  if (error_state) {
    //TODO, shall we react on this?
  }
}

void CellPowerBms::handle_incoming_can_frame(CAN_frame rx_frame) {

  switch (rx_frame.ID) {
    case 0x1A4:  //PDO1_TX - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cell_voltage_max_mV = (uint16_t)((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      cell_voltage_min_mV = (uint16_t)((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      pack_temperature_high_C = (int8_t)rx_frame.data.u8[4];
      pack_temperature_low_C = (int8_t)rx_frame.data.u8[5];
      system_state_discharge = (rx_frame.data.u8[6] & 0x01);
      system_state_charge = ((rx_frame.data.u8[6] & 0x02) >> 1);
      system_state_cellbalancing = ((rx_frame.data.u8[6] & 0x04) >> 2);
      system_state_tricklecharge = ((rx_frame.data.u8[6] & 0x08) >> 3);
      system_state_idle = ((rx_frame.data.u8[6] & 0x10) >> 4);
      system_state_chargecompleted = ((rx_frame.data.u8[6] & 0x20) >> 5);
      system_state_maintenancecharge = ((rx_frame.data.u8[6] & 0x40) >> 6);
      IO_state_main_positive_relay = (rx_frame.data.u8[7] & 0x01);
      IO_state_main_negative_relay = ((rx_frame.data.u8[7] & 0x02) >> 1);
      IO_state_charge_enable = ((rx_frame.data.u8[7] & 0x04) >> 2);
      IO_state_precharge_relay = ((rx_frame.data.u8[7] & 0x08) >> 3);
      IO_state_discharge_enable = ((rx_frame.data.u8[7] & 0x10) >> 4);
      IO_state_IO_6 = ((rx_frame.data.u8[7] & 0x20) >> 5);
      IO_state_IO_7 = ((rx_frame.data.u8[7] & 0x40) >> 6);
      IO_state_IO_8 = ((rx_frame.data.u8[7] & 0x80) >> 7);
      break;
    case 0x2A4:  //PDO2_TX - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_pack_voltage_dV = (uint16_t)((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      battery_pack_current_dA = (int16_t)((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      battery_SOH_percentage = (uint8_t)rx_frame.data.u8[4];
      battery_SOC_percentage = (uint8_t)rx_frame.data.u8[5];
      battery_remaining_dAh = (uint16_t)((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);
      break;
    case 0x3A4:  //PDO3_TX - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cell_with_highest_voltage = (uint8_t)rx_frame.data.u8[0];
      cell_with_lowest_voltage = (uint8_t)rx_frame.data.u8[1];
      break;
    case 0x4A4:  //PDO4_TX - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      error_Cell_overvoltage = (rx_frame.data.u8[0] & 0x01);
      error_Cell_undervoltage = ((rx_frame.data.u8[0] & 0x02) >> 1);
      error_Cell_end_of_life_voltage = ((rx_frame.data.u8[0] & 0x04) >> 2);
      error_Cell_voltage_misread = ((rx_frame.data.u8[0] & 0x08) >> 3);
      error_Cell_over_temperature = ((rx_frame.data.u8[0] & 0x10) >> 4);
      error_Cell_under_temperature = ((rx_frame.data.u8[0] & 0x20) >> 5);
      error_Cell_unmanaged = ((rx_frame.data.u8[0] & 0x40) >> 6);
      error_LMU_over_temperature = ((rx_frame.data.u8[0] & 0x80) >> 7);
      error_LMU_under_temperature = (rx_frame.data.u8[1] & 0x01);
      error_Temp_sensor_open_circuit = ((rx_frame.data.u8[1] & 0x02) >> 1);
      error_Temp_sensor_short_circuit = ((rx_frame.data.u8[1] & 0x04) >> 2);
      error_SUB_communication = ((rx_frame.data.u8[1] & 0x08) >> 3);
      error_LMU_communication = ((rx_frame.data.u8[1] & 0x10) >> 4);
      error_Over_current_IN = ((rx_frame.data.u8[1] & 0x20) >> 5);
      error_Over_current_OUT = ((rx_frame.data.u8[1] & 0x40) >> 6);
      error_Short_circuit = ((rx_frame.data.u8[1] & 0x80) >> 7);
      error_Leak_detected = (rx_frame.data.u8[2] & 0x01);
      error_Leak_detection_failed = ((rx_frame.data.u8[2] & 0x02) >> 1);
      error_Voltage_difference = ((rx_frame.data.u8[2] & 0x04) >> 2);
      error_BMCU_supply_over_voltage = ((rx_frame.data.u8[2] & 0x08) >> 3);
      error_BMCU_supply_under_voltage = ((rx_frame.data.u8[2] & 0x10) >> 4);
      error_Main_positive_contactor = ((rx_frame.data.u8[2] & 0x20) >> 5);
      error_Main_negative_contactor = ((rx_frame.data.u8[2] & 0x40) >> 6);
      error_Precharge_contactor = ((rx_frame.data.u8[2] & 0x80) >> 7);
      error_Midpack_contactor = (rx_frame.data.u8[3] & 0x01);
      error_Precharge_timeout = ((rx_frame.data.u8[3] & 0x02) >> 1);
      error_Emergency_connector_override = ((rx_frame.data.u8[3] & 0x04) >> 2);
      warning_High_cell_voltage = (rx_frame.data.u8[4] & 0x01);
      warning_Low_cell_voltage = ((rx_frame.data.u8[4] & 0x02) >> 1);
      warning_High_cell_temperature = ((rx_frame.data.u8[4] & 0x04) >> 2);
      warning_Low_cell_temperature = ((rx_frame.data.u8[4] & 0x08) >> 3);
      warning_High_LMU_temperature = ((rx_frame.data.u8[4] & 0x10) >> 4);
      warning_Low_LMU_temperature = ((rx_frame.data.u8[4] & 0x20) >> 5);
      warning_SUB_communication_interfered = ((rx_frame.data.u8[4] & 0x40) >> 6);
      warning_LMU_communication_interfered = ((rx_frame.data.u8[4] & 0x80) >> 7);
      warning_High_current_IN = (rx_frame.data.u8[5] & 0x01);
      warning_High_current_OUT = ((rx_frame.data.u8[5] & 0x02) >> 1);
      warning_Pack_resistance_difference = ((rx_frame.data.u8[5] & 0x04) >> 2);
      warning_High_pack_resistance = ((rx_frame.data.u8[5] & 0x08) >> 3);
      warning_Cell_resistance_difference = ((rx_frame.data.u8[5] & 0x10) >> 4);
      warning_High_cell_resistance = ((rx_frame.data.u8[5] & 0x20) >> 5);
      warning_High_BMCU_supply_voltage = ((rx_frame.data.u8[5] & 0x40) >> 6);
      warning_Low_BMCU_supply_voltage = ((rx_frame.data.u8[5] & 0x80) >> 7);
      warning_Low_SOC = (rx_frame.data.u8[6] & 0x01);
      warning_Balancing_required_OCV_model = ((rx_frame.data.u8[6] & 0x02) >> 1);
      warning_Charger_not_responding = ((rx_frame.data.u8[6] & 0x04) >> 2);
      break;
    case 0x7A4:  //PDO7_TX - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      requested_charge_current_dA = (uint16_t)((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      average_charge_current_dA = (uint16_t)((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      actual_charge_current_dA = (uint16_t)((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);
      requested_exceeding_average_current = (rx_frame.data.u8[6] & 0x01);
      break;
    case 0x7A5:  //PDO7.1_TX - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      error_state = (rx_frame.data.u8[0] & 0x01);
      break;
    default:
      break;
  }
}

void CellPowerBms::transmit_can(unsigned long currentMillis) {

  // Send 1s CAN Message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    /*
    transmit_can_frame(&CELLPOWER_18FF50E9, can_config.battery);
    transmit_can_frame(&CELLPOWER_18FF50E8, can_config.battery);
    transmit_can_frame(&CELLPOWER_18FF50E7, can_config.battery);
    transmit_can_frame(&CELLPOWER_18FF50E5, can_config.battery);
    */
  }
}

void CellPowerBms::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
