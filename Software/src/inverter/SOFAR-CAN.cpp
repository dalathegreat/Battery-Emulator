#include "SOFAR-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

/* This implementation of the SOFAR can protocol is halfway done. What's missing is implementing the inverter replies, all the CAN messages are listed, but the can sending is missing. */

void SofarInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
  SOFAR_351.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  SOFAR_351.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SOFAR_351.data.u8[2] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  SOFAR_351.data.u8[3] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SOFAR_351.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  SOFAR_351.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
  SOFAR_351.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  SOFAR_351.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  //SOC
  SOFAR_355.data.u8[0] = (datalayer.battery.status.reported_soc / 100);
  SOFAR_355.data.u8[2] = (datalayer.battery.status.soh_pptt / 100);
  //SOFAR_355.data.u8[6] = (AH_remaining & 0x00FF);
  //SOFAR_355.data.u8[7] = (AH_remaining >> 8);

  //Voltage (370.0)
  SOFAR_356.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SOFAR_356.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  SOFAR_356.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  SOFAR_356.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  SOFAR_356.data.u8[4] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SOFAR_356.data.u8[5] = (datalayer.battery.status.temperature_max_dC >> 8);
}

void SofarInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //In here we need to respond to the inverter. TODO: make logic
    case 0x605:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //frame1_605 = rx_frame.data.u8[1];
      //frame3_605 = rx_frame.data.u8[3];
      break;
    case 0x705:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //frame1_705 = rx_frame.data.u8[1];
      //frame3_705 = rx_frame.data.u8[3];
      break;
    default:
      break;
  }
}

void SofarInverter::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //Frames actively reported by BMS
    transmit_can_frame(&SOFAR_351, can_config.inverter);
    transmit_can_frame(&SOFAR_355, can_config.inverter);
    transmit_can_frame(&SOFAR_356, can_config.inverter);
    transmit_can_frame(&SOFAR_30F, can_config.inverter);
    transmit_can_frame(&SOFAR_359, can_config.inverter);
    transmit_can_frame(&SOFAR_35E, can_config.inverter);
    transmit_can_frame(&SOFAR_35F, can_config.inverter);
    transmit_can_frame(&SOFAR_35A, can_config.inverter);
  }
}

void SofarInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
