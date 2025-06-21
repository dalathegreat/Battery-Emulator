#include "FOXESS-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/* Based on info from this excellent repo: https://github.com/FozzieUK/FoxESS-Canbus-Protocol */
/* The FoxESS protocol emulates the stackable (1-8) 48V towers found in the HV2600 / ECS4100 batteries
We emulate a full tower setup by default (8*50V=400V) to be more suitable for an EV pack. There are settings
below that you can customize, incase you use a lower voltage battery with this protocol */

#define STATUS_OPERATIONAL_PACKS \
  0b11111111  //0x1875 b2 contains status for operational packs (responding) in binary so 01111111 is pack 8 not operational, 11101101 is pack 5 & 2 not operational
#define NUMBER_OF_PACKS 8         //1-8
#define BATTERY_TYPE_MASTER 0x52  //0x52 is HV2600 V2 BMS master
#define BATTERY_TYPE_SLAVE 0x82   //0x82 is HV2600 V1, 0x83 is ECS4100 v1, 0x84 is HV2600 V2
#define FIRMWARE_VERSION_MASTER 0xFF
#define FIRMWARE_VERSION_SLAVE 0x20
//for the PACK_ID (b7 =10,20,30,40,50,60,70,80) then FIRMWARE_VERSION 0x1F = 0001 1111, version is v1.15, and if FIRMWARE_VERSION was 0x20 = 0010 0000 then = v2.0
#define MASTER 0
#define MAX_AC_VOLTAGE 2567              //256.7VAC max
#define TOTAL_LIFETIME_WH_ACCUMULATED 0  //We dont have this value in the emulator

/* Do not change code below unless you are sure what you are doing */

void FoxessCanInverter::
    update_values() {  //This function maps all the CAN values fetched from battery. It also checks some safeties.

  //Calculate the required values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  //Put the values into the CAN messages
  //BMS_Limits
  FOXESS_1872.data.u8[0] = (uint8_t)datalayer.battery.info.max_design_voltage_dV;
  FOXESS_1872.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  FOXESS_1872.data.u8[2] = (uint8_t)datalayer.battery.info.min_design_voltage_dV;
  FOXESS_1872.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  FOXESS_1872.data.u8[4] = (uint8_t)datalayer.battery.status.max_charge_current_dA;
  FOXESS_1872.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);
  FOXESS_1872.data.u8[6] = (uint8_t)datalayer.battery.status.max_discharge_current_dA;
  FOXESS_1872.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);

  //BMS_PackData
  FOXESS_1873.data.u8[0] = (uint8_t)datalayer.battery.status.voltage_dV;  // OK
  FOXESS_1873.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  FOXESS_1873.data.u8[2] = (int8_t)datalayer.battery.status.current_dA;  // OK, Signed (Active current in Amps x 10)
  FOXESS_1873.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  FOXESS_1873.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);  //SOC (0-100%)
  FOXESS_1873.data.u8[5] = 0x00;
  FOXESS_1873.data.u8[6] = (uint8_t)(datalayer.battery.status.reported_remaining_capacity_Wh / 10);
  FOXESS_1873.data.u8[7] = ((datalayer.battery.status.reported_remaining_capacity_Wh / 10) >> 8);

  //BMS_CellData
  FOXESS_1874.data.u8[0] = (int8_t)datalayer.battery.status.temperature_max_dC;
  FOXESS_1874.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  FOXESS_1874.data.u8[2] = (int8_t)datalayer.battery.status.temperature_min_dC;
  FOXESS_1874.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
  FOXESS_1874.data.u8[4] = (uint8_t)(3300);  //cut_mv_max (Should we send a limit, or the actual mV?)
  FOXESS_1874.data.u8[5] = (3300 >> 8);
  FOXESS_1874.data.u8[6] = (uint8_t)(3300);  //cut_mV_min (Should we send a limit, or the actual mV?)
  FOXESS_1874.data.u8[7] = (3300 >> 8);

  //BMS_Status
  FOXESS_1875.data.u8[0] = (uint8_t)temperature_average;
  FOXESS_1875.data.u8[1] = (temperature_average >> 8);
  FOXESS_1875.data.u8[2] = (uint8_t)STATUS_OPERATIONAL_PACKS;
  FOXESS_1875.data.u8[3] = (uint8_t)NUMBER_OF_PACKS;
  FOXESS_1875.data.u8[4] = (uint8_t)1;     // Contactor Status 0=off, 1=on.
  FOXESS_1875.data.u8[5] = (uint8_t)0;     //Unused
  FOXESS_1875.data.u8[6] = (uint8_t)0x8E;  //Cycle count
  FOXESS_1875.data.u8[7] = (uint8_t)0;     //Cycle count

  //BMS_PackTemps
  // 0x1876 b0 bit 0 appears to be 1 when at maxsoc and BMS says charge is not allowed -
  // when at 0 indicates charge is possible - additional note there is something more to it than this,
  // it's not as straight forward - needs more testing to find what sets/unsets bit0 of byte0
  if ((datalayer.battery.status.max_charge_current_dA == 0) || (datalayer.battery.status.reported_soc == 10000) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    FOXESS_1876.data.u8[0] = 0x01;
  } else {  //continue using battery
    FOXESS_1876.data.u8[0] = 0x00;
  }

  FOXESS_1876.data.u8[1] = (uint8_t)0;  //Unused
  FOXESS_1876.data.u8[2] = (uint8_t)datalayer.battery.status.cell_max_voltage_mV;
  FOXESS_1876.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  FOXESS_1876.data.u8[4] = (uint8_t)0;  //Unused
  FOXESS_1876.data.u8[5] = (uint8_t)0;  //Unused
  FOXESS_1876.data.u8[6] = (uint8_t)datalayer.battery.status.cell_min_voltage_mV;
  FOXESS_1876.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //BMS_ErrorsBrand
  //0x1877 b0 appears to be an error code, 0x02 when pack is in error.
  if (datalayer.battery.status.bms_status == FAULT) {
    FOXESS_1877.data.u8[0] = (uint8_t)0x02;
  } else {
    FOXESS_1877.data.u8[0] = (uint8_t)0;
  }
  FOXESS_1877.data.u8[1] = (uint8_t)0;  //Unused
  FOXESS_1877.data.u8[2] = (uint8_t)0;  //Unused
  FOXESS_1877.data.u8[3] = (uint8_t)0;  //Unused
  FOXESS_1877.data.u8[5] = (uint8_t)0;  //Unused
  if (current_pack_info == MASTER) {
    FOXESS_1877.data.u8[4] = (uint8_t)BATTERY_TYPE_MASTER;
    FOXESS_1877.data.u8[5] = (uint8_t)0x22;  //Unused?
    FOXESS_1877.data.u8[6] = (uint8_t)FIRMWARE_VERSION_MASTER;
    FOXESS_1877.data.u8[7] = (uint8_t)0x01;
  } else {  // 1-8
    FOXESS_1877.data.u8[4] = (uint8_t)BATTERY_TYPE_SLAVE;
    FOXESS_1877.data.u8[6] = (uint8_t)FIRMWARE_VERSION_SLAVE;
    FOXESS_1877.data.u8[7] = (uint8_t)(current_pack_info << 4);
  }

  //BMS_PackStats
  FOXESS_1878.data.u8[0] = (uint8_t)(MAX_AC_VOLTAGE);
  FOXESS_1878.data.u8[1] = ((MAX_AC_VOLTAGE) >> 8);
  FOXESS_1878.data.u8[2] = (uint8_t)0;  //Unused
  FOXESS_1878.data.u8[3] = (uint8_t)0;  //Unused
  FOXESS_1878.data.u8[4] = (uint8_t)TOTAL_LIFETIME_WH_ACCUMULATED;
  FOXESS_1878.data.u8[5] = (TOTAL_LIFETIME_WH_ACCUMULATED >> 8);
  FOXESS_1878.data.u8[6] = (TOTAL_LIFETIME_WH_ACCUMULATED >> 16);
  FOXESS_1878.data.u8[7] = (TOTAL_LIFETIME_WH_ACCUMULATED >> 24);

  //Errorcodes and flags
  FOXESS_1879.data.u8[0] = (uint8_t)0;  // Error codes go here, still unsure of bitmasking
  if (datalayer.battery.status.current_dA > 0) {
    FOXESS_1879.data.u8[1] = 0x35;  //Charging
  }  // Mappings taken from https://github.com/FozzieUK/FoxESS-Canbus-Protocol
  else {
    FOXESS_1879.data.u8[1] = 0x2B;  //Discharging
  }

  current_pack_info = (current_pack_info + 1);
  if (current_pack_info > NUMBER_OF_PACKS) {
    current_pack_info = 0;
  }

  if (NUMBER_OF_PACKS > 0) {  //div0 safeguard
    //We calculate how much each emulated pack should show
    voltage_per_pack = (datalayer.battery.status.voltage_dV / NUMBER_OF_PACKS);
    current_per_pack = (datalayer.battery.status.current_dA / NUMBER_OF_PACKS);
    if (datalayer.battery.status.temperature_max_dC >= 0) {
      temperature_max_per_pack = (uint8_t)((datalayer.battery.status.temperature_max_dC / 10) + 40);
    } else {  // negative values, cap to 0*C for now. Most LFPs are not allowed to go below 0*C.
      temperature_max_per_pack = 0;
    }  //TODO, make this configurable based on if we detect LFP or not, same as in MODBUS-BYD
    if (datalayer.battery.status.temperature_min_dC >= 0) {
      temperature_min_per_pack = (uint8_t)((datalayer.battery.status.temperature_min_dC / 10) + 40);
    } else {  // negative values, cap to 0*C for now. Most LFPs are not allowed to go below 0*C.
      temperature_min_per_pack = 0;
    }  //TODO, make this configurable based on if we detect LFP or not, same as in MODBUS-BYD
  }

  // Individual pack data
  // Pack 1
  FOXESS_0C05.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C05.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C05.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C05.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C05.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C05.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C05.data.u8[6] = 0xD0;  //pack_1_volts (53.456V) //TODO, does hardcoded value work?
  FOXESS_0C05.data.u8[7] = 0xD0;  //pack_1_volts (53.456V) //Or shall we put in 'voltage_per_pack'

  // Pack 2
  FOXESS_0C06.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C06.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C06.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C06.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C06.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C06.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C06.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C06.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  // Pack 3
  FOXESS_0C07.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C07.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C07.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C07.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C07.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C07.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C07.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C07.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  // Pack 4
  FOXESS_0C08.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C08.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C08.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C08.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C08.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C08.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C08.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C08.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  // Pack 5
  FOXESS_0C09.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C09.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C09.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C09.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C09.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C09.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C09.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C09.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  // Pack 6
  FOXESS_0C0A.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C0A.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C0A.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C0A.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C0A.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C0A.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C0A.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C0A.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  // Pack 7
  FOXESS_0C0B.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C0B.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C0B.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C0B.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C0B.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C0B.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C0B.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C0B.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  // Pack 8
  FOXESS_0C0C.data.u8[0] = (uint8_t)current_per_pack;
  FOXESS_0C0C.data.u8[1] = (current_per_pack >> 8);
  FOXESS_0C0C.data.u8[2] = (uint8_t)temperature_max_per_pack;
  FOXESS_0C0C.data.u8[3] = (uint8_t)temperature_min_per_pack;
  FOXESS_0C0C.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
  FOXESS_0C0C.data.u8[5] = 0x0A;  //b5-7chg/dis?
  FOXESS_0C0C.data.u8[6] = 0xD0;  //pack_1_volts (53.456V)
  FOXESS_0C0C.data.u8[7] = 0xD0;  //pack_1_volts (53.456V)

  //Cellvoltages
  /*
  FOXESS_0C1D.data.u8[0] = (uint8_t)datalayer.battery.status.cell_max_voltage_mV;
  FOXESS_0C1D.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  FOXESS_0C1D.data.u8[2] = 
  FOXESS_0C1D.data.u8[3] = 
  FOXESS_0C1D.data.u8[4] = 
  FOXESS_0C1D.data.u8[5] = 
  FOXESS_0C1D.data.u8[6] = 
  FOXESS_0C1D.data.u8[7] = 
  */
  //TODO map cells somehow in a smart way, extend the 96S of a typical EV battery to 144 cells?
  // Really it does not matter much, since we already send max and min cellvoltage, but we just need to satisfy the inverter

  //Temperatures
  //TODO, should we even bother mapping all these fake values? We already send min and max temperature,
  // So do we really need to map the same two values over and over to 32 places?
}

void FoxessCanInverter::transmit_can(unsigned long currentMillis) {

  if (send_bms_info) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisBMSinfo >= delay_between_batches_ms) {
      previousMillisBMSinfo = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (can_message_bms_index) {
        case 0:
          //TODO, should we limit this incase NUMBER_OF_PACKS =! 8?
          transmit_can_frame(&FOXESS_1872, can_config.inverter);
          transmit_can_frame(&FOXESS_1873, can_config.inverter);
          transmit_can_frame(&FOXESS_1874, can_config.inverter);
          transmit_can_frame(&FOXESS_1875, can_config.inverter);
          break;
        case 1:
          transmit_can_frame(&FOXESS_1876, can_config.inverter);
          transmit_can_frame(&FOXESS_1877, can_config.inverter);
          transmit_can_frame(&FOXESS_1878, can_config.inverter);
          transmit_can_frame(&FOXESS_1879, can_config.inverter);
          send_bms_info = false;
          break;
        default:
          break;
      }

      // Increment message index and wrap around if needed
      can_message_bms_index++;

      if (send_bms_info == false) {
        can_message_bms_index = 0;
      }
    }
  }

  if (send_individual_pack_status) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisIndividualPacks >= delay_between_batches_ms) {
      previousMillisIndividualPacks = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (can_message_individualpack_index) {
        case 0:
          //TODO, should we limit this incase NUMBER_OF_PACKS =! 8?
          transmit_can_frame(&FOXESS_0C05, can_config.inverter);
          transmit_can_frame(&FOXESS_0C06, can_config.inverter);
          transmit_can_frame(&FOXESS_0C07, can_config.inverter);
          transmit_can_frame(&FOXESS_0C08, can_config.inverter);
          break;
        case 1:
          transmit_can_frame(&FOXESS_0C09, can_config.inverter);
          transmit_can_frame(&FOXESS_0C0A, can_config.inverter);
          transmit_can_frame(&FOXESS_0C0B, can_config.inverter);
          transmit_can_frame(&FOXESS_0C0C, can_config.inverter);
          send_individual_pack_status = false;
          break;
        default:
          break;
      }

      // Increment message index and wrap around if needed
      can_message_individualpack_index++;

      if (send_individual_pack_status == false) {
        can_message_individualpack_index = 0;
      }
    }
  }

  if (send_serial_numbers) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisSerialNumber >= delay_between_batches_ms) {
      previousMillisSerialNumber = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (can_message_serial_index) {
        case 0:
          FOXESS_1881.data.u8[0] = 0;
          FOXESS_1882.data.u8[0] = 0;
          FOXESS_1883.data.u8[0] = 0;
          transmit_can_frame(&FOXESS_1881, can_config.inverter);
          transmit_can_frame(&FOXESS_1882, can_config.inverter);
          transmit_can_frame(&FOXESS_1883, can_config.inverter);
          break;
        case 1:
          if (NUMBER_OF_PACKS > 0) {
            FOXESS_1881.data.u8[0] = 1;
            FOXESS_1882.data.u8[0] = 1;
            FOXESS_1883.data.u8[0] = 1;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 2:
          if (NUMBER_OF_PACKS > 1) {
            FOXESS_1881.data.u8[0] = 2;
            FOXESS_1882.data.u8[0] = 2;
            FOXESS_1883.data.u8[0] = 2;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 3:
          if (NUMBER_OF_PACKS > 2) {
            FOXESS_1881.data.u8[0] = 3;
            FOXESS_1882.data.u8[0] = 3;
            FOXESS_1883.data.u8[0] = 3;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 4:
          if (NUMBER_OF_PACKS > 3) {
            FOXESS_1881.data.u8[0] = 4;
            FOXESS_1882.data.u8[0] = 4;
            FOXESS_1883.data.u8[0] = 4;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 5:
          if (NUMBER_OF_PACKS > 4) {
            FOXESS_1881.data.u8[0] = 5;
            FOXESS_1882.data.u8[0] = 5;
            FOXESS_1883.data.u8[0] = 5;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 6:
          if (NUMBER_OF_PACKS > 5) {
            FOXESS_1881.data.u8[0] = 6;
            FOXESS_1882.data.u8[0] = 6;
            FOXESS_1883.data.u8[0] = 6;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 7:
          if (NUMBER_OF_PACKS > 6) {
            FOXESS_1881.data.u8[0] = 7;
            FOXESS_1882.data.u8[0] = 7;
            FOXESS_1883.data.u8[0] = 7;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          break;
        case 8:
          if (NUMBER_OF_PACKS > 7) {
            FOXESS_1881.data.u8[0] = 8;
            FOXESS_1882.data.u8[0] = 8;
            FOXESS_1883.data.u8[0] = 8;
            transmit_can_frame(&FOXESS_1881, can_config.inverter);
            transmit_can_frame(&FOXESS_1882, can_config.inverter);
            transmit_can_frame(&FOXESS_1883, can_config.inverter);
          }
          send_serial_numbers = false;
          break;
        default:
          break;
      }

      // Increment message index and wrap around if needed
      can_message_serial_index++;

      if (send_serial_numbers == false) {
        can_message_serial_index = 0;
      }
    }
  }

  if (send_cellvoltages) {

    // Check if enough time has passed since the last batch
    if (currentMillis - previousMillisCellvoltage >= delay_between_batches_ms) {
      previousMillisCellvoltage = currentMillis;  // Update the time of the last message batch

      // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
      switch (can_message_cellvolt_index) {
        case 0:
          transmit_can_frame(&FOXESS_0C1D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C21, can_config.inverter);
          transmit_can_frame(&FOXESS_0C29, can_config.inverter);
          transmit_can_frame(&FOXESS_0C2D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C31, can_config.inverter);
          break;
        case 1:
          transmit_can_frame(&FOXESS_0C35, can_config.inverter);
          transmit_can_frame(&FOXESS_0C39, can_config.inverter);
          transmit_can_frame(&FOXESS_0C3D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C41, can_config.inverter);
          transmit_can_frame(&FOXESS_0C45, can_config.inverter);
          break;
        case 2:
          transmit_can_frame(&FOXESS_0C49, can_config.inverter);
          transmit_can_frame(&FOXESS_0C4D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C51, can_config.inverter);
          transmit_can_frame(&FOXESS_0C55, can_config.inverter);
          transmit_can_frame(&FOXESS_0C59, can_config.inverter);
          break;
        case 3:
          transmit_can_frame(&FOXESS_0C5D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C61, can_config.inverter);
          transmit_can_frame(&FOXESS_0C65, can_config.inverter);
          transmit_can_frame(&FOXESS_0C69, can_config.inverter);
          transmit_can_frame(&FOXESS_0C6D, can_config.inverter);
          break;
        case 4:
          transmit_can_frame(&FOXESS_0C71, can_config.inverter);
          transmit_can_frame(&FOXESS_0C75, can_config.inverter);
          transmit_can_frame(&FOXESS_0C79, can_config.inverter);
          transmit_can_frame(&FOXESS_0C7D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C81, can_config.inverter);
          break;
        case 5:
          transmit_can_frame(&FOXESS_0C85, can_config.inverter);
          transmit_can_frame(&FOXESS_0C89, can_config.inverter);
          transmit_can_frame(&FOXESS_0C8D, can_config.inverter);
          transmit_can_frame(&FOXESS_0C91, can_config.inverter);
          transmit_can_frame(&FOXESS_0C95, can_config.inverter);
          break;
        case 6:
          transmit_can_frame(&FOXESS_0C99, can_config.inverter);
          transmit_can_frame(&FOXESS_0C9D, can_config.inverter);
          transmit_can_frame(&FOXESS_0CA1, can_config.inverter);
          transmit_can_frame(&FOXESS_0CA5, can_config.inverter);
          transmit_can_frame(&FOXESS_0CA9, can_config.inverter);
          break;
        case 7:  //Celltemperatures
          transmit_can_frame(&FOXESS_0D21, can_config.inverter);
          transmit_can_frame(&FOXESS_0D29, can_config.inverter);
          transmit_can_frame(&FOXESS_0D31, can_config.inverter);
          transmit_can_frame(&FOXESS_0D39, can_config.inverter);
          break;
        case 8:  //Celltemperatures
          transmit_can_frame(&FOXESS_0D41, can_config.inverter);
          transmit_can_frame(&FOXESS_0D49, can_config.inverter);
          transmit_can_frame(&FOXESS_0D51, can_config.inverter);
          transmit_can_frame(&FOXESS_0D59, can_config.inverter);
          send_cellvoltages = false;
          break;
        default:
          break;
      }

      // Increment message index and wrap around if needed
      can_message_cellvolt_index++;

      if (send_cellvoltages == false) {
        can_message_cellvolt_index = 0;
      }
    }
  }
}

void FoxessCanInverter::map_can_frame_to_variable(CAN_frame rx_frame) {

  if (rx_frame.ID == 0x1871) {
    datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
    if (rx_frame.data.u8[0] == 0x03) {  //0x1871 [0x03, 0x06, 0x17, 0x05, 0x09, 0x09, 0x28, 0x22]
//This message is sent by the inverter every '6' seconds (0.5s after the pack serial numbers)
//and contains a timestamp in bytes 2-7 i.e. <YY>,<MM>,<DD>,<HH>,<mm>,<ss>
#ifdef DEBUG_LOG
      logging.println("Inverter sends current time and date");
#endif
    } else if (rx_frame.data.u8[0] == 0x01) {
      if (rx_frame.data.u8[4] == 0x00) {
// Inverter wants to know bms info (every 1s)
#ifdef DEBUG_LOG
        logging.println("Inverter requests 1s BMS info, we reply");
#endif
        send_bms_info = true;
      } else if (rx_frame.data.u8[4] == 0x01) {  // b4 0x01 , 0x1871 [0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
        //Inverter wants to know all individual cellvoltages (occurs 6 seconds after valid BMS reply)
#ifdef DEBUG_LOG
        logging.println("Inverter requests individual battery pack status, we reply");
#endif
        send_individual_pack_status = true;
      } else if (rx_frame.data.u8[4] == 0x04) {  // b4 0x01 , 0x1871 [0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
        //Inverter wants to know all individual cellvoltages (occurs 6 seconds after valid BMS reply)
#ifdef DEBUG_LOG
        logging.println("Inverter requests cellvoltages and temps, we reply");
#endif
        send_cellvoltages = true;
      }
    } else if (rx_frame.data.u8[0] == 0x02) {  //0x1871 [0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00]
// Ack message
#ifdef DEBUG_LOG
      logging.println("Inverter acks, no reply needed");
#endif
    } else if (rx_frame.data.u8[0] == 0x05) {  //0x1871 [0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00]
#ifdef DEBUG_LOG
      logging.println("Inverter wants to know serial numbers, we reply");
#endif
      send_serial_numbers = true;
    }
  }
}
void FoxessCanInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
