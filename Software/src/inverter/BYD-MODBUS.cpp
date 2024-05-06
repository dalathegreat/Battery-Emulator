#include "../include.h"
#ifdef BYD_MODBUS
#include "../datalayer/datalayer.h"
#include "BYD-MODBUS.h"

// For modbus register definitions, see https://gitlab.com/pelle8/inverter_resources/-/blob/main/byd_registers_modbus_rtu.md

void update_modbus_registers_inverter() {
  verify_temperature_modbus();
  handle_update_data_modbusp201_byd();
  handle_update_data_modbusp301_byd();
}

void handle_static_data_modbus_byd() {
  // Store the data into the array
  static uint16_t si_data[] = {21321, 1};
  static uint16_t byd_data[] = {16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static uint16_t battery_data[] = {16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752,
                                    20594, 25965, 26997, 27936, 18518, 0,     0,     0};
  static uint16_t volt_data[] = {13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12598, 0, 0, 0, 0, 0, 0};
  static uint16_t serial_data[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static uint16_t static_data[] = {1, 0};
  static uint16_t* data_array_pointers[] = {si_data, byd_data, battery_data, volt_data, serial_data, static_data};
  static uint16_t data_sizes[] = {sizeof(si_data),   sizeof(byd_data),    sizeof(battery_data),
                                  sizeof(volt_data), sizeof(serial_data), sizeof(static_data)};
  static uint16_t i = 100;
  for (uint8_t arr_idx = 0; arr_idx < sizeof(data_array_pointers) / sizeof(uint16_t*); arr_idx++) {
    uint16_t data_size = data_sizes[arr_idx];
    memcpy(&mbPV[i], data_array_pointers[arr_idx], data_size);
    i += data_size / sizeof(uint16_t);
  }
}

void handle_update_data_modbusp201_byd() {
  // Store the data into the array
  static uint16_t system_data[13];
  //system_data[0] = 0;  // Id: p201 Value: 0 Scaled: 0 Comment: Always 0
  //system_data[1] = 0;  // Id: p202 Value: 0 Scaled: 0 Comment: Always 0
  system_data[2] = std::min(static_cast<int>(datalayer.battery.info.total_capacity_Wh), 60000);
  // Id: p203 Value: 32000 Scaled: 32kWh Comment: Capacity rated, maximum value is 60000 (60kWh)
  system_data[3] = MAX_POWER;  // Id: p204 Value: 32000 Scaled: 32kWh Comment: Nominal capacity
  system_data[4] = MAX_POWER;  // Id: p205 Value: 14500 Scaled: 30,42kW Comment: Max Charge/Discharge Power=10.24kW
  system_data[5] = (datalayer.battery.info.max_design_voltage_dV);
  // Id: p206 Value: 3667 Scaled: 362,7VDC Comment: Max Voltage, if higher charging is not possible (goes into forced discharge)
  system_data[6] = (datalayer.battery.info.min_design_voltage_dV);
  // Id: p207 Value: 2776 Scaled: 277,6VDC Comment: Min Voltage, if lower Gen24 disables battery
  system_data[7] = 53248;  // Id: p208 Value: 53248 Scaled: 53248 Comment: Always 53248 for BYD
  system_data[8] = 10;     // Id: p209 Value: 10 Scaled: 10 Comment: Always 10
  system_data[9] = 53248;  // Id: p210 Value: 53248 Scaled: 53248 Comment: Always 53248 for BYD
  system_data[10] = 10;    // Id: p211 Value: 10 Scaled: 10 Comment: Always 10
  //system_data[11] = 0;   // Id: p212 Value: 0 Scaled: 0 Comment: Always 0
  //system_data[12] = 0;   // Id: p213 Value: 0 Scaled: 0 Comment: Always 0
  memcpy(&mbPV[200], system_data, sizeof(system_data));
}

void handle_update_data_modbusp301_byd() {
  // Store the data into the array
  static uint16_t battery_data[24];
  static uint8_t bms_char_dis_status = STANDBY;
  if (datalayer.battery.status.current_dA == 0) {
    bms_char_dis_status = STANDBY;
  } else if (datalayer.battery.status.current_dA < 0) {  //Negative value = Discharging
    bms_char_dis_status = DISCHARGING;
  } else {  //Positive value = Charging
    bms_char_dis_status = CHARGING;
  }

  if (datalayer.battery.status.bms_status == ACTIVE) {
    battery_data[8] = datalayer.battery.status.voltage_dV;
    // Id: p309 Value: 3161 Scaled: 316,1VDC Comment: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  } else {
    battery_data[8] = 0;
  }
  battery_data[0] = datalayer.battery.status.bms_status;
  // Id: p301 Value: 3 Scaled: 3 Comment: status(*): ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
  //battery_data[1] = 0;  // Id: p302 Value: 0 Scaled: 0 Comment: always 0
  battery_data[2] = 128 + bms_char_dis_status;              // Id: p303 Value: 130 Scaled: 130 Comment: mode(*): normal
  battery_data[3] = datalayer.battery.status.reported_soc;  // Id: p304 Value: 1700 Scaled: 17.00% Comment: SOC
  battery_data[4] = std::min(static_cast<int>(datalayer.battery.info.total_capacity_Wh), 60000);
  // Id: p305 Value: 32000 Scaled: 32kWh Comment: tot cap: , maximum value is 60000 (60kWh)
  battery_data[5] = std::min(static_cast<int>(datalayer.battery.status.remaining_capacity_Wh), 60000);
  // Id: p306 Value: 13260 Scaled: 13,26kWh Comment: remaining cap: 7.68kWh , maximum value is 60000 (60kWh)

  // Convert max discharge Amp value to max Watt
  static int user_configured_max_discharge_W =
      ((datalayer.battery.info.max_discharge_amp_dA * datalayer.battery.info.max_design_voltage_dV) / 100);
  // Use the smaller value, battery reported value OR user configured value
  static int max_discharge_W =
      std::min(static_cast<int>(datalayer.battery.status.max_discharge_power_W), user_configured_max_discharge_W);
  battery_data[6] = std::min(max_discharge_W, 30000);  //Finally, map and cap to 30000 if needed
  // Id: p307 Value: 25604 Scaled: 25,604kW Comment: max discharge power: 0W (0W > restricts to no discharge)

  // Convert max charge Amp value to max Watt
  static int user_configured_max_charge_W =
      ((datalayer.battery.info.max_charge_amp_dA * datalayer.battery.info.max_design_voltage_dV) / 100);
  // Use the smaller value, battery reported value OR user configured value
  static int max_charge_W =
      std::min(static_cast<int>(datalayer.battery.status.max_charge_power_W), user_configured_max_charge_W);
  battery_data[7] = std::min(max_charge_W, 30000);  //Finally, map and cap to 30000 if needed
  // Id: p308 Value: 25604 Scaled: 25,604kW Comment: max charge power: 4.3kW (during charge), both 307&308 can be set (>0) at the same time

  //battery_data[8] set previously in function
  battery_data[9] = 2000;  // Id: p310 Value: 64121 Scaled: 6412,1W Comment: Current Power to API
  battery_data[10] = datalayer.battery.status.voltage_dV;  // Id: p311 Value: 3161 Scaled: 316,1V Comment: Batt Voltage
  battery_data[11] = 2000;  // Id: p312 Value: 64121 Scaled: 6412,1W Comment: same as p310
  battery_data[12] = datalayer.battery.status.temperature_min_dC;  // Id: p313 Value: 75 Scaled: 7,5 Comment: temp min
  battery_data[13] = datalayer.battery.status.temperature_max_dC;  // Id: p314 Value: 95 Scaled: 9,5 Comment: temp max
  //battery_data[14] = 0;   // Id: p315 Value: 0 Scaled: 0 Comment: always 0
  //battery_data[15] = 0;   // Id: p316 Value: 0 Scaled: 0 Comment: always 0
  battery_data[16] = 16;     // Id: p317 Value: 0 Scaled: 0 Comment: counter charge hi
  battery_data[17] = 22741;  // Id: p318 Value: 0 Scaled: 0 Comment: counter charge lo
  //battery_data[18] = 0;   // Id: p319 Value: 0 Scaled: 0 Comment: always 0
  //battery_data[19] = 0;   // Id: p320 Value: 0 Scaled: 0 Comment: always 0
  battery_data[20] = 13;     // Id: p321 Value: 0 Scaled: 0 Comment: counter discharge hi
  battery_data[21] = 52064;  // Id: p322 Value: 0 Scaled: 0 Comment: counter discharge lo
  battery_data[22] = 230;    // Id: p323 Value: 0 Scaled: 0 Comment: device temperature (23 degrees)
  battery_data[23] = datalayer.battery.status.soh_pptt;  // Id: p324 Value: 9900 Scaled: 99% Comment: SOH
  memcpy(&mbPV[300], battery_data, sizeof(battery_data));
}

void verify_temperature_modbus() {
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    return;  // Skip the following section
  }
  // This section checks if the battery temperature is negative, and incase it falls between -9.0 and -20.0C degrees
  // The Fronius Gen24 (and other Fronius inverters also affected), will stop charge/discharge if the battery gets colder than -10°C.
  // This is due to the original battery pack (BYD HVM), is a lithium iron phosphate battery, that cannot be charged in cold weather.
  // When using EV packs with NCM/LMO/NCA chemsitry, this is not a problem, since these chemistries are OK for outdoor cold use.
  if (datalayer.battery.status.temperature_min_dC < 0) {
    if (datalayer.battery.status.temperature_min_dC < -90 &&
        datalayer.battery.status.temperature_min_dC > -200) {  // Between -9.0 and -20.0C degrees
      datalayer.battery.status.temperature_min_dC = -90;       //Cap value to -9.0C
    }
  }
  if (datalayer.battery.status.temperature_max_dC < 0) {  // Signed value on negative side
    if (datalayer.battery.status.temperature_max_dC < -90 &&
        datalayer.battery.status.temperature_max_dC > -200) {  // Between -9.0 and -20.0C degrees
      datalayer.battery.status.temperature_max_dC = -90;       //Cap value to -9.0C
    }
  }
}
#endif
