#include "../include.h"
#ifdef FOXESS_CAN
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "FOXESS-CAN.h"

/* Based on info from this excellent repo: https://github.com/FozzieUK/FoxESS-Canbus-Protocol */

#define STATUS_OPERATIONAL_PACKS 0b11111111 //0x1875 b2 contains status for operational packs (responding) in binary so 01111111 is pack 8 not operational, 11101101 is pack 5 & 2 not operational
#define NUMBER_OF_PACKS 8
#define BATTERY_TYPE 0x82 //0x82 is HV2600 V1, 0x83 is ECS4100 v1, 0x84 is HV2600 V2
#define FIRMWARE_VERSION 0x20 //for the PACK_ID (b7 =10,20,30,40,50,60,70,80) then FIRMWARE_VERSION 0x1F = 0001 1111, version is v1.15, and if FIRMWARE_VERSION was 0x20 = 0010 0000 then = v2.0
#define PACK_ID 0x10
#define MAX_AC_VOLTAGE 2567 //256.7VAC max
#define TOTAL_LIFETIME_WH_ACCUMULATED 0 //We dont have this value in the emulator

/* Do not change code below unless you are sure what you are doing */
static uint16_t max_charge_rate_amp = 0;
static uint16_t max_discharge_rate_amp = 0;
static int16_t temperature_average = 0;
static uint8_t STATE = BATTERY_ANNOUNCE;
static unsigned long LastFrameTime = 0;
static uint16_t capped_capacity_Wh;
static uint16_t capped_remaining_capacity_Wh;

//CAN message translations from this amazing repository: https://github.com/rand12345/FOXESS_can_bus

CAN_frame FOXESS_1872 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1872,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_Limits
CAN_frame FOXESS_1873 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1873,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_PackData
CAN_frame FOXESS_1874 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1874,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_CellData
CAN_frame FOXESS_1875 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1875,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_Status
CAN_frame FOXESS_1876 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1876,
                        .data = {0x0, 0x0, 0xE2, 0x0C, 0x0, 0x0, 0xD7, 0x0C}};  //BMS_PackTemps
CAN_frame FOXESS_1877 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1877,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame FOXESS_1878 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1878,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_PackStats
CAN_frame FOXESS_1879 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1879,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame FOXESS_1881 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1881,
                        .data = {0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // E.g.: 0 6 S B M S F A
CAN_frame FOXESS_1882 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1882,
                        .data = {0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // E.g.: 0 2 3 A B 0 5 2
CAN_frame FOXESS_1883 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1883,
                        .data = {0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // E.g.: 0 2 3 A B 0 5 2

CAN_frame FOXESS_0C05 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C05,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C06 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C06,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C07 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C07,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C08 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C08,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C09 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C09,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C0A = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C0A,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C0B = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C0B,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
CAN_frame FOXESS_0C0C = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C0C,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
// Cellvoltages
CAN_frame FOXESS_0C1D = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C1D,
                        .data = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}; //All cells init to 4112mV
CAN_frame FOXESS_0C21 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C21,
                        .data = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}; //All cells init to 4112mV
CAN_frame FOXESS_0C25 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C25,
                        .data = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}; //All cells init to 4112mV
CAN_frame FOXESS_0C29 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C29,
                        .data = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}; //All cells init to 4112mV
CAN_frame FOXESS_0C2D = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C2D,
                        .data = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}; //All cells init to 4112mV
CAN_frame FOXESS_0C31 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x0C31,
                        .data = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}}; //All cells init to 4112mV


void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  //Calculate the required values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  //datalayer.battery.status.max_charge_power_W (30000W max)
  if (datalayer.battery.status.reported_soc > 9999) {  // 99.99%
    // Additional safety incase SOC% is 100, then do not charge battery further
    max_charge_rate_amp = 0;
  } else {  // We can pass on the battery charge rate (in W) to the inverter (that takes A)
    if (datalayer.battery.status.max_charge_power_W >= 30000) {
      max_charge_rate_amp = 75;  // Incase battery can take over 30kW, cap value to 75A
    } else {                     // Calculate the W value into A
      if (datalayer.battery.status.voltage_dV > 10) {
        max_charge_rate_amp =
            datalayer.battery.status.max_charge_power_W / (datalayer.battery.status.voltage_dV * 0.1);  // P/U=I
      } else {  // We avoid dividing by 0 and crashing the board
        // If we have no voltage, something has gone wrong, do not allow charging
        max_charge_rate_amp = 0;
      }
    }
  }

  //datalayer.battery.status.max_discharge_power_W (30000W max)
  if (datalayer.battery.status.reported_soc < 100) {  // 1.00%
    // Additional safety in case SOC% is below 1, then do not discharge battery further
    max_discharge_rate_amp = 0;
  } else {  // We can pass on the battery discharge rate to the inverter
    if (datalayer.battery.status.max_discharge_power_W >= 30000) {
      max_discharge_rate_amp = 75;  // Incase battery can be charged with over 30kW, cap value to 75A
    } else {                        // Calculate the W value into A
      if (datalayer.battery.status.voltage_dV > 10) {
        max_discharge_rate_amp =
            datalayer.battery.status.max_discharge_power_W / (datalayer.battery.status.voltage_dV * 0.1);  // P/U=I
      } else {  // We avoid dividing by 0 and crashing the board
        // If we have no voltage, something has gone wrong, do not allow discharging
        max_discharge_rate_amp = 0;
      }
    }
  }

  // Batteries might be larger than uint16_t value can take
  if (datalayer.battery.info.total_capacity_Wh > 65000) {
    capped_capacity_Wh = 65000;
  } else {
    capped_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
  }
  // Batteries might be larger than uint16_t value can take
  if (datalayer.battery.status.remaining_capacity_Wh > 65000) {
    capped_remaining_capacity_Wh = 65000;
  } else {
    capped_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
  }

  //Put the values into the CAN messages
  //BMS_Limits
  FOXESS_1872.data.u8[0] = (uint8_t)datalayer.battery.info.max_design_voltage_dV;
  FOXESS_1872.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  FOXESS_1872.data.u8[2] = (uint8_t)datalayer.battery.info.min_design_voltage_dV;
  FOXESS_1872.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  FOXESS_1872.data.u8[4] = (uint8_t)(max_charge_rate_amp * 10);
  FOXESS_1872.data.u8[5] = ((max_charge_rate_amp * 10) >> 8);
  FOXESS_1872.data.u8[6] = (uint8_t)(max_discharge_rate_amp * 10);
  FOXESS_1872.data.u8[7] = ((max_discharge_rate_amp * 10) >> 8);

  //BMS_PackData
  FOXESS_1873.data.u8[0] = (uint8_t)datalayer.battery.status.voltage_dV;  // OK
  FOXESS_1873.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  FOXESS_1873.data.u8[2] = (int8_t)datalayer.battery.status.current_dA;  // OK, Signed (Active current in Amps x 10)
  FOXESS_1873.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  FOXESS_1873.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);  //SOC (0-100%)
  FOXESS_1873.data.u8[5] = 0x00;
  FOXESS_1873.data.u8[6] = (uint8_t)(capped_remaining_capacity_Wh / 10);
  FOXESS_1873.data.u8[7] = ((capped_remaining_capacity_Wh / 10) >> 8);

  //BMS_CellData
  FOXESS_1874.data.u8[0] = (int8_t)datalayer.battery.status.temperature_max_dC;
  FOXESS_1874.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  FOXESS_1874.data.u8[2] = (int8_t)datalayer.battery.status.temperature_min_dC;
  FOXESS_1874.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
  FOXESS_1874.data.u8[4] = (uint8_t)(4200);  //cut_mv_max (Should we send a limit, or the actual mV?)
  FOXESS_1874.data.u8[5] = (4200 >> 8);
  FOXESS_1874.data.u8[6] = (uint8_t)(3000); //cut_mV_min (Should we send a limit, or the actual mV?)
  FOXESS_1874.data.u8[7] = (3000 >> 8);

  //BMS_Status
  FOXESS_1875.data.u8[0] = (uint8_t)temperature_average;
  FOXESS_1875.data.u8[1] = (temperature_average >> 8);
  FOXESS_1875.data.u8[2] = (uint8_t)STATUS_OPERATIONAL_PACKS;
  FOXESS_1875.data.u8[3] = (uint8_t)NUMBER_OF_PACKS;
  FOXESS_1875.data.u8[4] = (uint8_t)0;                  // Contactor Status 0=off, 1=on.
  FOXESS_1875.data.u8[5] = (uint8_t)0; //Unused
  FOXESS_1875.data.u8[6] = (uint8_t)1; //Cycle count
  FOXESS_1875.data.u8[7] = (uint8_t)2; //Cycle count

  //BMS_PackTemps
  FOXESS_1876.data.u8[0] = (uint8_t)0; //0x1876 b0 bit 0 appears to be 1 when at maxsoc and BMS says charge is not allowed - when at 0 indicates charge is possible - addn'l note there is something more to it than this, it's not as straight forward - needs more testing to find what sets/unsets bit0 of byte0
  FOXESS_1876.data.u8[1] = (uint8_t)0; //Unused
  FOXESS_1876.data.u8[2] = (uint8_t)datalayer.battery.status.cell_max_voltage_mV;
  FOXESS_1876.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  FOXESS_1876.data.u8[4] = (uint8_t)0; //Unused
  FOXESS_1876.data.u8[5] = (uint8_t)0; //Unused
  FOXESS_1876.data.u8[6] = (uint8_t)datalayer.battery.status.cell_min_voltage_mV;
  FOXESS_1876.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //BMS_ErrorsBrand
  FOXESS_1877.data.u8[0] = (uint8_t)0; //0x1877 b0 appears to be an error code, 0x02 when pack is in error.
  FOXESS_1877.data.u8[1] = (uint8_t)0; //Unused
  FOXESS_1877.data.u8[2] = (uint8_t)0; //Unused
  FOXESS_1877.data.u8[3] = (uint8_t)0; //Unused
  FOXESS_1877.data.u8[4] = (uint8_t)BATTERY_TYPE;
  FOXESS_1877.data.u8[5] = (uint8_t)0; //Unused
  FOXESS_1877.data.u8[6] = (uint8_t)FIRMWARE_VERSION;
  FOXESS_1877.data.u8[7] = (uint8_t)PACK_ID;

  //BMS_PackStats
  FOXESS_1878.data.u8[0] = (uint8_t)(MAX_AC_VOLTAGE);
  FOXESS_1878.data.u8[1] = ((MAX_AC_VOLTAGE) >> 8);
  FOXESS_1878.data.u8[2] = (uint8_t)0; //Unused
  FOXESS_1878.data.u8[3] = (uint8_t)0; //Unused
  FOXESS_1878.data.u8[4] = (uint8_t)TOTAL_LIFETIME_WH_ACCUMULATED;
  FOXESS_1878.data.u8[5] = (TOTAL_LIFETIME_WH_ACCUMULATED >> 8);
  FOXESS_1878.data.u8[6] = (TOTAL_LIFETIME_WH_ACCUMULATED >> 16);
  FOXESS_1878.data.u8[7] = (TOTAL_LIFETIME_WH_ACCUMULATED >> 24);

  //Errorcodes and flags
  FOXESS_1879.data.u8[0] = (uint8_t)0; // Error codes go here, still unsure of bitmasking
  if(datalayer.battery.status.current_dA > 0) { 
    FOXESS_1879.data.u8[1] = 0x35; //Charging
  } // Mappings taken from https://github.com/FozzieUK/FoxESS-Canbus-Protocol
  else{ 
    FOXESS_1879.data.u8[1] = 0x2B; //Discharging
  }

  // Individual pack data (1-8)
  FOXESS_0C05.data.u8[0] = 0; //Pack_1_Current
  FOXESS_0C05.data.u8[1] = 0; //Pack_1_Current
  FOXESS_0C05.data.u8[2] = 0; //Pack_1_avg_hitemp
  FOXESS_0C05.data.u8[3] = 0; //Pack_1_avg_lotemp
  FOXESS_0C05.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);  //pack_1_SoC
  FOXESS_0C05.data.u8[5] = 0; //b5-7chg/dis?
  FOXESS_0C05.data.u8[6] = 0xD0; //pack_1_volts (53.456V)
  FOXESS_0C05.data.u8[7] = 0xD0; //pack_1_volts (53.456V)

  //TODO, map all messages for 0C05 -> 0C0C

}

void send_can_inverter() {
  // No periodic sending used on this protocol, we react only on incoming CAN messages!
}

void receive_can_inverter(CAN_frame rx_frame) {

  if (rx_frame.ID == 0x1871){
    if(rx_frame.data.u8[0] == 0x03){ //0x1871 [0x03, 0x06, 0x17, 0x05, 0x09, 0x09, 0x28, 0x22]
      //This message is sent by the inverter every '6' seconds (0.5s after the pack serial numbers) 
      //and contains a timestamp in bytes 2-7 i.e. <YY>,<MM>,<DD>,<HH>,<mm>,<ss>
      #ifdef DEBUG_VIA_USB
        Serial.println("Inverter sends current time and date");
      #endif
    }
    else if(rx_frame.data.u8[0] == 0x01){ 
      if(rx_frame.data.u8[4] == 0x00){
      // Inverter wants to know bms info (every 1s)
      #ifdef DEBUG_VIA_USB
        Serial.println("Inverter requests 1s BMS info, we reply");
      #endif
          transmit_can(&FOXESS_1872, can_config.inverter);
          transmit_can(&FOXESS_1873, can_config.inverter);
          transmit_can(&FOXESS_1874, can_config.inverter);
          transmit_can(&FOXESS_1875, can_config.inverter);
          transmit_can(&FOXESS_1876, can_config.inverter);
          transmit_can(&FOXESS_1877, can_config.inverter);
          transmit_can(&FOXESS_1878, can_config.inverter);
          transmit_can(&FOXESS_1879, can_config.inverter);
      }
      else if(rx_frame.data.u8[4] == 0x01){ // b4 0x01 , 0x1871 [0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
        //Inverter wants to know all individual cellvoltages (occurs 6 seconds after valid BMS reply)
      #ifdef DEBUG_VIA_USB
        Serial.println("Inverter requests individual battery pack status, we reply");
      #endif
          transmit_can(&FOXESS_0C05, can_config.inverter); //TODO, should we limit this incase NUMBER_OF_PACKS =/= 8?
          transmit_can(&FOXESS_0C06, can_config.inverter);
          transmit_can(&FOXESS_0C07, can_config.inverter);
          transmit_can(&FOXESS_0C08, can_config.inverter);
          transmit_can(&FOXESS_0C09, can_config.inverter);
          transmit_can(&FOXESS_0C0A, can_config.inverter);
          transmit_can(&FOXESS_0C0B, can_config.inverter);
          transmit_can(&FOXESS_0C0C, can_config.inverter);
      }
      else if(rx_frame.data.u8[4] == 0x04){ // b4 0x01 , 0x1871 [0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
        //Inverter wants to know all individual cellvoltages (occurs 6 seconds after valid BMS reply)
      #ifdef DEBUG_VIA_USB
        Serial.println("Inverter requests cellvoltages, we reply");
      #endif
        //TODO, add them all
        //Cellvoltages
        transmit_can(&FOXESS_0C1D, can_config.inverter);
        transmit_can(&FOXESS_0C21, can_config.inverter);
        transmit_can(&FOXESS_0C25, can_config.inverter);
        transmit_can(&FOXESS_0C29, can_config.inverter);
        transmit_can(&FOXESS_0C2D, can_config.inverter);
        transmit_can(&FOXESS_0C31, can_config.inverter);  //TODO, add them all

        //Celltemperatures
         //TODO, add them all
      }
    }
    else if(rx_frame.data.u8[0] == 0x02){ //0x1871 [0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
      // Ack message
      #ifdef DEBUG_VIA_USB
        Serial.println("Inverter acks, no reply needed");
      #endif
    }
    else if(rx_frame.data.u8[0] == 0x05){  //0x1871 [0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00]
      #ifdef DEBUG_VIA_USB
        Serial.println("Inverter wants to know serial numbers, we reply");
      #endif
      for (int i = 0; i < (NUMBER_OF_PACKS+1); i++) {
        FOXESS_1881.data.u8[0] = (uint8_t)i;
        FOXESS_1882.data.u8[0] = (uint8_t)i;
        FOXESS_1883.data.u8[0] = (uint8_t)i;
        transmit_can(&FOXESS_1881, can_config.inverter);
        transmit_can(&FOXESS_1882, can_config.inverter);
        transmit_can(&FOXESS_1883, can_config.inverter);
      }
    }
  }
}
#endif
