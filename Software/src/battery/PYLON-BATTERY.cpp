#include "../include.h"
#ifdef PYLON_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "PYLON-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

//Actual content messages
CAN_frame PYLON_3010 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x3010,
                        .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_8200 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x8200,
                        .data = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_8210 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x8210,
                        .data = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_4200 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4200,
                        .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static int16_t celltemperature_max_dC = 0;
static int16_t celltemperature_min_dC = 0;
static int16_t current_dA = 0;
static uint16_t voltage_dV = 0;
static uint16_t cellvoltage_max_mV = 3700;
static uint16_t cellvoltage_min_mV = 3700;
static uint16_t charge_cutoff_voltage = 0;
static uint16_t discharge_cutoff_voltage = 0;
static int16_t max_charge_current = 0;
static int16_t max_discharge_current = 0;
static uint8_t ensemble_info_ack = 0;
static uint8_t battery_module_quantity = 0;
static uint8_t battery_modules_in_series = 0;
static uint8_t cell_quantity_in_module = 0;
static uint8_t voltage_level = 0;
static uint8_t ah_number = 0;
static uint8_t SOC = 0;
static uint8_t SOH = 0;
static uint8_t charge_forbidden = 0;
static uint8_t discharge_forbidden = 0;

void update_values_battery() {

  datalayer.battery.status.real_soc = (SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer.battery.status.soh_pptt = (SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery.status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = current_dA;  //value is *10 (150 = 15.0) , invert the sign

  datalayer.battery.status.max_charge_power_W = (max_charge_current * (voltage_dV / 10));

  datalayer.battery.status.max_discharge_power_W = (-max_discharge_current * (voltage_dV / 10));

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = cellvoltage_max_mV;
  datalayer.battery.status.cell_voltages_mV[0] = cellvoltage_max_mV;

  datalayer.battery.status.cell_min_voltage_mV = cellvoltage_min_mV;
  datalayer.battery.status.cell_voltages_mV[1] = cellvoltage_min_mV;

  datalayer.battery.status.temperature_min_dC = celltemperature_min_dC;

  datalayer.battery.status.temperature_max_dC = celltemperature_max_dC;

  datalayer.battery.info.max_design_voltage_dV = charge_cutoff_voltage;

  datalayer.battery.info.min_design_voltage_dV = discharge_cutoff_voltage;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {

    previousMillis1000 = currentMillis;

    transmit_can_frame(&PYLON_3010, can_config.battery);  // Heartbeat
    transmit_can_frame(&PYLON_4200, can_config.battery);  // Ensemble OR System equipment info, depends on frame0
    transmit_can_frame(&PYLON_8200, can_config.battery);  // Control device quit sleep status
    transmit_can_frame(&PYLON_8210, can_config.battery);  // Charge command

#ifdef DOUBLE_BATTERY
    transmit_can_frame(&PYLON_3010, can_config.battery_double);  // Heartbeat
    transmit_can_frame(&PYLON_4200, can_config.battery_double);  // Ensemble OR System equipment info, depends on frame0
    transmit_can_frame(&PYLON_8200, can_config.battery_double);  // Control device quit sleep status
    transmit_can_frame(&PYLON_8210, can_config.battery_double);  // Charge command
#endif                                                           //DOUBLE_BATTERY

    if (ensemble_info_ack) {
      PYLON_4200.data.u8[0] = 0x00;  //Request system equipment info
    }
  }
}

#ifdef DOUBLE_BATTERY

static int16_t battery2_celltemperature_max_dC = 0;
static int16_t battery2_celltemperature_min_dC = 0;
static int16_t battery2_current_dA = 0;
static uint16_t battery2_voltage_dV = 0;
static uint16_t battery2_cellvoltage_max_mV = 3700;
static uint16_t battery2_cellvoltage_min_mV = 3700;
static uint16_t battery2_charge_cutoff_voltage = 0;
static uint16_t battery2_discharge_cutoff_voltage = 0;
static int16_t battery2_max_charge_current = 0;
static int16_t battery2_max_discharge_current = 0;
static uint8_t battery2_ensemble_info_ack = 0;
static uint8_t battery2_module_quantity = 0;
static uint8_t battery2_modules_in_series = 0;
static uint8_t battery2_cell_quantity_in_module = 0;
static uint8_t battery2_voltage_level = 0;
static uint8_t battery2_ah_number = 0;
static uint8_t battery2_SOC = 0;
static uint8_t battery2_SOH = 0;
static uint8_t battery2_charge_forbidden = 0;
static uint8_t battery2_discharge_forbidden = 0;

void update_values_battery2() {

  datalayer.battery2.status.real_soc = (battery2_SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer.battery2.status.soh_pptt = (battery2_SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery2.status.voltage_dV = battery2_voltage_dV;  //value is *10 (3700 = 370.0)

  datalayer.battery2.status.current_dA = battery2_current_dA;  //value is *10 (150 = 15.0) , invert the sign

  datalayer.battery2.status.max_charge_power_W = (battery2_max_charge_current * (battery2_voltage_dV / 10));

  datalayer.battery2.status.max_discharge_power_W = (-battery2_max_discharge_current * (battery2_voltage_dV / 10));

  datalayer.battery2.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery2.status.real_soc) / 10000) * datalayer.battery2.info.total_capacity_Wh);

  datalayer.battery2.status.cell_max_voltage_mV = battery2_cellvoltage_max_mV;
  datalayer.battery2.status.cell_voltages_mV[0] = battery2_cellvoltage_max_mV;

  datalayer.battery2.status.cell_min_voltage_mV = battery2_cellvoltage_min_mV;
  datalayer.battery2.status.cell_voltages_mV[1] = battery2_cellvoltage_min_mV;

  datalayer.battery2.status.temperature_min_dC = battery2_celltemperature_min_dC;

  datalayer.battery2.status.temperature_max_dC = battery2_celltemperature_max_dC;

  datalayer.battery2.info.max_design_voltage_dV = battery2_charge_cutoff_voltage;

  datalayer.battery2.info.min_design_voltage_dV = battery2_discharge_cutoff_voltage;

  datalayer.battery2.info.number_of_cells = datalayer.battery.info.number_of_cells;
}

void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x7310:
    case 0x7311:
      battery2_ensemble_info_ack = true;
      // This message contains software/hardware version info. No interest to us
      break;
    case 0x7320:
    case 0x7321:
      battery2_ensemble_info_ack = true;
      battery2_module_quantity = rx_frame.data.u8[0];
      battery2_modules_in_series = rx_frame.data.u8[2];
      battery2_cell_quantity_in_module = rx_frame.data.u8[3];
      battery2_voltage_level = rx_frame.data.u8[4];
      battery2_ah_number = rx_frame.data.u8[6];
      break;
    case 0x4210:
    case 0x4211:
      battery2_voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      battery2_current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 30000;
      battery2_SOC = rx_frame.data.u8[6];
      battery2_SOH = rx_frame.data.u8[7];
      break;
    case 0x4220:
    case 0x4221:
      battery2_charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      battery2_discharge_cutoff_voltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      battery2_max_charge_current = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * 0.1) - 3000;
      battery2_max_discharge_current = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) * 0.1) - 3000;
      break;
    case 0x4230:
    case 0x4231:
      battery2_cellvoltage_max_mV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      battery2_cellvoltage_min_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      break;
    case 0x4240:
    case 0x4241:
      battery2_celltemperature_max_dC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 1000;
      battery2_celltemperature_min_dC = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 1000;
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
      battery2_charge_forbidden = rx_frame.data.u8[0];
      battery2_discharge_forbidden = rx_frame.data.u8[1];
      break;
    case 0x4290:
    case 0x4291:
      break;
    default:
      break;
  }
}
#endif  //DOUBLE_BATTERY

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Pylon compatible battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 2;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;

#ifdef DOUBLE_BATTERY
  datalayer.battery2.info.number_of_cells = datalayer.battery.info.number_of_cells;
  datalayer.battery2.info.max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  datalayer.battery2.info.min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  datalayer.battery2.info.max_cell_voltage_mV = datalayer.battery.info.max_cell_voltage_mV;
  datalayer.battery2.info.min_cell_voltage_mV = datalayer.battery.info.min_cell_voltage_mV;
  datalayer.battery2.info.max_cell_voltage_deviation_mV = datalayer.battery.info.max_cell_voltage_deviation_mV;
#endif  //DOUBLE_BATTERY
}

#endif
