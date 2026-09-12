#include "FERROAMP-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../inverter/INVERTERS.h"

/*Ferroamp uses a Pylon variant with Inverted HighLow bytes, and 30K offset on some values. We also send batch1 instead of batch0 for this protocol*/

static uint16_t estimate_pylon_lfp_cell_voltage_mV(uint8_t soc_percent) {
  struct SocVoltagePoint {
    uint8_t soc;
    uint16_t voltage_mV;
  };

  //Approximate Pylontech/LFP open-circuit-looking cell voltage curve.
  //Used for non-LFP donor batteries so reported cell min/max voltages stay in a range
  //that Ferroamp expects from a Pylontech/LFP battery.
  static const SocVoltagePoint curve[] = {
      {0, 3000},  {5, 3150},  {10, 3210}, {20, 3260}, {35, 3290},
      {50, 3310}, {65, 3330}, {80, 3360}, {90, 3380}, {100, 3420},
  };

  if (soc_percent <= curve[0].soc) {
    return curve[0].voltage_mV;
  }

  const uint8_t points = sizeof(curve) / sizeof(curve[0]);
  for (uint8_t i = 1; i < points; i++) {
    if (soc_percent <= curve[i].soc) {
      const SocVoltagePoint lower = curve[i - 1];
      const SocVoltagePoint upper = curve[i];
      return lower.voltage_mV +
             ((uint32_t)(soc_percent - lower.soc) * (upper.voltage_mV - lower.voltage_mV)) / (upper.soc - lower.soc);
    }
  }

  return curve[points - 1].voltage_mV;
}

void FerroampCanInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  //There are more mappings that could be added, but this should be enough to use as a starting point

  uint8_t reported_soc_percent = (datalayer.battery.status.reported_soc / 100);  //Remove decimals

  //Ferroamp only supports LFP batteries. We need to fake an LFP voltage range if the battery used is not LFP
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    //Already LFP, pass thru value
    cell_tweaked_max_voltage_mV = datalayer.battery.status.cell_max_voltage_mV;
    cell_tweaked_min_voltage_mV = datalayer.battery.status.cell_min_voltage_mV;
  } else {
    //Non-LFP donor batteries can have a very different voltage/SOC curve than Pylontech LFP.
    //Report a Pylontech/LFP-like cell voltage from SOC instead of the actual donor-cell voltage.
    uint16_t pylon_cell_average_mV = estimate_pylon_lfp_cell_voltage_mV(reported_soc_percent);
    cell_tweaked_max_voltage_mV = pylon_cell_average_mV + PYLON_CELL_SPREAD_mV;
    if (pylon_cell_average_mV > PYLON_CELL_SPREAD_mV) {
      cell_tweaked_min_voltage_mV = pylon_cell_average_mV - PYLON_CELL_SPREAD_mV;
    } else {
      cell_tweaked_min_voltage_mV = pylon_cell_average_mV;
    }
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
  FERROAMP_4211.data.u8[6] = reported_soc_percent;

  //StateOfHealth (100.00%)
  FERROAMP_4211.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  // Status=Bit 0,1,2= 0:Sleep, 1:Charge, 2:Discharge 3:Idle. Bit3 ForceChargeReq. Bit4 Balance charge Request
  FERROAMP_4251.data.u8[1] = 0x00;  // Cycle period, matches observed real Pylontech/ESO15 log

  if (datalayer.system.status.system_status == FAULT) {
    FERROAMP_4251.data.u8[0] = (0x00);  // Sleep
  } else if (datalayer.battery.status.reported_current_dA < -PYLON_IDLE_CURRENT_THRESHOLD_dA) {
    FERROAMP_4251.data.u8[0] = (0x01);  // Charge
  } else if (datalayer.battery.status.reported_current_dA > PYLON_IDLE_CURRENT_THRESHOLD_dA) {
    FERROAMP_4251.data.u8[0] = (0x02);  // Discharge
  } else {
    FERROAMP_4251.data.u8[0] = (0x03);  // Idle
  }

  //Voltage (370.0)
  FERROAMP_4211.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  FERROAMP_4211.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);

  //Current (15.0)
  FERROAMP_4211.data.u8[2] = ((datalayer.battery.status.reported_current_dA + 30000) & 0x00FF);
  FERROAMP_4211.data.u8[3] = ((datalayer.battery.status.reported_current_dA + 30000) >> 8);

  // BMS Temperature (We dont have BMS temp, send max cell temperature instead)
  FERROAMP_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC) & 0x00FF);
  FERROAMP_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC) >> 8);

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
  FERROAMP_4241.data.u8[0] = ((datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC) & 0x00FF);
  FERROAMP_4241.data.u8[1] = ((datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC) >> 8);

  //Min temperature per cell
  FERROAMP_4241.data.u8[2] = ((datalayer.battery.status.temperature_min_dC + TEMPERATURE_OFFSET_dC) & 0x00FF);
  FERROAMP_4241.data.u8[3] = ((datalayer.battery.status.temperature_min_dC + TEMPERATURE_OFFSET_dC) >> 8);

  //Max temperature per module
  FERROAMP_4271.data.u8[0] = ((datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC) & 0x00FF);
  FERROAMP_4271.data.u8[1] = ((datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC) >> 8);

  //Min temperature per module
  FERROAMP_4271.data.u8[2] = ((datalayer.battery.status.temperature_min_dC + TEMPERATURE_OFFSET_dC) & 0x00FF);
  FERROAMP_4271.data.u8[3] = ((datalayer.battery.status.temperature_min_dC + TEMPERATURE_OFFSET_dC) >> 8);

  //Extra Pylontech-like runtime frames 42A1..42E1
  FERROAMP_42A1.data.u8[0] = (TEMPERATURE_OFFSET_dC & 0x00FF);
  FERROAMP_42A1.data.u8[1] = (TEMPERATURE_OFFSET_dC >> 8);
  FERROAMP_42A1.data.u8[2] = (TEMPERATURE_OFFSET_dC & 0x00FF);
  FERROAMP_42A1.data.u8[3] = (TEMPERATURE_OFFSET_dC >> 8);
  FERROAMP_42A1.data.u8[4] = 0;
  FERROAMP_42A1.data.u8[5] = 0;
  FERROAMP_42A1.data.u8[6] = 0;
  FERROAMP_42A1.data.u8[7] = 0;

  //42B1 mirrors pack voltage, signed current without +30000 offset, SOC and SOH
  int16_t ferroamp_signed_current_dA = datalayer.battery.status.reported_current_dA;
  FERROAMP_42B1.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  FERROAMP_42B1.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  FERROAMP_42B1.data.u8[2] = (ferroamp_signed_current_dA & 0x00FF);
  FERROAMP_42B1.data.u8[3] = ((uint16_t)ferroamp_signed_current_dA >> 8);
  FERROAMP_42B1.data.u8[4] = 0;
  FERROAMP_42B1.data.u8[5] = 0;
  FERROAMP_42B1.data.u8[6] = FERROAMP_4211.data.u8[6];
  FERROAMP_42B1.data.u8[7] = FERROAMP_4211.data.u8[7];

  //42C1 mirrors BMS temperature and voltage limits
  uint16_t bms_temperature_offset_dC = datalayer.battery.status.temperature_max_dC + TEMPERATURE_OFFSET_dC;
  FERROAMP_42C1.data.u8[0] = (bms_temperature_offset_dC & 0x00FF);
  FERROAMP_42C1.data.u8[1] = (bms_temperature_offset_dC >> 8);
  FERROAMP_42C1.data.u8[2] = 0;
  FERROAMP_42C1.data.u8[3] = 0;
  FERROAMP_42C1.data.u8[4] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  FERROAMP_42C1.data.u8[5] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  FERROAMP_42C1.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  FERROAMP_42C1.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  //42D1 current limits, signed and without +30000 offset.
  int16_t ferroamp_max_charge_current_dA = datalayer.battery.status.max_charge_current_dA;
  int16_t ferroamp_max_discharge_current_signed_dA = -datalayer.battery.status.max_discharge_current_dA;
  FERROAMP_42D1.data.u8[0] = (ferroamp_max_charge_current_dA & 0x00FF);
  FERROAMP_42D1.data.u8[1] = ((uint16_t)ferroamp_max_charge_current_dA >> 8);
  FERROAMP_42D1.data.u8[2] = 0;
  FERROAMP_42D1.data.u8[3] = 0;
  FERROAMP_42D1.data.u8[4] = (ferroamp_max_discharge_current_signed_dA & 0x00FF);
  FERROAMP_42D1.data.u8[5] = ((uint16_t)ferroamp_max_discharge_current_signed_dA >> 8);
  FERROAMP_42D1.data.u8[6] = 0xFF;
  FERROAMP_42D1.data.u8[7] = 0xFF;

  //42E1 module voltage summary in centivolts, using the same module count as 0x7321
  uint8_t modules_in_series = FERROAMP_7321.data.u8[2];
  if (modules_in_series == 0) {
    modules_in_series = MODULES_IN_SERIES;
  }

  uint16_t module_voltage_cV = (uint16_t)(((uint32_t)datalayer.battery.status.voltage_dV * 10U) / modules_in_series);
  FERROAMP_42E1.data.u8[0] = (module_voltage_cV & 0x00FF);
  FERROAMP_42E1.data.u8[1] = (module_voltage_cV >> 8);
  FERROAMP_42E1.data.u8[2] = (module_voltage_cV & 0x00FF);
  FERROAMP_42E1.data.u8[3] = (module_voltage_cV >> 8);
  FERROAMP_42E1.data.u8[4] = 0;
  FERROAMP_42E1.data.u8[5] = 0;
  FERROAMP_42E1.data.u8[6] = modules_in_series - 1;
  FERROAMP_42E1.data.u8[7] = 0;
  FERROAMP_4261.data.u8[6] = modules_in_series - 1;

  //4281 follows the observed real Pylontech/ESO15 log:
  //byte0 charge forbidden, byte1 discharge forbidden, byte2 heartbeat, byte3 SOE/SOC, byte4-7 zero.
  if (datalayer.system.status.system_status == FAULT) {
    FERROAMP_4281.data.u8[0] = 0xAA;
    FERROAMP_4281.data.u8[1] = 0xAA;
  } else {
    FERROAMP_4281.data.u8[0] = 0x00;
    FERROAMP_4281.data.u8[1] = 0x00;
  }
  FERROAMP_4281.data.u8[3] = reported_soc_percent;
  FERROAMP_4281.data.u8[4] = 0x00;
  FERROAMP_4281.data.u8[5] = 0x00;
  FERROAMP_4281.data.u8[6] = 0x00;
  FERROAMP_4281.data.u8[7] = 0x00;
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

    case 0x8201:  //Sleep/Awake command from ESO. Pylontech does not reply to this frame.
    case 0x8211:  //Charge/discharge command from ESO. Pylontech does not reply to this frame.
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;

    case 0x8221:  //Force open / allow closing main relay command. Reply on 0x8231.
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      FERROAMP_8231.data.u8[0] = rx_frame.data.u8[0];
      FERROAMP_8231.data.u8[1] = 0x00;
      FERROAMP_8231.data.u8[2] = 0x00;
      FERROAMP_8231.data.u8[3] = 0x00;
      FERROAMP_8231.data.u8[4] = 0x00;
      FERROAMP_8231.data.u8[5] = 0x00;
      FERROAMP_8231.data.u8[6] = 0x00;
      FERROAMP_8231.data.u8[7] = 0x00;
      transmit_can_frame(&FERROAMP_8231);
      break;

    case 0x8241:  //Mask external communication error command. Reply on 0x8251 if ESO asks for it.
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      FERROAMP_8251.data.u8[0] = rx_frame.data.u8[0];
      transmit_can_frame(&FERROAMP_8251);
      break;

    case 0x8261:  //System operation / run command. Reply on 0x8271 if ESO asks for it.
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      FERROAMP_8271.data.u8[0] = rx_frame.data.u8[0];
      transmit_can_frame(&FERROAMP_8271);
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
  transmit_can_frame(&FERROAMP_7331);
  transmit_can_frame(&FERROAMP_7341);
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
  FERROAMP_4281.data.u8[2] = pylon_heartbeat++;
  transmit_can_frame(&FERROAMP_4281);
  transmit_can_frame(&FERROAMP_4291);
  transmit_can_frame(&FERROAMP_42A1);
  transmit_can_frame(&FERROAMP_42B1);
  transmit_can_frame(&FERROAMP_42C1);
  transmit_can_frame(&FERROAMP_42D1);
  transmit_can_frame(&FERROAMP_42E1);
}
