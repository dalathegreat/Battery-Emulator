#include "MODBUS-BYD.h"

void update_modbus_registers_byd() {
    //Updata for ModbusRTU Server for BYD
    handle_update_data_modbusp201_byd();
    handle_update_data_modbusp301_byd(); 
}

void handle_static_data_modbus_byd() {
  // Store the data into the array
  static uint16_t si_data[] = { 21321, 1 };
  static uint16_t byd_data[] = { 16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  static uint16_t battery_data[] = { 16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752, 20594, 25965, 26997, 27936, 18518, 0, 0, 0 };
  static uint16_t volt_data[] = { 13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12598, 0, 0, 0, 0, 0, 0 };
  static uint16_t serial_data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  static uint16_t static_data[] = { 1, 0 };
  static uint16_t* data_array_pointers[] = { si_data, byd_data, battery_data, volt_data, serial_data, static_data };
  static uint16_t data_sizes[] = { sizeof(si_data), sizeof(byd_data), sizeof(battery_data), sizeof(volt_data), sizeof(serial_data), sizeof(static_data) };
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
  system_data[0] = 0;                                                          // Id.: p201 Value.: 0 Scaled value.: 0 Comment.: Always 0
  system_data[1] = 0;                                                          // Id.: p202 Value.: 0 Scaled value.: 0 Comment.: Always 0
  system_data[2] = (capacity_Wh_startup);                                      // Id.: p203 Value.: 32000 Scaled value.: 32kWh Comment.: Capacity rated, maximum value is 60000 (60kWh)
  system_data[3] = (max_power);                                                // Id.: p204 Value.: 32000 Scaled value.: 32kWh Comment.: Nominal capacity
  system_data[4] = (max_power);                                                // Id.: p205 Value.: 14500 Scaled value.: 30,42kW  Comment.: Max Charge/Discharge Power=10.24kW (lowest value of 204 and 205 will be enforced by Gen24)
  system_data[5] = (max_volt_modbus_byd);                                      // Id.: p206 Value.: 3667 Scaled value.: 362,7VDC Comment.: Max Voltage, if higher charging is not possible (goes into forced discharge)
  system_data[6] = (min_volt_modbus_byd);                                      // Id.: p207 Value.: 2776 Scaled value.: 277,6VDC Comment.: Min Voltage, if lower Gen24 disables battery
  system_data[7] = 53248;                                                      // Id.: p208 Value.: 53248 Scaled value.: 53248 Comment.: Always 53248 for this BYD, Peak Charge power?
  system_data[8] = 10;                                                         // Id.: p209 Value.: 10 Scaled value.: 10 Comment.: Always 10
  system_data[9] = 53248;                                                      // Id.: p210 Value.: 53248 Scaled value.: 53248 Comment.: Always 53248 for this BYD, Peak Discharge power?
  system_data[10] = 10;                                                        // Id.: p211 Value.: 10 Scaled value.: 10 Comment.: Always 10
  system_data[11] = 0;                                                         // Id.: p212 Value.: 0 Scaled value.: 0 Comment.: Always 0
  system_data[12] = 0;                                                         // Id.: p213 Value.: 0 Scaled value.: 0 Comment.: Always 0
  static uint16_t i = 200;
  memcpy(&mbPV[i], system_data, sizeof(system_data));
}

void handle_update_data_modbusp301_byd() {
  // Store the data into the array
  static uint16_t battery_data[24];
  if (battery_current > 0) { //Positive value = Charging
    bms_char_dis_status = 2; //Charging
  } else if (battery_current < 0) { //Negative value = Discharging
    bms_char_dis_status = 1; //Discharging
  } else { //battery_current == 0
    bms_char_dis_status = 0; //idle
  }

  if (bms_status == ACTIVE) {
    battery_data[8] = battery_voltage;  // Id.: p309 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  } else {
    battery_data[8] = 0;
  }
  battery_data[0] = bms_status;                          // Id.: p301 Value.: 3 Scaled value.: 3 Comment.: status(*): ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
  battery_data[1] = 0;                                   // Id.: p302 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[2] = 128 + bms_char_dis_status;           // Id.: p303 Value.: 130 Scaled value.: 130 Comment.: mode(*): normal
  battery_data[3] = SOC;                                 // Id.: p304 Value.: 1700 Scaled value.: 50% Comment.: SOC: (50% would equal 5000)
  battery_data[4] = capacity_Wh;                         // Id.: p305 Value.: 32000 Scaled value.: 32kWh Comment.: tot cap:
  battery_data[5] = remaining_capacity_Wh;               // Id.: p306 Value.: 13260 Scaled value.: 13,26kWh Comment.: remaining cap: 7.68kWh
  battery_data[6] = max_target_discharge_power;          // Id.: p307 Value.: 25604 Scaled value.: 25,604kW Comment.: max/target discharge power: 0W (0W > restricts to no discharge)
  battery_data[7] = max_target_charge_power;             // Id.: p308 Value.: 25604 Scaled value.: 25,604kW Comment.: max/target charge power: 4.3kW (during charge), both 307&308 can be set (>0) at the same time
  //Battery_data[8] set previously in function           // Id.: p309 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  battery_data[9] = 2000;                                // Id.: p310 Value.: 64121 Scaled value.: 6412,1W Comment.: Current Power to API: if>32768... -(65535-61760)=3775W
  battery_data[10] = battery_voltage;                    // Id.: p311 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage inner: 173.2V (LEAF voltage is in whole volts, need to add a decimal)
  battery_data[11] = 2000;                               // Id.: p312 Value.: 64121 Scaled value.: 6412,1W Comment.: p310
  battery_data[12] = temperature_min;                    // Id.: p313 Value.: 75 Scaled value.: 7,5 Comment.: temp min: 7 degrees (if below 0....65535-t)
  battery_data[13] = temperature_max;                    // Id.: p314 Value.: 95 Scaled value.: 9,5 Comment.: temp max: 9 degrees (if below 0....65535-t)
  battery_data[14] = 0;                                  // Id.: p315 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[15] = 0;                                  // Id.: p316 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[16] = 16;                                 // Id.: p317 Value.: 0 Scaled value.: 0 Comment.: counter charge hi
  battery_data[17] = 22741;                              // Id.: p318 Value.: 0 Scaled value.: 0 Comment.: counter charge lo....65536*101+9912 = 6629048 Wh?
  battery_data[18] = 0;                                  // Id.: p319 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[19] = 0;                                  // Id.: p320 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[20] = 13;                                 // Id.: p321 Value.: 0 Scaled value.: 0 Comment.: counter discharge hi
  battery_data[21] = 52064;                              // Id.: p322 Value.: 0 Scaled value.: 0 Comment.: counter discharge lo....65536*92+7448 = 6036760 Wh?
  battery_data[22] = 230;                                // Id.: p323 Value.: 0 Scaled value.: 0 Comment.: device temperature (23 degrees)
  battery_data[23] = StateOfHealth;                      // Id.: p324 Value.: 9900 Scaled value.: 99% Comment.: SOH
  static uint16_t i = 300;
  memcpy(&mbPV[i], battery_data, sizeof(battery_data));
}
