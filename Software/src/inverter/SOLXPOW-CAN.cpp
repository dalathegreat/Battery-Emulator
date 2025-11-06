#include "SOLXPOW-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../inverter/INVERTERS.h"

#define INVERT_LOW_HIGH_BYTES  //If defined, certain frames will have inverted low/high bytes
//#define SET_30K_OFFSET  //If defined, current values are sent with a 30k offest (useful for ferroamp)

void SolxpowInverter::
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

  //Charge / Discharge allowed flags
  if (datalayer.battery.status.max_charge_current_dA == 0) {
    SOLXPOW_4280.data.u8[0] = 0xAA;  //Charge forbidden
  } else {
    SOLXPOW_4280.data.u8[0] = 0;  //Charge allowed
  }

  if (datalayer.battery.status.max_discharge_current_dA == 0) {
    SOLXPOW_4280.data.u8[1] = 0xAA;  //Discharge forbidden
  } else {
    SOLXPOW_4280.data.u8[1] = 0;  //Discharge allowed
  }

  //In case run into a FAULT state, let inverter know to stop any charge/discharge
  if (datalayer.battery.status.bms_status == FAULT) {
    SOLXPOW_4280.data.u8[0] = 0xAA;  //Charge forbidden
    SOLXPOW_4280.data.u8[1] = 0xAA;  //Discharge forbidden
  }

  //Voltage (370.0)
  SOLXPOW_4210.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  SOLXPOW_4210.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);

  //Current (15.0)
  SOLXPOW_4210.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  SOLXPOW_4210.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);

  // BMS Temperature (We dont have BMS temp, send max cell voltage instead)
#ifdef INVERT_LOW_HIGH_BYTES  //Useful for Sofar inverters
  SOLXPOW_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  SOLXPOW_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
#else   // Not INVERT_LOW_HIGH_BYTES
  SOLXPOW_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  SOLXPOW_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
#endif  // INVERT_LOW_HIGH_BYTES
  //SOC (100.00%)
  SOLXPOW_4210.data.u8[6] = (datalayer.battery.status.reported_soc / 100);  //Remove decimals

  //StateOfHealth (100.00%)
  SOLXPOW_4210.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  // Status=Bit 0,1,2= 0:Sleep, 1:Charge, 2:Discharge 3:Idle. Bit3 ForceChargeReq. Bit4 Balance charge Request
  if (datalayer.battery.status.bms_status == FAULT) {
    SOLXPOW_4250.data.u8[0] = (0x00);  // Sleep
  } else if (datalayer.battery.status.current_dA < 0) {
    SOLXPOW_4250.data.u8[0] = (0x01);  // Charge
  } else if (datalayer.battery.status.current_dA > 0) {
    SOLXPOW_4250.data.u8[0] = (0x02);  // Discharge
  } else if (datalayer.battery.status.current_dA == 0) {
    SOLXPOW_4250.data.u8[0] = (0x03);  // Idle
  }

#ifdef INVERT_LOW_HIGH_BYTES  //Useful for Sofar inverters
  //Voltage (370.0)
  SOLXPOW_4210.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SOLXPOW_4210.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);

#ifdef SET_30K_OFFSET
  //Current (15.0)
  SOLXPOW_4210.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
  SOLXPOW_4210.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) >> 8);
#else   // Not SET_30K_OFFSET
  SOLXPOW_4210.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  SOLXPOW_4210.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
#endif  //SET_30K_OFFSET

  // BMS Temperature (We dont have BMS temp, send max cell voltage instead)
  SOLXPOW_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  SOLXPOW_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
  SOLXPOW_4220.data.u8[0] = (charge_cutoff_voltage_dV & 0x00FF);
  SOLXPOW_4220.data.u8[1] = (charge_cutoff_voltage_dV >> 8);

  //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
  SOLXPOW_4220.data.u8[2] = (discharge_cutoff_voltage_dV & 0x00FF);
  SOLXPOW_4220.data.u8[3] = (discharge_cutoff_voltage_dV >> 8);

#ifdef SET_30K_OFFSET
  //Max ChargeCurrent
  SOLXPOW_4220.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);
  SOLXPOW_4220.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);

  //Max DischargeCurrent
  SOLXPOW_4220.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
  SOLXPOW_4220.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);
#else   // Not SET_30K_OFFSET
  //Max ChargeCurrent
  SOLXPOW_4220.data.u8[4] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  SOLXPOW_4220.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);

  //Max DishargeCurrent
  SOLXPOW_4220.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  SOLXPOW_4220.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);
#endif  // SET_30K_OFFSET

  //Max cell voltage
  SOLXPOW_4230.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SOLXPOW_4230.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

  //Min cell voltage
  SOLXPOW_4230.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SOLXPOW_4230.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //Max temperature per cell
  SOLXPOW_4240.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SOLXPOW_4240.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Max/Min temperature per cell
  SOLXPOW_4240.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  SOLXPOW_4240.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);

  //Max temperature per module
  SOLXPOW_4270.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SOLXPOW_4270.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Min temperature per module
  SOLXPOW_4270.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  SOLXPOW_4270.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
#else  // Not INVERT_LOW_HIGH_BYTES
  //Voltage (370.0)
  SOLXPOW_4210.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  SOLXPOW_4210.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);

#ifdef SET_30K_OFFSET
  //Current (15.0)
  SOLXPOW_4210.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) >> 8);
  SOLXPOW_4210.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
#else   // Not SET_30K_OFFSET
  SOLXPOW_4210.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  SOLXPOW_4210.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
#endif  //SET_30K_OFFSET

  // BMS Temperature (We dont have BMS temp, send max cell voltage instead)
  SOLXPOW_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  SOLXPOW_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
  SOLXPOW_4220.data.u8[0] = (charge_cutoff_voltage_dV >> 8);
  SOLXPOW_4220.data.u8[1] = (charge_cutoff_voltage_dV & 0x00FF);

  //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
  SOLXPOW_4220.data.u8[2] = (discharge_cutoff_voltage_dV >> 8);
  SOLXPOW_4220.data.u8[3] = (discharge_cutoff_voltage_dV & 0x00FF);

#ifdef SET_30K_OFFSET
  //Max ChargeCurrent
  SOLXPOW_4220.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);
  SOLXPOW_4220.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);

  //Max DischargeCurrent
  SOLXPOW_4220.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);
  SOLXPOW_4220.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
#else   // Not SET_30K_OFFSET
  //Max ChargeCurrent
  SOLXPOW_4220.data.u8[4] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SOLXPOW_4220.data.u8[5] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

  //Max DishargeCurrent
  SOLXPOW_4220.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  SOLXPOW_4220.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
#endif  //SET_30K_OFFSET

  //Max cell voltage
  SOLXPOW_4230.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  SOLXPOW_4230.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);

  //Min cell voltage
  SOLXPOW_4230.data.u8[2] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  SOLXPOW_4230.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);

  //Max temperature per cell
  SOLXPOW_4240.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  SOLXPOW_4240.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);

  //Max/Min temperature per cell
  SOLXPOW_4240.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  SOLXPOW_4240.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);

  //Max temperature per module
  SOLXPOW_4270.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  SOLXPOW_4270.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);

  //Min temperature per module
  SOLXPOW_4270.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  SOLXPOW_4270.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
#endif  // Not INVERT_LOW_HIGH_BYTES
}

void SolxpowInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
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

void SolxpowInverter::transmit_can(unsigned long currentMillis) {
  // No periodic sending, we only react on received can messages
}

void SolxpowInverter::send_setup_info() {  //Ensemble information
  transmit_can_frame(&SOLXPOW_7310);
  transmit_can_frame(&SOLXPOW_7320);
  transmit_can_frame(&SOLXPOW_7330);
  transmit_can_frame(&SOLXPOW_7340);
}

void SolxpowInverter::send_system_data() {  //System equipment information
  transmit_can_frame(&SOLXPOW_4210);
  transmit_can_frame(&SOLXPOW_4220);
  transmit_can_frame(&SOLXPOW_4230);
  transmit_can_frame(&SOLXPOW_4240);
  transmit_can_frame(&SOLXPOW_4250);
  transmit_can_frame(&SOLXPOW_4260);
  transmit_can_frame(&SOLXPOW_4270);
  transmit_can_frame(&SOLXPOW_4280);
  transmit_can_frame(&SOLXPOW_4290);
}

bool SolxpowInverter::setup() {
  if (user_selected_inverter_cells > 0) {
    SOLXPOW_7320.data.u8[0] = user_selected_inverter_cells & 0xff;
    SOLXPOW_7320.data.u8[1] = (uint8_t)(user_selected_inverter_cells >> 8);
  }

  if (user_selected_inverter_modules > 0) {
    SOLXPOW_7320.data.u8[2] = user_selected_inverter_modules;
  }

  if (user_selected_inverter_cells_per_module > 0) {
    SOLXPOW_7320.data.u8[3] = user_selected_inverter_cells_per_module;
  }

  if (user_selected_inverter_voltage_level > 0) {
    SOLXPOW_7320.data.u8[4] = user_selected_inverter_voltage_level & 0xff;
    SOLXPOW_7320.data.u8[5] = (uint8_t)(user_selected_inverter_voltage_level >> 8);
  }

  if (user_selected_inverter_ah_capacity > 0) {
    SOLXPOW_7320.data.u8[6] = user_selected_inverter_ah_capacity & 0xff;
    SOLXPOW_7320.data.u8[7] = (uint8_t)(user_selected_inverter_ah_capacity >> 8);
  }

  return true;
}
