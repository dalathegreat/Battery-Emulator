#include "GROWATT-HV-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

/* TODO:
This protocol has not been tested with any inverter. Proceed with extreme caution.
Search the file for "TODO" to see all the places that might require work /*

/* Growatt BMS CAN-Bus-protocol High Voltage V1.10 2023-11-06
29-bit identifier
500kBit/sec
Big-endian

Terms and abbreviations:
PCS - Power conversion system (the Storage Inverter)
Cell - A single battery cell
Module - A battery module composed of 16 strings of cells
Pack - A battery pack composed of the BMS and battery modules connected in parallel and series, which can work independently
FCC - Full charge capacity
RM - Remaining capacity
BMS - Battery Information Collector*/

void GrowattHvInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    ampere_hours_remaining =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) *
         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
    ampere_hours_full = ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.status.voltage_dV) *
                         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
  }

  //Map values to CAN messages
  //Battery operating parameters and status information
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    //User specified charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_3110.data.u8[0] = (datalayer.battery.settings.max_user_set_charge_voltage_dV >> 8);
    GROWATT_3110.data.u8[1] = (datalayer.battery.settings.max_user_set_charge_voltage_dV & 0x00FF);
  } else {
    //Battery max voltage used as charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_3110.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV >> 8);
    GROWATT_3110.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  }
  //Charge limited current, 125 =12.5A (0.1, A) (Min 0, Max 300A)
  GROWATT_3110.data.u8[2] = (datalayer.battery.status.max_charge_current_dA >> 8);
  GROWATT_3110.data.u8[3] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  //Discharge limited current, 500 = 50A, (0.1, A)
  GROWATT_3110.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  GROWATT_3110.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Status bits (see documentation for all bits, most important are mapped
  GROWATT_3110.data.u8[7] = 0x00;                      // Clear all bits
  if (datalayer.battery.status.active_power_W < -1) {  // Discharging
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b00000011);
  } else if (datalayer.battery.status.active_power_W > 1) {  // Charging
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b00000010);
  } else {  //Idle
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b00000001);
  }
  if ((datalayer.battery.status.max_charge_current_dA == 0) || (datalayer.battery.status.reported_soc == 10000) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b01000000);  // No Charge
  } else {                                                             //continue using battery
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b00000000);  // Charge allowed
  }
  if ((datalayer.battery.status.max_discharge_current_dA == 0) || (datalayer.battery.status.reported_soc == 0) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b00100000);  // No Discharge
  } else {                                                             //continue using battery
    GROWATT_3110.data.u8[7] = (GROWATT_3110.data.u8[7] | 0b00000000);  // Discharge allowed
  }
  GROWATT_3110.data.u8[6] = (GROWATT_3110.data.u8[6] | 0b00100000);  // ISO Detection status: Detected
  GROWATT_3110.data.u8[6] = (GROWATT_3110.data.u8[6] | 0b00010000);  // Battery status: Normal

  //Battery protection and alarm information
  //Fault and warning status bits. TODO, map these according to documentation.
  //GROWATT_3120.data.u8[0] =
  //GROWATT_3120.data.u8[1] =
  //GROWATT_3120.data.u8[2] =
  //GROWATT_3120.data.u8[3] =
  //GROWATT_3120.data.u8[4] =
  //GROWATT_3120.data.u8[5] =
  //GROWATT_3120.data.u8[6] =
  //GROWATT_3120.data.u8[7] =

  //Battery operation information
  //Voltage of the pack (0.1V) [0-1000V]
  GROWATT_3130.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  GROWATT_3130.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Total current (0.1A -300 to 300A)
  GROWATT_3130.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  GROWATT_3130.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Cell max temperature (0.1C) [-40 to 120*C]
  GROWATT_3130.data.u8[4] = (datalayer.battery.status.temperature_max_dC >> 8);
  GROWATT_3130.data.u8[5] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //SOC (%) [0-100]
  GROWATT_3130.data.u8[6] = (datalayer.battery.status.reported_soc / 100);
  //SOH (%) (Bit 0~ Bit6 SOH Counters) Bit7 SOH flag (Indicates that battery is in unsafe use)
  GROWATT_3130.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  //Battery capacity information
  //Remaining capacity (10 mAh) [0.0 ~ 500000.0 mAH]
  GROWATT_3140.data.u8[0] = ((ampere_hours_remaining * 100) >> 8);
  GROWATT_3140.data.u8[1] = ((ampere_hours_remaining * 100) & 0x00FF);
  //Fully charged capacity (10 mAh) [0.0 ~ 500000.0 mAH]
  GROWATT_3140.data.u8[2] = ((ampere_hours_full * 100) >> 8);
  GROWATT_3140.data.u8[3] = ((ampere_hours_full * 100) & 0x00FF);
  //Manufacturer code
  GROWATT_3140.data.u8[4] = MANUFACTURER_ASCII_0;
  GROWATT_3140.data.u8[5] = MANUFACTURER_ASCII_1;
  //Cycle count (h)
  GROWATT_3140.data.u8[6] = 0;
  GROWATT_3140.data.u8[7] = 0;

  //Battery working parameters and module number information
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    //Use user specified voltage as Discharge cutoff voltage (0.1V) [0-1000V]
    GROWATT_3150.data.u8[0] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV >> 8);
    GROWATT_3150.data.u8[1] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV & 0x00FF);
  } else {
    //Use battery min design voltage as Discharge cutoff voltage (0.1V) [0-1000V]
    GROWATT_3150.data.u8[0] = (datalayer.battery.info.min_design_voltage_dV >> 8);
    GROWATT_3150.data.u8[1] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  }
  //Main control unit temperature (0.1C) [-40 to 120*C]
  GROWATT_3150.data.u8[2] = (datalayer.battery.status.temperature_max_dC >> 8);
  GROWATT_3150.data.u8[3] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //Total number of cells
  GROWATT_3150.data.u8[4] = (TOTAL_NUMBER_OF_CELLS >> 8);
  GROWATT_3150.data.u8[5] = (TOTAL_NUMBER_OF_CELLS & 0x00FF);
  //Number of modules in series
  GROWATT_3150.data.u8[6] = (NUMBER_OF_MODULES_IN_SERIES >> 8);
  GROWATT_3150.data.u8[7] = (NUMBER_OF_MODULES_IN_SERIES & 0x00FF);

  //Battery fault and voltage number information
  //Fault flag bit
  GROWATT_3160.data.u8[0] = 0;  //TODO: Map according to documentation
  //Fault extension flag bit
  GROWATT_3160.data.u8[1] = 0;  //TODO: Map according to documentation
  //Number of module with the maximum cell voltage (1-32)
  GROWATT_3160.data.u8[2] = 1;
  //Number of cell with the maximum cell voltage (1-128)
  GROWATT_3160.data.u8[3] = 1;
  //Number of module with the minimum cell voltage (1-32)
  GROWATT_3160.data.u8[4] = 1;
  //Number of cell with the minimum cell voltage (1-128)
  GROWATT_3160.data.u8[5] = 2;
  //Minimum cell temperature (0.1C) [-40 to 120*C]
  GROWATT_3160.data.u8[6] = (datalayer.battery.status.temperature_min_dC >> 8);
  GROWATT_3160.data.u8[7] = (datalayer.battery.status.temperature_min_dC & 0x00FF);

  //Software version and temperature number information
  //Number of Module with the maximum cell temperature (1-32)
  GROWATT_3170.data.u8[0] = 1;
  //Number of cell with the maximum cell temperature (1-128)
  GROWATT_3170.data.u8[1] = 1;
  //Number of module with the minimum cell temperature (1-32)
  GROWATT_3170.data.u8[2] = 1;
  //Number of cell with the minimum cell temperature (1-128)
  GROWATT_3170.data.u8[3] = 2;
  //Battery actial capacity (0-100) TODO, what is unit?
  GROWATT_3170.data.u8[4] = 50;
  //Battery correction status display value (0-255)
  GROWATT_3170.data.u8[5] = 0;
  // Remaining balancing time (0-255)
  GROWATT_3170.data.u8[6] = 0;
  //Balancing state, bit0-3(range 0-15) , Internal short circuit state, bit4-7 (range 0-15)
  GROWATT_3170.data.u8[7] = 0;

  //Battery Code and quantity information
  //Manufacturer code
  GROWATT_3180.data.u8[0] = MANUFACTURER_ASCII_0;
  GROWATT_3180.data.u8[1] = MANUFACTURER_ASCII_1;
  //Number of Packs in parallel (1-65536)
  GROWATT_3180.data.u8[2] = (NUMBER_OF_PACKS_IN_PARALLEL >> 8);
  GROWATT_3180.data.u8[3] = (NUMBER_OF_PACKS_IN_PARALLEL & 0x00FF);
  //Total number of cells (1-65536)
  GROWATT_3180.data.u8[4] = (TOTAL_NUMBER_OF_CELLS >> 8);
  GROWATT_3180.data.u8[5] = (TOTAL_NUMBER_OF_CELLS & 0x00FF);
  //Pack number + BIC forward/reverse encoding number
  // Bits 0-3: Pack number
  // Bits 4-9: Max. number of BIC in forward BIC encoding in daisy- chain communication
  // Bits 10-15: Max. number of BIC in reverse BIC encoding in daisy- chain communication
  GROWATT_3180.data.u8[6] = 0;  //TODO, this OK?
  GROWATT_3180.data.u8[7] = 0;  //TODO, this OK?

  //Cell voltage and status information
  //Battery status
  GROWATT_3190.data.u8[0] = 0;  //LFP, no forced charge
  //Maximum cell voltage (mV)
  GROWATT_3190.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  GROWATT_3190.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  // Min cell voltage (mV)
  GROWATT_3190.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  GROWATT_3190.data.u8[4] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  //Reserved
  GROWATT_3190.data.u8[5] = 0;
  // Faulty battery pack number (1-16)
  GROWATT_3190.data.u8[6] = 0;
  //Faulty battery module number (1-16)
  GROWATT_3190.data.u8[7] = 0;

  //Manufacturer name and version information
  // Manufacturer name (ASCII) Battery manufacturer abbreviation in capital letters
  GROWATT_3200.data.u8[0] = MANUFACTURER_ASCII_0;
  GROWATT_3200.data.u8[1] = MANUFACTURER_ASCII_1;
  // Hardware revision (0 null, 1 verA, 2verB)
  GROWATT_3200.data.u8[2] = 0x01;
  // Reserved
  GROWATT_3200.data.u8[3] = 0;  //Reserved
  //Circulating current value (0.1A), Range 0-20A
  GROWATT_3200.data.u8[4] = 0;
  GROWATT_3200.data.u8[5] = 0;
  //Cell charge cutoff voltage
  GROWATT_3200.data.u8[6] = (datalayer.battery.info.max_cell_voltage_mV >> 8);
  GROWATT_3200.data.u8[7] = (datalayer.battery.info.max_cell_voltage_mV & 0x00FF);

  //Upgrade information
  //Message 0x3210 is update status. All blank is OK
  //GROWATT_3210.data.u8[0] = 0;

  //De-rating and fault information (reserved)
  //Power reduction sign
  GROWATT_3220.data.u8[0] = 0;  //Bits set to high incase we need to derate
  GROWATT_3220.data.u8[1] = 0;  //Bits set to high incase we need to derate
  //System fault status
  GROWATT_3220.data.u8[2] = 0;  //All normal
  GROWATT_3220.data.u8[3] = 0;  //All normal
  //Forced discharge mark
  GROWATT_3220.data.u8[4] = 0;  //When you want to force charge battery, send 0xAA here
  //Battery rated energy information (Unit 0.1 kWh )  30kWh = 300 , so 30000Wh needs to be div by 100
  GROWATT_3220.data.u8[5] = ((datalayer.battery.info.total_capacity_Wh / 100) >> 8);
  GROWATT_3220.data.u8[6] = ((datalayer.battery.info.total_capacity_Wh / 100) & 0x00FF);
  //Software subversion number
  GROWATT_3220.data.u8[7] = 0;

  //Serial number
  //Frame number
  GROWATT_3230.data.u8[0] = serial_number_counter;
  //Serial number content
  //The serial number includes the PACK number (1byte: range [1, 11]) and serial number (16bytes).
  //(Reserved and filled with 0x00) Explanation: Byte 1 (Battery ID) = 0:Invalid.
  //When Byte 1 (Battery ID) = 1, it represents the SN (Serial Number) of the
  //high-voltage controller. When BYTE1 (Battery ID) = 2~11, it represents the SN of PACK 1~10.
  switch (serial_number_counter) {
    case 0:
      GROWATT_3230.data.u8[1] = 0;  // BATTERY ID
      GROWATT_3230.data.u8[2] = 0;  // SN0 //TODO, is this needed?
      GROWATT_3230.data.u8[3] = 0;  // SN1
      GROWATT_3230.data.u8[4] = 0;  // SN2
      GROWATT_3230.data.u8[5] = 0;  // SN3
      GROWATT_3230.data.u8[6] = 0;  // SN4
      GROWATT_3230.data.u8[7] = 0;  // SN5
      break;
    case 1:
      GROWATT_3230.data.u8[1] = 0;  // SN6
      GROWATT_3230.data.u8[2] = 0;  // SN7
      GROWATT_3230.data.u8[3] = 0;  // SN8
      GROWATT_3230.data.u8[4] = 0;  // SN9
      GROWATT_3230.data.u8[5] = 0;  // SN10
      GROWATT_3230.data.u8[6] = 0;  // SN11
      GROWATT_3230.data.u8[7] = 0;  // SN12
      break;
    case 2:
      GROWATT_3230.data.u8[1] = 0;  // SN13
      GROWATT_3230.data.u8[2] = 0;  // SN14
      GROWATT_3230.data.u8[3] = 0;  // SN15
      GROWATT_3230.data.u8[4] = 0;  // RESERVED
      GROWATT_3230.data.u8[5] = 0;  // RESERVED
      GROWATT_3230.data.u8[6] = 0;  // RESERVED
      GROWATT_3230.data.u8[7] = 0;  // RESERVED
      break;
    default:
      break;
  }
  serial_number_counter = (serial_number_counter + 1) % 3;  // cycles between 0-1-2-0-1...

  //Total charge/discharge energy
  //Pack number (1-16)
  GROWATT_3240.data.u8[0] = 1;
  //Total lifetime discharge energy Unit: 0.1kWh Range [0.0~10000000.0kWh]
  GROWATT_3240.data.u8[1] = 0;
  GROWATT_3240.data.u8[2] = 0;
  GROWATT_3240.data.u8[3] = 0;
  //Pack number (1-16)
  GROWATT_3240.data.u8[4] = 1;
  //Total lifetime charge energy Unit: 0.1kWh Range [0.0~10000000.0kWh]
  GROWATT_3240.data.u8[5] = 0;
  GROWATT_3240.data.u8[6] = 0;
  GROWATT_3240.data.u8[7] = 0;

  //Fault history
  // Not applicable for our use. All values at 0 indicates no fault
  GROWATT_3250.data.u8[0] = 0;
  GROWATT_3250.data.u8[1] = 0;
  GROWATT_3250.data.u8[2] = 0;
  GROWATT_3250.data.u8[3] = 0;
  GROWATT_3250.data.u8[4] = 0;
  GROWATT_3250.data.u8[5] = 0;
  GROWATT_3250.data.u8[6] = 0;
  GROWATT_3250.data.u8[7] = 0;

  //Battery internal debugging fault message
  // Not applicable for our use. All values at 0 indicates no fault
  GROWATT_3260.data.u8[0] = 0;
  GROWATT_3260.data.u8[1] = 0;
  GROWATT_3260.data.u8[2] = 0;
  GROWATT_3260.data.u8[3] = 0;
  GROWATT_3260.data.u8[4] = 0;
  GROWATT_3260.data.u8[5] = 0;
  GROWATT_3260.data.u8[6] = 0;
  GROWATT_3260.data.u8[7] = 0;

  //Battery internal debugging fault message
  // Not applicable for our use. All values at 0 indicates no fault
  GROWATT_3270.data.u8[0] = 0;
  GROWATT_3270.data.u8[1] = 0;
  GROWATT_3270.data.u8[2] = 0;
  GROWATT_3270.data.u8[3] = 0;
  GROWATT_3270.data.u8[4] = 0;
  GROWATT_3270.data.u8[5] = 0;
  GROWATT_3270.data.u8[6] = 0;
  GROWATT_3270.data.u8[7] = 0;

  //Product Version Information
  GROWATT_3280.data.u8[0] = 0;  //Reserved
  //Product version number (1-5)
  GROWATT_3280.data.u8[1] = 0;  //No software version (1 indicates PRODUCT, 2 indicates COMMUNICATION version)
  //For example, the version code for the main control unit (product) of a high-voltage battery is QBAA,
  // and the code formonitoring (communication)is ZEAA
  //TODO, is this needed?
  GROWATT_3280.data.u8[2] = 0;  //ASCII
  GROWATT_3280.data.u8[3] = 0;  //ASCII
  GROWATT_3280.data.u8[4] = 0;  //ASCII
  GROWATT_3280.data.u8[5] = 0;  //ASCII
  GROWATT_3280.data.u8[6] = 0;  //Software version information

  //Battery series information
  GROWATT_3290.data.u8[0] = 0;  // DTC, range Range [0,65536], Default 12041 (TODO, shall we send that?)
  GROWATT_3290.data.u8[1] = 0;  // RESERVED
  GROWATT_3290.data.u8[2] = 0;  // RESERVED
  GROWATT_3290.data.u8[3] = 0;  // RESERVED
  GROWATT_3290.data.u8[4] = 0;  // RESERVED
  GROWATT_3290.data.u8[5] = 0;  // RESERVED
  GROWATT_3290.data.u8[6] = 0;  // RESERVED
  GROWATT_3290.data.u8[7] = 0;  // RESERVED

  //Internal alarm information
  //Battery internal debugging fault message
  GROWATT_3F00.data.u8[0] = 0;  // RESERVED
  GROWATT_3F00.data.u8[1] = 0;  // RESERVED
  GROWATT_3F00.data.u8[2] = 0;  // RESERVED
  GROWATT_3F00.data.u8[3] = 0;  // RESERVED
  GROWATT_3F00.data.u8[4] = 0;  // RESERVED
  GROWATT_3F00.data.u8[5] = 0;  // RESERVED
  GROWATT_3F00.data.u8[6] = 0;  // RESERVED
  GROWATT_3F00.data.u8[7] = 0;  // RESERVED
}

void GrowattHvInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x3010:  // Heartbeat command, 1000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      send_times = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      safety_specification = rx_frame.data.u8[2];
      break;
    case 0x3020:  // Control command, 1000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      charging_command = rx_frame.data.u8[0];
      discharging_command = rx_frame.data.u8[1];
      shielding_external_communication_failure = rx_frame.data.u8[2];
      clearing_battery_fault = rx_frame.data.u8[3];
      ISO_detection_command = rx_frame.data.u8[4];
      sleep_wakeup_control = rx_frame.data.u8[7];
      break;
    case 0x3030:  // Time command, 1000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      unix_time = ((rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) |
                   rx_frame.data.u8[3]);
      PCS_working_status = rx_frame.data.u8[7];
      break;
    default:
      break;
  }
}

void GrowattHvInverter::transmit_can(unsigned long currentMillis) {

  if (!inverter_alive) {
    return;  //Dont send messages towards inverter until it has started
  }

  //Check if 1 second has passed, then we start sending!
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;
    time_to_send_1s_data = true;
  }

  // Check if enough time has passed since the last batch
  if (currentMillis - previousMillisBatchSend >= delay_between_batches_ms) {
    previousMillisBatchSend = currentMillis;  // Update the time of the last message batch

    // Send a subset of messages per iteration to avoid overloading the CAN bus / transmit buffer
    switch (can_message_batch_index) {
      case 0:
        transmit_can_frame(&GROWATT_3110, can_config.inverter);
        transmit_can_frame(&GROWATT_3120, can_config.inverter);
        transmit_can_frame(&GROWATT_3130, can_config.inverter);
        transmit_can_frame(&GROWATT_3140, can_config.inverter);
        break;
      case 1:
        transmit_can_frame(&GROWATT_3150, can_config.inverter);
        transmit_can_frame(&GROWATT_3160, can_config.inverter);
        transmit_can_frame(&GROWATT_3170, can_config.inverter);
        transmit_can_frame(&GROWATT_3180, can_config.inverter);
        break;
      case 2:
        transmit_can_frame(&GROWATT_3190, can_config.inverter);
        transmit_can_frame(&GROWATT_3200, can_config.inverter);
        transmit_can_frame(&GROWATT_3210, can_config.inverter);
        transmit_can_frame(&GROWATT_3220, can_config.inverter);
        break;
      case 3:
        transmit_can_frame(&GROWATT_3230, can_config.inverter);
        transmit_can_frame(&GROWATT_3240, can_config.inverter);
        transmit_can_frame(&GROWATT_3250, can_config.inverter);
        transmit_can_frame(&GROWATT_3260, can_config.inverter);
        break;
      case 4:
        transmit_can_frame(&GROWATT_3270, can_config.inverter);
        transmit_can_frame(&GROWATT_3280, can_config.inverter);
        transmit_can_frame(&GROWATT_3290, can_config.inverter);
        transmit_can_frame(&GROWATT_3F00, can_config.inverter);
        time_to_send_1s_data = false;
        break;
      default:
        break;
    }

    // Increment message index and wrap around if needed
    can_message_batch_index++;

    if (time_to_send_1s_data == false) {
      can_message_batch_index = 0;
    }
  }
}

void GrowattHvInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
