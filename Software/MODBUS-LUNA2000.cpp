#include "MODBUS-LUNA2000.h"

void update_modbus_registers_luna2000()
{
    //Updata for ModbusRTU Server for Luna2000
    handle_update_data_modbus32051();
    handle_update_data_modbus39500();
}

void handle_update_data_modbus32051() {
  // Store the data into the array
  static uint16_t system_data[9];
  system_data[0] = 1;
  system_data[1] = 534;                 //Goes between 534-498 depending on SOC
  system_data[2] = 110;                 //Goes between 110- -107 [NOTE, SIGNED VALUE]
  system_data[3] = 0;                   //Goes between 0 and -1  [NOTE, SIGNED VALUE]
  system_data[4] = 616;                 //Goes between 616- -520 [NOTE, SIGNED VALUE]
  system_data[5] = temperature_max;     //Temperature max?
  system_data[6] = temperature_min;     //Temperature min?
  system_data[7] = (SOC/100);           //SOC 0-100%, no decimals 
  system_data[8] = 98;                  //Always 98 in logs
  static uint16_t i = 2051;
  memcpy(&mbPV[i], system_data, sizeof(system_data));
}

void handle_update_data_modbus39500() {
  // Store the data into the array
  static uint16_t system_data[26];
  system_data[0] = 0;
  system_data[1] = capacity_Wh_startup; //Capacity? 5000 with 5kWh battery
  system_data[2] = 0;
  system_data[3] = capacity_Wh_startup; //Capacity? 5000 with 5kWh battery
  system_data[4] = 0;       
  system_data[5] = 2500;                //???
  system_data[6] = 0;
  system_data[7] = 2500;                //???
  system_data[8] = (SOC/100);           //SOC 0-100%, no decimals 
  system_data[9] = 1;                   //Running status, equiv to register 37762, 0 = Offline, 1 = Standby,2 = Running, 3 = Fault, 4 = sleep mode
  system_data[10] = battery_voltage;    //Battery bus voltage (766.5V = 7665)
  system_data[11] = 9;                  //TODO, GOES LOWER WITH LOW SOC
  system_data[12] = 0;                
  system_data[13] = 699;                //TODO, GOES LOWER WITH LOW SOC
  system_data[14] = 1;                  //Always 1 in logs
  system_data[15] = 18;                 //Always 18 in logs
  system_data[16] = 8066;               //TODO, GOES HIGHER WITH LOW SOC (max allowed charge W?)
  system_data[17] = 17;       
  system_data[18] = 44027;              //TODO, GOES LOWER WITH LOW SOC
  system_data[19] = 0;
  system_data[20] = 435;                //Always 435 in logs
  system_data[21] = 0;
  system_data[22] = 0;
  system_data[23] = 0;
  system_data[24] = (SOC/10);           //SOC 0-100.0%, 1x decimal 
  system_data[25] = 0;
  system_data[26] = 1;                  //Always 1 in logs
  static uint16_t i = 9500;
  memcpy(&mbPV[i], system_data, sizeof(system_data));
}