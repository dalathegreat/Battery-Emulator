#include "SMA-LV-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

/* SMA Sunny Island Low Voltage (48V) CAN protocol:
CAN 2.0A
500kBit/sec
11-Bit Identifiers */

void SmaLvInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  // Update values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  //Map values to CAN messages

  //Battery charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 41V, MAX 63V, default 54V)
  SMA_351.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) >> 8);
  SMA_351.data.u8[1] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) & 0x00FF);
  if (datalayer.battery.info.max_design_voltage_dV > MAX_VOLTAGE_DV) {
    //If the battery is designed for more than 63.0V, cap the value
    SMA_351.data.u8[0] = (MAX_VOLTAGE_DV >> 8);
    SMA_351.data.u8[1] = (MAX_VOLTAGE_DV & 0x00FF);
    //TODO; raise event?
  }
  //Discharge limited current, 500 = 50A, (0.1, A) (MIN 0, MAX 1200)
  SMA_351.data.u8[2] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  SMA_351.data.u8[3] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Charge limited current, 125 =12.5A (0.1, A) (MIN 0, MAX 1200)
  SMA_351.data.u8[4] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SMA_351.data.u8[5] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  //Discharge voltage (eg 300.0V = 3000 , 16bits long) (MIN 41V, MAX 48V, default 41V)
  SMA_351.data.u8[6] = ((datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV) >> 8);
  SMA_351.data.u8[7] = ((datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV) & 0x00FF);
  if (datalayer.battery.info.min_design_voltage_dV < MIN_VOLTAGE_DV) {
    //If the battery is designed for discharge voltage below 41.0V, cap the value
    SMA_351.data.u8[6] = (MIN_VOLTAGE_DV >> 8);
    SMA_351.data.u8[7] = (MIN_VOLTAGE_DV & 0x00FF);
    //TODO; raise event?
  }

  //SOC (100%)
  SMA_355.data.u8[0] = ((datalayer.battery.status.reported_soc / 100) >> 8);
  SMA_355.data.u8[1] = ((datalayer.battery.status.reported_soc / 100) & 0x00FF);
  //StateOfHealth (100%)
  SMA_355.data.u8[2] = ((datalayer.battery.status.soh_pptt / 100) >> 8);
  SMA_355.data.u8[3] = ((datalayer.battery.status.soh_pptt / 100) & 0x00FF);
  //State of charge High Precision (100.00%)
  SMA_355.data.u8[4] = (datalayer.battery.status.reported_soc >> 8);
  SMA_355.data.u8[5] = (datalayer.battery.status.reported_soc & 0x00FF);

  //Voltage (370.0)
  SMA_356.data.u8[0] = ((datalayer.battery.status.voltage_dV * 10) >> 8);
  SMA_356.data.u8[1] = ((datalayer.battery.status.voltage_dV * 10) & 0x00FF);
  //Current (S16 dA)
  SMA_356.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  SMA_356.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Temperature (s16 degC)
  SMA_356.data.u8[4] = (temperature_average >> 8);
  SMA_356.data.u8[5] = (temperature_average & 0x00FF);

  //TODO: Map error/warnings in 0x35A
}

void SmaLvInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Frame0-1 Battery Voltage
      //Frame2-3 Battery Current
      //Frame4-5 Battery Temperature
      //Frame6-7 SOC Battery
      break;
    case 0x306:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Frame0-1 SOH Battery
      //Frame2 Charging procedure
      //Frame3 Operating state
      //Frame4-5 Active error message
      //Frame6-7 Battery charge voltage setpoint
      break;
    default:
      break;
  }
}

void SmaLvInverter::transmit_can(unsigned long currentMillis) {

  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&SMA_351, can_config.inverter);
    transmit_can_frame(&SMA_355, can_config.inverter);
    transmit_can_frame(&SMA_356, can_config.inverter);
    transmit_can_frame(&SMA_35A, can_config.inverter);
    transmit_can_frame(&SMA_35B, can_config.inverter);
    transmit_can_frame(&SMA_35E, can_config.inverter);
    transmit_can_frame(&SMA_35F, can_config.inverter);

    //Remote quick stop (optional)
    if (datalayer.battery.status.bms_status == FAULT) {
      transmit_can_frame(&SMA_00F, can_config.inverter);
      //After receiving this message, Sunny Island will immediately go into standby.
      //Please send start command, to start again. Manual start is also possible.
    }
  }
}

void SmaLvInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
