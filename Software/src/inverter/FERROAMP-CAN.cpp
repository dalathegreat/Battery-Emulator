#include "FERROAMP-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../inverter/INVERTERS.h"

/*Ferroamp uses a Pylon variant with Inverted HighLow bytes, and 30K offset on some values. We also send batch1 instead of batch0 for this protocol*/

void FerroampCanInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  //There are more mappings that could be added, but this should be enough to use as a starting point

  //Ferroamp only supports LFP batteries. We need to fake an LFP voltage range if the battery used is not LFP
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    //Already LFP, pass thru value
    cell_tweaked_max_voltage_mV = datalayer.battery.status.cell_max_voltage_mV;
    cell_tweaked_min_voltage_mV = datalayer.battery.status.cell_min_voltage_mV;
  } else {  //linear interpolation to remap the value from the range [2500-4200] to [2500-3400]
    cell_tweaked_max_voltage_mV =
        (2500 + ((datalayer.battery.status.cell_max_voltage_mV - 2500) * (3400 - 2500)) / (4200 - 2500));
    cell_tweaked_min_voltage_mV =
        (2500 + ((datalayer.battery.status.cell_min_voltage_mV - 2500) * (3400 - 2500)) / (4200 - 2500));
  }

  //Incase user has tweaked capacity of batteries in the Webserver, map this to the CAN messages
  /*  CAN_frame FERROAMP_7321 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7321,
                          .data = {(TOTAL_CELL_AMOUNT & 0xFF), (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES,
                                   CELLS_PER_MODULE, (uint8_t)(VOLTAGE_LEVEL & 0x00FF), (uint8_t)(VOLTAGE_LEVEL >> 8),
                                   (uint8_t)(AH_CAPACITY & 0x00FF), (uint8_t)(AH_CAPACITY >> 8)}};*/
  if (user_selected_inverter_cells > 0) {
    FERROAMP_7321.data.u8[0] = user_selected_inverter_cells & 0xff;
    FERROAMP_7321.data.u8[1] = (uint8_t)(user_selected_inverter_cells >> 8);
  }
  if (user_selected_inverter_modules > 0) {
    FERROAMP_7321.data.u8[2] = user_selected_inverter_modules;
  }
  if (user_selected_inverter_cells_per_module > 0) {
    FERROAMP_7321.data.u8[3] = user_selected_inverter_cells_per_module;
  }
  if (user_selected_inverter_voltage_level > 0) {
    FERROAMP_7321.data.u8[4] = user_selected_inverter_voltage_level & 0xff;
    FERROAMP_7321.data.u8[5] = (uint8_t)(user_selected_inverter_voltage_level >> 8);
  }
  if (user_selected_inverter_ah_capacity > 0) {
    FERROAMP_7321.data.u8[6] = user_selected_inverter_ah_capacity & 0xff;
    FERROAMP_7321.data.u8[7] = (uint8_t)(user_selected_inverter_ah_capacity >> 8);
  }

  //SOC (100.00%)
  FERROAMP_4211.data.u8[6] = (datalayer.battery.status.reported_soc / 100);  //Remove decimals

  //StateOfHealth (100.00%)
  FERROAMP_4211.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  // Status=Bit 0,1,2= 0:Sleep, 1:Charge, 2:Discharge 3:Idle. Bit3 ForceChargeReq. Bit4 Balance charge Request
  if (datalayer.battery.status.bms_status == FAULT) {
    FERROAMP_4251.data.u8[0] = (0x00);  // Sleep
  } else if (datalayer.battery.status.current_dA < 0) {
    FERROAMP_4251.data.u8[0] = (0x01);  // Charge
  } else if (datalayer.battery.status.current_dA > 0) {
    FERROAMP_4251.data.u8[0] = (0x02);  // Discharge
  } else if (datalayer.battery.status.current_dA == 0) {
    FERROAMP_4251.data.u8[0] = (0x03);  // Idle
  }

  //Voltage (370.0)
  FERROAMP_4211.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  FERROAMP_4211.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);

  //Current (15.0)
  FERROAMP_4211.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
  FERROAMP_4211.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) >> 8);

  // BMS Temperature (We dont have BMS temp, send max cell voltage instead)
  FERROAMP_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  FERROAMP_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Discharge Cutoff Voltage
  FERROAMP_4221.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  FERROAMP_4221.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);

  //Minvoltage (eg 300.0V = 3000 , 16bits long) Charge Cutoff Voltage
  FERROAMP_4221.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  FERROAMP_4221.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  //Max ChargeCurrent
  FERROAMP_4221.data.u8[4] = ((datalayer.battery.status.max_charge_current_dA + 30000) & 0x00FF);
  FERROAMP_4221.data.u8[5] = ((datalayer.battery.status.max_charge_current_dA + 30000) >> 8);

  //Max DischargeCurrent
  FERROAMP_4221.data.u8[6] = ((30000 - datalayer.battery.status.max_discharge_current_dA) & 0x00FF);
  FERROAMP_4221.data.u8[7] = ((30000 - datalayer.battery.status.max_discharge_current_dA) >> 8);

  //Max cell voltage
  FERROAMP_4231.data.u8[0] = (cell_tweaked_max_voltage_mV & 0x00FF);
  FERROAMP_4231.data.u8[1] = (cell_tweaked_max_voltage_mV >> 8);

  //Min cell voltage
  FERROAMP_4231.data.u8[2] = (cell_tweaked_min_voltage_mV & 0x00FF);
  FERROAMP_4231.data.u8[3] = (cell_tweaked_min_voltage_mV >> 8);

  //Max temperature per cell
  FERROAMP_4241.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  FERROAMP_4241.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Min temperature per cell
  FERROAMP_4241.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  FERROAMP_4241.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);

  //Max temperature per module
  FERROAMP_4271.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  FERROAMP_4271.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Min temperature per module
  FERROAMP_4271.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  FERROAMP_4271.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);

  //In case we run into any errors/faults, we can set charge / discharge forbidden
  if (datalayer.battery.status.bms_status == FAULT) {
    FERROAMP_4281.data.u8[0] = 0xAA;
    FERROAMP_4281.data.u8[1] = 0xAA;
    FERROAMP_4281.data.u8[2] = 0xAA;
    FERROAMP_4281.data.u8[3] = 0xAA;
  } else {
    //Charge / Discharge allowed
    FERROAMP_4281.data.u8[0] = 0;
    FERROAMP_4281.data.u8[1] = 0;
    FERROAMP_4281.data.u8[2] = 0;
    FERROAMP_4281.data.u8[3] = 0;
  }
}

void FerroampCanInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
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

void FerroampCanInverter::transmit_can(unsigned long currentMillis) {
  // No periodic sending, we only react on received can messages
}

void FerroampCanInverter::send_setup_info() {  //Ensemble information
  //Ferroamp protocol sends Pylon 7311 instead of 7310 etc. Send1 in Pylon lingo
  transmit_can_frame(&FERROAMP_7311);
  transmit_can_frame(&FERROAMP_7321);
}

void FerroampCanInverter::send_system_data() {  //System equipment information
  //Ferroamp protocol sends Pylon 4211 instead of 4210 etc. Send1 in Pylon lingo
  transmit_can_frame(&FERROAMP_4211);
  transmit_can_frame(&FERROAMP_4221);
  transmit_can_frame(&FERROAMP_4231);
  transmit_can_frame(&FERROAMP_4241);
  transmit_can_frame(&FERROAMP_4251);
  transmit_can_frame(&FERROAMP_4261);
  transmit_can_frame(&FERROAMP_4271);
  transmit_can_frame(&FERROAMP_4281);
  transmit_can_frame(&FERROAMP_4291);
}
