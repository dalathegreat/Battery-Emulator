#include "../include.h"
#ifdef CELLPOWER_BMS
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "CELLPOWER-BMS.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1s = 0;  // will store last time a 1s CAN Message was sent

//Actual content messages
// Optional add-on charger module. Might not be needed to send these towards the BMS to keep it happy.
CAN_frame CELLPOWER_18FF50E9 = {.FD = false,
                                .ext_ID = true,
                                .DLC = 5,
                                .ID = 0x18FF50E9,
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame CELLPOWER_18FF50E8 = {.FD = false,
                                .ext_ID = true,
                                .DLC = 5,
                                .ID = 0x18FF50E8,
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame CELLPOWER_18FF50E7 = {.FD = false,
                                .ext_ID = true,
                                .DLC = 5,
                                .ID = 0x18FF50E7,
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame CELLPOWER_18FF50E5 = {.FD = false,
                                .ext_ID = true,
                                .DLC = 5,
                                .ID = 0x18FF50E5,
                                .data = {0x00, 0x00, 0x00, 0x00, 0x00}};

static bool system_state_discharge = false;
static bool system_state_charge = false;
static bool system_state_cellbalancing = false;
static bool system_state_tricklecharge = false;
static bool system_state_idle = false;
static bool system_state_chargecompleted = false;
static bool system_state_maintenancecharge = false;
static bool IO_state_main_positive_relay = false;
static bool IO_state_main_negative_relay = false;
static bool IO_state_charge_enable = false;
static bool IO_state_precharge_relay = false;
static bool IO_state_discharge_enable = false;
static bool IO_state_IO_6 = false;
static bool IO_state_IO_7 = false;
static bool IO_state_IO_8 = false;
static bool error_Cell_overvoltage = false;
static bool error_Cell_undervoltage = false;
static bool error_Cell_end_of_life_voltage = false;
static bool error_Cell_voltage_misread = false;
static bool error_Cell_over_temperature = false;
static bool error_Cell_under_temperature = false;
static bool error_Cell_unmanaged = false;
static bool error_LMU_over_temperature = false;
static bool error_LMU_under_temperature = false;
static bool error_Temp_sensor_open_circuit = false;
static bool error_Temp_sensor_short_circuit = false;
static bool error_SUB_communication = false;
static bool error_LMU_communication = false;
static bool error_Over_current_IN = false;
static bool error_Over_current_OUT = false;
static bool error_Short_circuit = false;
static bool error_Leak_detected = false;
static bool error_Leak_detection_failed = false;
static bool error_Voltage_difference = false;
static bool error_BMCU_supply_over_voltage = false;
static bool error_BMCU_supply_under_voltage = false;
static bool error_Main_positive_contactor = false;
static bool error_Main_negative_contactor = false;
static bool error_Precharge_contactor = false;
static bool error_Midpack_contactor = false;
static bool error_Precharge_timeout = false;
static bool error_Emergency_connector_override = false;
static bool warning_High_cell_voltage = false;
static bool warning_Low_cell_voltage = false;
static bool warning_High_cell_temperature = false;
static bool warning_Low_cell_temperature = false;
static bool warning_High_LMU_temperature = false;
static bool warning_Low_LMU_temperature = false;
static bool warning_SUB_communication_interfered = false;
static bool warning_LMU_communication_interfered = false;
static bool warning_High_current_IN = false;
static bool warning_High_current_OUT = false;
static bool warning_Pack_resistance_difference = false;
static bool warning_High_pack_resistance = false;
static bool warning_Cell_resistance_difference = false;
static bool warning_High_cell_resistance = false;
static bool warning_High_BMCU_supply_voltage = false;
static bool warning_Low_BMCU_supply_voltage = false;
static bool warning_Low_SOC = false;
static bool warning_Balancing_required_OCV_model = false;
static bool warning_Charger_not_responding = false;
static uint16_t cell_voltage_max_mV = 3700;
static uint16_t cell_voltage_min_mV = 3700;
static int8_t pack_temperature_high_C = 0;
static int8_t pack_temperature_low_C = 0;
static uint16_t battery_pack_voltage_dV = 3700;
static int16_t battery_pack_current_dA = 0;
static uint8_t battery_SOH_percentage = 99;
static uint8_t battery_SOC_percentage = 50;
static uint16_t battery_remaining_dAh = 0;
static uint8_t cell_with_highest_voltage = 0;
static uint8_t cell_with_lowest_voltage = 0;
static uint16_t requested_charge_current_dA = 0;
static uint16_t average_charge_current_dA = 0;
static uint16_t actual_charge_current_dA = 0;
static bool requested_exceeding_average_current = 0;
static bool error_state = false;

void update_values_battery() {

  /* Update values from CAN */

  datalayer.battery.status.real_soc = battery_SOC_percentage * 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = battery_SOH_percentage * 100;

  datalayer.battery.status.voltage_dV = battery_pack_voltage_dV;

  datalayer.battery.status.current_dA = battery_pack_current_dA;

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.max_charge_power_W = ((requested_charge_current_dA * battery_pack_voltage_dV) / 100);

  datalayer.battery.status.max_discharge_power_W = 5000;  //TODO, is this available via CAN?

  datalayer.battery.status.temperature_min_dC = (int16_t)(pack_temperature_low_C * 10);

  datalayer.battery.status.temperature_max_dC = (int16_t)(pack_temperature_high_C * 10);

  datalayer.battery.status.cell_max_voltage_mV = cell_voltage_max_mV;

  datalayer.battery.status.cell_min_voltage_mV = cell_voltage_min_mV;

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
}

void receive_can_battery(CAN_frame rx_frame) {

  /*
  // All CAN messages recieved will be logged via serial
  Serial.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  Serial.print("  ");
  Serial.print(rx_frame.ID, HEX);
  Serial.print("  ");
  Serial.print(rx_frame.DLC);
  Serial.print("  ");
  for (int i = 0; i < rx_frame.DLC; ++i) {
    Serial.print(rx_frame.data.u8[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
  */
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

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 1s CAN Message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {

    previousMillis1s = currentMillis;

    /*
    transmit_can(&CELLPOWER_18FF50E9, can_config.battery);
    transmit_can(&CELLPOWER_18FF50E8, can_config.battery);
    transmit_can(&CELLPOWER_18FF50E7, can_config.battery);
    transmit_can(&CELLPOWER_18FF50E5, can_config.battery);
    */
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Cellpower BMS selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}

#endif  // CELLPOWER_BMS
