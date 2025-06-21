#include "AFORE-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

#define SOCMAX 100
#define SOCMIN 1

/* Do not change code below unless you are sure what you are doing */

void AforeCanInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  //There are more mappings that could be added, but this should be enough to use as a starting point

  /*0x350 Operation Information*/
  AFORE_350.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  AFORE_350.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  //Total battery current unit: 0.1A offset 5000; positive is charging,
  //Negative means discharge; for example: 1A = (5010-5000)/10
  AFORE_350.data.u8[2] = ((datalayer.battery.status.current_dA + 5000) & 0x00FF);
  AFORE_350.data.u8[3] = ((datalayer.battery.status.current_dA + 5000) >> 8);
  //Battery temperature unit: 0.1C offset 1000; for example: 20C
  //= (1200 -1000)/10; when all temperatures are greater than or equal to 0
  AFORE_350.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  AFORE_350.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);

  /*0x351 -  Battery information*/
  AFORE_351.data.u8[0] = (datalayer.battery.status.reported_soc / 100);  //Remove decimals, 0-100
  AFORE_351.data.u8[1] = (datalayer.battery.status.soh_pptt / 100);      //Remove decimals, 0-100
  AFORE_351.data.u8[2] = SOCMAX;
  AFORE_351.data.u8[3] = SOCMIN;
  AFORE_351.data.u8[4] = 0x03;  //Bit0 and Bit1 set
  if ((datalayer.battery.status.max_charge_current_dA == 0) || (datalayer.battery.status.reported_soc == 10000) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    AFORE_351.data.u8[4] &= ~0x01;  // Remove Bit0 (clear) Charge enable flag
  }
  if ((datalayer.battery.status.max_discharge_current_dA == 0) || (datalayer.battery.status.reported_soc == 0) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    AFORE_351.data.u8[4] &= ~0x02;  // Remove Bit1 (clear) Discharge enable flag
  }
  // Bit5-7 is BMS working status.
  //A value of 0 here is INIT, 1 = Normal operation, 2 = standby/sleep, 3 = warning, 4 = fault, rest is reserved
  AFORE_351.data.u8[4] &= ~(0xE0);  // Clear bits 5, 6, and 7 (11100000)
  if (datalayer.battery.status.bms_status == FAULT) {
    AFORE_351.data.u8[4] |= 0x80;  // Set bits 5-7 to 0b100 (Fault = 4)
  } else {                         // Normal mode
    AFORE_351.data.u8[4] |= 0x20;  // Set Bit5 to 1 (Normal operation)
  }
  AFORE_351.data.u8[6] = (datalayer.battery.info.number_of_cells & 0x00FF);
  AFORE_351.data.u8[7] = (datalayer.battery.info.number_of_cells >> 8);

  /*0x352 - Protection parameters*/
  AFORE_352.data.u8[0] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  AFORE_352.data.u8[1] = (datalayer.battery.status.max_charge_current_dA >> 8);
  AFORE_352.data.u8[2] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  AFORE_352.data.u8[3] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  AFORE_352.data.u8[4] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  AFORE_352.data.u8[5] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  AFORE_352.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  AFORE_352.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  /*0x353 - Fault information*/
  /* Fault H, bit, definitions
  0 ErrCellOverVolt
  1 ErrCellUnderVolt
  2 ErrOverVolt
  3 ErrUnderVolt
  4 ErrOverTemperature
  5 ErrUnderTemperature
  6 ErrBatIsolation
  7 ErrChargeCurrentOver
  8 ErrDischargeCurrentOver
  9 ErrSOCtooLow
  10 ErrBatteryInternalCommunicationFailure
  11 ErrBMSVoltageSupplyTooHigh
  12 ErrBMSVoltageSupplyTooLow
  13 ErrRelayFailure
  14 ErrPreChargerFailure
  15 InterlockOpen
  AFORE_353.data.u8[0] = Fault H table & 0x00FF
  AFORE_353.data.u8[1] = Fault H table >> 8);
  /* Fault L, bit, definitions
  8 VoltageInterlockShortCircuit
  9 SystemFailure
  10 ErrorChargeReferenceOvervoltage
  11 ChargingMOSdamaged
  12 DischargeMOSdamaged
  13 CellOverTemperature
  14 CellUnderTemperature
  15 CellUnbalance
  AFORE_353.data.u8[2] = Fault L table & 0x00FF
  AFORE_353.data.u8[3] = Fault L table >> 8);
  */

  /*0x354 - Single cell voltage parameters*/
  AFORE_354.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  AFORE_354.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  AFORE_354.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  AFORE_354.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  AFORE_354.data.u8[4] = (1 & 0x00FF);  //Maximum single cell voltage number, not used on emulator
  AFORE_354.data.u8[5] = (1 >> 8);
  AFORE_354.data.u8[6] = (2 & 0x00FF);  //Minimum single cell voltage number, not used on emulator
  AFORE_354.data.u8[7] = (2 >> 8);

  /*0x355 - Single cell temperature parameters*/
  AFORE_355.data.u8[0] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  AFORE_355.data.u8[1] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  AFORE_355.data.u8[2] = ((datalayer.battery.status.temperature_min_dC + 1000) & 0x00FF);
  AFORE_355.data.u8[3] = ((datalayer.battery.status.temperature_min_dC + 1000) >> 8);
  AFORE_355.data.u8[4] = (1 & 0x00FF);  //Maximum single cell temperature number, not used on emulator
  AFORE_355.data.u8[5] = (1 >> 8);
  AFORE_355.data.u8[6] = (2 & 0x00FF);  //Minimum single cell temperature number, not used on emulator
  AFORE_355.data.u8[7] = (2 >> 8);

  /*0x356 - Single cell protection parameters*/
  AFORE_356.data.u8[0] = (datalayer.battery.info.max_cell_voltage_mV & 0x00FF);
  AFORE_356.data.u8[1] = (datalayer.battery.info.max_cell_voltage_mV >> 8);
  AFORE_356.data.u8[2] = (datalayer.battery.info.min_cell_voltage_mV & 0x00FF);
  AFORE_356.data.u8[3] = (datalayer.battery.info.min_cell_voltage_mV >> 8);

  /*0x357 - Warning information*/
  /* Warning, bit, definitions
  0 CellOverVolt
  1 CellUnderVolt
  2 OverVolt
  3 UnderVolt
  4 CellOverTemperature
  5 CellUnderTemperature
  6 TempOver
  7 TempUnder
  8 ChgCurrOver
  9 ErrSOCtooLow
  10 SocLow
  AFORE_357.data.u8[0] = Warning table & 0x00FF
  AFORE_357.data.u8[1] = Warning table >> 8);
  */
}

void AforeCanInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:  // Every 1s from inverter
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      char0 = rx_frame.data.u8[0];  // A
      char1 = rx_frame.data.u8[0];  // F
      char2 = rx_frame.data.u8[0];  // O
      char3 = rx_frame.data.u8[0];  // R
      char4 = rx_frame.data.u8[0];  // E
      inverter_status = rx_frame.data.u8[7];
      time_to_send_info = true;
      break;
    default:
      break;
  }
}

void AforeCanInverter::transmit_can(unsigned long currentMillis) {
  if (time_to_send_info) {  // Set every 1s if we get message from inverter
    transmit_can_frame(&AFORE_350, can_config.inverter);
    transmit_can_frame(&AFORE_351, can_config.inverter);
    transmit_can_frame(&AFORE_352, can_config.inverter);
    transmit_can_frame(&AFORE_353, can_config.inverter);
    transmit_can_frame(&AFORE_354, can_config.inverter);
    transmit_can_frame(&AFORE_355, can_config.inverter);
    transmit_can_frame(&AFORE_356, can_config.inverter);
    transmit_can_frame(&AFORE_357, can_config.inverter);
    transmit_can_frame(&AFORE_358, can_config.inverter);
    transmit_can_frame(&AFORE_359, can_config.inverter);
    transmit_can_frame(&AFORE_35A, can_config.inverter);
    time_to_send_info = false;
  }
}

void AforeCanInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
