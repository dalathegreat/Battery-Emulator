#include "PYLON-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

void PylonInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  //Check what discharge and charge cutoff voltages to send
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    discharge_cutoff_voltage_dV = datalayer.battery.settings.max_user_set_discharge_voltage_dV;
    charge_cutoff_voltage_dV = datalayer.battery.settings.max_user_set_charge_voltage_dV;
  } else {
    discharge_cutoff_voltage_dV = (datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV);
    charge_cutoff_voltage_dV = (datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV);
  }

  //There are more mappings that could be added, but this should be enough to use as a starting point
  // Note we map both 0 and 1 messages

  //Charge / Discharge allowed
  PYLON_4280.data.u8[0] = 0;
  PYLON_4280.data.u8[1] = 0;
  PYLON_4280.data.u8[2] = 0;
  PYLON_4280.data.u8[3] = 0;
  PYLON_4281.data.u8[0] = 0;
  PYLON_4281.data.u8[1] = 0;
  PYLON_4281.data.u8[2] = 0;
  PYLON_4281.data.u8[3] = 0;

  //Voltage (370.0)
  PYLON_4210.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  PYLON_4210.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  PYLON_4211.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  PYLON_4211.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);

  //Current (15.0)
  PYLON_4210.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  PYLON_4210.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  PYLON_4211.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  PYLON_4211.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);

  // BMS Temperature (We dont have BMS temp, send max cell voltage instead)

  if (INVERT_LOW_HIGH_BYTES) {  //Useful for Sofar inverters
    PYLON_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
    PYLON_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
    PYLON_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
    PYLON_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  } else {  // Not INVERT_LOW_HIGH_BYTES
    PYLON_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
    PYLON_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
    PYLON_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
    PYLON_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  }
  //SOC (100.00%)
  PYLON_4210.data.u8[6] = (datalayer.battery.status.reported_soc / 100);  //Remove decimals
  PYLON_4211.data.u8[6] = (datalayer.battery.status.reported_soc / 100);  //Remove decimals

  //StateOfHealth (100.00%)
  PYLON_4210.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);
  PYLON_4211.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  // Status=Bit 0,1,2= 0:Sleep, 1:Charge, 2:Discharge 3:Idle. Bit3 ForceChargeReq. Bit4 Balance charge Request
  if (datalayer.battery.status.bms_status == FAULT) {
    PYLON_4250.data.u8[0] = (0x00);  // Sleep
    PYLON_4251.data.u8[0] = (0x00);  // Sleep
  } else if (datalayer.battery.status.current_dA < 0) {
    PYLON_4250.data.u8[0] = (0x01);  // Charge
    PYLON_4251.data.u8[0] = (0x01);  // Charge
  } else if (datalayer.battery.status.current_dA > 0) {
    PYLON_4250.data.u8[0] = (0x02);  // Discharge
    PYLON_4251.data.u8[0] = (0x02);  // Discharge
  } else if (datalayer.battery.status.current_dA == 0) {
    PYLON_4250.data.u8[0] = (0x03);  // Idle
    PYLON_4251.data.u8[0] = (0x03);  // Idle
  }

  if (INVERT_LOW_HIGH_BYTES) {  //Useful for Sofar inverters
    //Voltage (370.0)
    PYLON_4210.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
    PYLON_4210.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
    PYLON_4211.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
    PYLON_4211.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);

    if (SET_30K_OFFSET) {
      //Current (15.0)
      PYLON_4210.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
      PYLON_4210.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) >> 8);
      PYLON_4211.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
      PYLON_4211.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) >> 8);
    } else {
      PYLON_4210.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
      PYLON_4210.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
      PYLON_4211.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
      PYLON_4211.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
    }

    // BMS Temperature (We dont have BMS temp, send max cell voltage instead)
    PYLON_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
    PYLON_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
    PYLON_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
    PYLON_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);

    //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
    PYLON_4220.data.u8[0] = (charge_cutoff_voltage_dV & 0x00FF);
    PYLON_4220.data.u8[1] = (charge_cutoff_voltage_dV >> 8);
    PYLON_4221.data.u8[0] = (charge_cutoff_voltage_dV & 0x00FF);
    PYLON_4221.data.u8[1] = (charge_cutoff_voltage_dV >> 8);

    //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
    PYLON_4220.data.u8[2] = (discharge_cutoff_voltage_dV & 0x00FF);
    PYLON_4220.data.u8[3] = (discharge_cutoff_voltage_dV >> 8);
    PYLON_4221.data.u8[2] = (discharge_cutoff_voltage_dV & 0x00FF);
    PYLON_4221.data.u8[3] = (discharge_cutoff_voltage_dV >> 8);

    if (SET_30K_OFFSET) {
      //Max ChargeCurrent
      PYLON_4220.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);
      PYLON_4220.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);
      PYLON_4221.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);
      PYLON_4221.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);

      //Max DischargeCurrent
      PYLON_4220.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
      PYLON_4220.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);
      PYLON_4221.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
      PYLON_4221.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);
    } else {
      //Max ChargeCurrent
      PYLON_4220.data.u8[4] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
      PYLON_4220.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);
      PYLON_4221.data.u8[4] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
      PYLON_4221.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);

      //Max DishargeCurrent
      PYLON_4220.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
      PYLON_4220.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);
      PYLON_4221.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
      PYLON_4221.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);
    }

    //Max cell voltage
    PYLON_4230.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
    PYLON_4230.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
    PYLON_4231.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
    PYLON_4231.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

    //Min cell voltage
    PYLON_4230.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
    PYLON_4230.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
    PYLON_4231.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
    PYLON_4231.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

    //Max temperature per cell
    PYLON_4240.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
    PYLON_4240.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
    PYLON_4241.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
    PYLON_4241.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);

    //Max/Min temperature per cell
    PYLON_4240.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
    PYLON_4240.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
    PYLON_4241.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
    PYLON_4241.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);

    //Max temperature per module
    PYLON_4270.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
    PYLON_4270.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
    PYLON_4271.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
    PYLON_4271.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);

    //Min temperature per module
    PYLON_4270.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
    PYLON_4270.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
    PYLON_4271.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
    PYLON_4271.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
  } else {  // Not INVERT_LOW_HIGH_BYTES
    //Voltage (370.0)
    PYLON_4210.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
    PYLON_4210.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
    PYLON_4211.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
    PYLON_4211.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);

    if (SET_30K_OFFSET) {
      //Current (15.0)
      PYLON_4210.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) >> 8);
      PYLON_4210.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
      PYLON_4211.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) >> 8);
      PYLON_4211.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
    } else {
      PYLON_4210.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
      PYLON_4210.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
      PYLON_4211.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
      PYLON_4211.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
    }

    // BMS Temperature (We dont have BMS temp, send max cell voltage instead)
    PYLON_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
    PYLON_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
    PYLON_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
    PYLON_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);

    //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
    PYLON_4220.data.u8[0] = (charge_cutoff_voltage_dV >> 8);
    PYLON_4220.data.u8[1] = (charge_cutoff_voltage_dV & 0x00FF);
    PYLON_4221.data.u8[0] = (charge_cutoff_voltage_dV >> 8);
    PYLON_4221.data.u8[1] = (charge_cutoff_voltage_dV & 0x00FF);

    //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
    PYLON_4220.data.u8[2] = (discharge_cutoff_voltage_dV >> 8);
    PYLON_4220.data.u8[3] = (discharge_cutoff_voltage_dV & 0x00FF);
    PYLON_4221.data.u8[2] = (discharge_cutoff_voltage_dV >> 8);
    PYLON_4221.data.u8[3] = (discharge_cutoff_voltage_dV & 0x00FF);

    if (SET_30K_OFFSET) {

      //Max ChargeCurrent
      PYLON_4220.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);
      PYLON_4220.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);
      PYLON_4221.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);
      PYLON_4221.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);

      //Max DischargeCurrent
      PYLON_4220.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);
      PYLON_4220.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
      PYLON_4221.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);
      PYLON_4221.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
    } else {
      //Max ChargeCurrent
      PYLON_4220.data.u8[4] = (datalayer.battery.status.max_charge_current_dA >> 8);
      PYLON_4220.data.u8[5] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
      PYLON_4221.data.u8[4] = (datalayer.battery.status.max_charge_current_dA >> 8);
      PYLON_4221.data.u8[5] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

      //Max DishargeCurrent
      PYLON_4220.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA >> 8);
      PYLON_4220.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
      PYLON_4221.data.u8[6] = (datalayer.battery.status.max_discharge_current >> 8);
      PYLON_4221.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
    }

    //Max cell voltage
    PYLON_4230.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
    PYLON_4230.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
    PYLON_4231.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
    PYLON_4231.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);

    //Min cell voltage
    PYLON_4230.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
    PYLON_4230.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
    PYLON_4231.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
    PYLON_4231.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);

    //Max temperature per cell
    PYLON_4240.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
    PYLON_4240.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
    PYLON_4241.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
    PYLON_4241.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);

    //Max/Min temperature per cell
    PYLON_4240.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
    PYLON_4240.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
    PYLON_4241.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
    PYLON_4241.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);

    //Max temperature per module
    PYLON_4270.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
    PYLON_4270.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
    PYLON_4271.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
    PYLON_4271.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);

    //Min temperature per module
    PYLON_4270.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
    PYLON_4270.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
    PYLON_4271.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
    PYLON_4271.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  }

  //In case we run into any errors/faults, we can set charge / discharge forbidden
  if (datalayer.battery.status.bms_status == FAULT) {
    PYLON_4280.data.u8[0] = 0xAA;
    PYLON_4280.data.u8[1] = 0xAA;
    PYLON_4280.data.u8[2] = 0xAA;
    PYLON_4280.data.u8[3] = 0xAA;
    PYLON_4281.data.u8[0] = 0xAA;
    PYLON_4281.data.u8[1] = 0xAA;
    PYLON_4281.data.u8[2] = 0xAA;
    PYLON_4281.data.u8[3] = 0xAA;
  }
}

void PylonInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x4200:  //Message originating from inverter. Depending on which data is required, act accordingly
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] == 0x02) {
        send_setup_info();
      }
      if (rx_frame.data.u8[0] == 0x00) {
        send_system_data();
      }
      break;
    default:
      break;
  }
}

void PylonInverter::transmit_can(unsigned long currentMillis) {
  // No periodic sending, we only react on received can messages
}

void PylonInverter::send_setup_info() {  //Ensemble information
  if (SEND_1) {
    transmit_can_frame(&PYLON_7311, can_config.inverter);
    transmit_can_frame(&PYLON_7321, can_config.inverter);
  } else {
    transmit_can_frame(&PYLON_7310, can_config.inverter);
    transmit_can_frame(&PYLON_7320, can_config.inverter);
  }
}

void PylonInverter::send_system_data() {  //System equipment information
  if (SEND_1) {
    transmit_can_frame(&PYLON_4211, can_config.inverter);
    transmit_can_frame(&PYLON_4221, can_config.inverter);
    transmit_can_frame(&PYLON_4231, can_config.inverter);
    transmit_can_frame(&PYLON_4241, can_config.inverter);
    transmit_can_frame(&PYLON_4251, can_config.inverter);
    transmit_can_frame(&PYLON_4261, can_config.inverter);
    transmit_can_frame(&PYLON_4271, can_config.inverter);
    transmit_can_frame(&PYLON_4281, can_config.inverter);
    transmit_can_frame(&PYLON_4291, can_config.inverter);
  } else {
    transmit_can_frame(&PYLON_4210, can_config.inverter);
    transmit_can_frame(&PYLON_4220, can_config.inverter);
    transmit_can_frame(&PYLON_4230, can_config.inverter);
    transmit_can_frame(&PYLON_4240, can_config.inverter);
    transmit_can_frame(&PYLON_4250, can_config.inverter);
    transmit_can_frame(&PYLON_4260, can_config.inverter);
    transmit_can_frame(&PYLON_4270, can_config.inverter);
    transmit_can_frame(&PYLON_4280, can_config.inverter);
    transmit_can_frame(&PYLON_4290, can_config.inverter);
  }
}

void PylonInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, "Pylontech battery over CAN bus", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
