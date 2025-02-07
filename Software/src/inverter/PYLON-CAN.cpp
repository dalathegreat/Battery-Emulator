#include "../include.h"
#ifdef PYLON_CAN
#include "../datalayer/datalayer.h"
#include "PYLON-CAN.h"

#define SEND_0  //If defined, the messages will have ID ending with 0 (useful for some inverters)
//#define SEND_1 //If defined, the messages will have ID ending with 1 (useful for some inverters)
#define INVERT_LOW_HIGH_BYTES  //If defined, certain frames will have inverted low/high bytes \
                               //useful for some inverters like Sofar that report the voltages incorrect otherwise
//#define SET_30K_OFFSET  //If defined, current values are sent with a 30k offest (useful for ferroamp)

/* Some inverters need to see a specific amount of cells/modules to emulate a specific Pylon battery.
Change the following only if your inverter is generating fault codes about voltage range */
#define TOTAL_CELL_AMOUNT 120
#define MODULES_IN_SERIES 4
#define CELLS_PER_MODULE 30
#define VOLTAGE_LEVEL 384
#define AH_CAPACITY 37

/* Do not change code below unless you are sure what you are doing */
//Actual content messages
CAN_frame PYLON_7310 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x7310,
                        .data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
CAN_frame PYLON_7311 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x7311,
                        .data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
CAN_frame PYLON_7320 = {
    .FD = false,
    .ext_ID = true,
    .DLC = 8,
    .ID = 0x7320,
    .data = {TOTAL_CELL_AMOUNT, (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES, CELLS_PER_MODULE, VOLTAGE_LEVEL,
             (uint8_t)(VOLTAGE_LEVEL >> 8), AH_CAPACITY, (uint8_t)(AH_CAPACITY >> 8)}};
CAN_frame PYLON_7321 = {
    .FD = false,
    .ext_ID = true,
    .DLC = 8,
    .ID = 0x7321,
    .data = {TOTAL_CELL_AMOUNT, (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES, CELLS_PER_MODULE, VOLTAGE_LEVEL,
             (uint8_t)(VOLTAGE_LEVEL >> 8), AH_CAPACITY, (uint8_t)(AH_CAPACITY >> 8)}};
CAN_frame PYLON_4210 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4210,
                        .data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
CAN_frame PYLON_4220 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4220,
                        .data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
CAN_frame PYLON_4230 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4230,
                        .data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
CAN_frame PYLON_4240 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4240,
                        .data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
CAN_frame PYLON_4250 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4250,
                        .data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_4260 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4260,
                        .data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
CAN_frame PYLON_4270 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4270,
                        .data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
CAN_frame PYLON_4280 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4280,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_4290 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4290,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_4211 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4211,
                        .data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
CAN_frame PYLON_4221 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4221,
                        .data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
CAN_frame PYLON_4231 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4231,
                        .data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
CAN_frame PYLON_4241 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4241,
                        .data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
CAN_frame PYLON_4251 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4251,
                        .data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_4261 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4261,
                        .data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
CAN_frame PYLON_4271 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4271,
                        .data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
CAN_frame PYLON_4281 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4281,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_4291 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x4291,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static uint16_t discharge_cutoff_voltage_dV = 0;
static uint16_t charge_cutoff_voltage_dV = 0;
#define VOLTAGE_OFFSET_DV 20  // Small offset voltage to avoid generating voltage events

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

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
#ifdef INVERT_LOW_HIGH_BYTES  //Useful for Sofar inverters
  PYLON_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  PYLON_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  PYLON_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  PYLON_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
#else   // Not INVERT_LOW_HIGH_BYTES
  PYLON_4210.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  PYLON_4210.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
  PYLON_4211.data.u8[4] = ((datalayer.battery.status.temperature_max_dC + 1000) >> 8);
  PYLON_4211.data.u8[5] = ((datalayer.battery.status.temperature_max_dC + 1000) & 0x00FF);
#endif  // INVERT_LOW_HIGH_BYTES
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

#ifdef INVERT_LOW_HIGH_BYTES  //Useful for Sofar inverters
  //Voltage (370.0)
  PYLON_4210.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  PYLON_4210.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  PYLON_4211.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  PYLON_4211.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);

#ifdef SET_30K_OFFSET
  //Current (15.0)
  PYLON_4210.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
  PYLON_4210.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) >> 8);
  PYLON_4211.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
  PYLON_4211.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) >> 8);
#else   // Not SET_30K_OFFSET
  PYLON_4210.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  PYLON_4210.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  PYLON_4211.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  PYLON_4211.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
#endif  //SET_30K_OFFSET

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

#ifdef SET_30K_OFFSET
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
#else   // Not SET_30K_OFFSET
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
#endif  // SET_30K_OFFSET

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
#else  // Not INVERT_LOW_HIGH_BYTES
  //Voltage (370.0)
  PYLON_4210.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  PYLON_4210.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  PYLON_4211.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  PYLON_4211.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);

#ifdef SET_30K_OFFSET
  //Current (15.0)
  PYLON_4210.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) >> 8);
  PYLON_4210.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
  PYLON_4211.data.u8[2] = ((datalayer.battery.status.current_dA + 30000) >> 8);
  PYLON_4211.data.u8[3] = ((datalayer.battery.status.current_dA + 30000) & 0x00FF);
#else   // Not SET_30K_OFFSET
  PYLON_4210.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  PYLON_4210.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  PYLON_4211.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  PYLON_4211.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
#endif  //SET_30K_OFFSET

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

#ifdef SET_30K_OFFSET
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
#else   // Not SET_30K_OFFSET
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
#endif  //SET_30K_OFFSET

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
#endif  // Not INVERT_LOW_HIGH_BYTES

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

void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {
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

void transmit_can_inverter() {
  // No periodic sending, we only react on received can messages
}

void send_setup_info() {  //Ensemble information
#ifdef SEND_0
  transmit_can_frame(&PYLON_7310, can_config.inverter);
  transmit_can_frame(&PYLON_7320, can_config.inverter);
#endif
#ifdef SEND_1
  transmit_can_frame(&PYLON_7311, can_config.inverter);
  transmit_can_frame(&PYLON_7321, can_config.inverter);
#endif
}

void send_system_data() {  //System equipment information
#ifdef SEND_0
  transmit_can_frame(&PYLON_4210, can_config.inverter);
  transmit_can_frame(&PYLON_4220, can_config.inverter);
  transmit_can_frame(&PYLON_4230, can_config.inverter);
  transmit_can_frame(&PYLON_4240, can_config.inverter);
  transmit_can_frame(&PYLON_4250, can_config.inverter);
  transmit_can_frame(&PYLON_4260, can_config.inverter);
  transmit_can_frame(&PYLON_4270, can_config.inverter);
  transmit_can_frame(&PYLON_4280, can_config.inverter);
  transmit_can_frame(&PYLON_4290, can_config.inverter);
#endif
#ifdef SEND_1
  transmit_can_frame(&PYLON_4211, can_config.inverter);
  transmit_can_frame(&PYLON_4221, can_config.inverter);
  transmit_can_frame(&PYLON_4231, can_config.inverter);
  transmit_can_frame(&PYLON_4241, can_config.inverter);
  transmit_can_frame(&PYLON_4251, can_config.inverter);
  transmit_can_frame(&PYLON_4261, can_config.inverter);
  transmit_can_frame(&PYLON_4271, can_config.inverter);
  transmit_can_frame(&PYLON_4281, can_config.inverter);
  transmit_can_frame(&PYLON_4291, can_config.inverter);
#endif
}
void setup_inverter(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, "Pylontech battery over CAN bus", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
#endif
