#include "../include.h"
#ifdef SUNGROW_CAN
#include "../datalayer/datalayer.h"
#include "SUNGROW-CAN.h"

#include "../communication/can/comm_can.h"

/* TODO: 
This protocol is still under development. It can not be used yet for Sungrow inverters, 
see the Wiki for more info on how to use your Sungrow inverter */

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis500ms = 0;
static bool alternate = false;
static uint8_t mux = 0;
static uint8_t version_char[14] = {0};
static uint8_t manufacturer_char[14] = {0};
static uint8_t model_char[14] = {0};
static bool inverter_sends_000 = false;

//Actual content messages
CAN_frame SUNGROW_000 = {.FD = false,  // Sent by inv or BMS?
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x000,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_001 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x001,
                         .data = {0xF0, 0x05, 0x20, 0x03, 0x2C, 0x01, 0x2C, 0x01}};
CAN_frame SUNGROW_002 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x002,
                         .data = {0xA2, 0x05, 0x10, 0x27, 0x9B, 0x03, 0x00, 0x19}};
CAN_frame SUNGROW_003 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x003,
                         .data = {0x2A, 0x1D, 0x00, 0x00, 0xBF, 0x18, 0x00, 0x00}};
CAN_frame SUNGROW_004 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x004,
                         .data = {0x27, 0x05, 0x00, 0x00, 0x24, 0x05, 0x08, 0x01}};
CAN_frame SUNGROW_005 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x005,
                         .data = {0x02, 0x00, 0x01, 0xE6, 0x20, 0x24, 0x05, 0x00}};
CAN_frame SUNGROW_006 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x006,
                         .data = {0x0E, 0x01, 0x01, 0x01, 0xDE, 0x0C, 0xD5, 0x0C}};
CAN_frame SUNGROW_013 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x013,
                         .data = {0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x0E, 0x01}};
CAN_frame SUNGROW_014 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x014,
                         .data = {0x05, 0x01, 0xAC, 0x80, 0x10, 0x02, 0x57, 0x80}};
CAN_frame SUNGROW_015 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x015,
                         .data = {0x93, 0x80, 0xAC, 0x80, 0x57, 0x80, 0x93, 0x80}};
CAN_frame SUNGROW_016 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x016,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_017 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x017,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_018 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x018,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_019 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x019,
                         .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_01A = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x01A,
                         .data = {0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_01B = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x01B,
                         .data = {0xBE, 0x8F, 0x61, 0x01, 0xBE, 0x8F, 0x61, 0x01}};
CAN_frame SUNGROW_01C = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x01C,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_01D = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x01D,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_01E = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x01E,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_400 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x400,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_500 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x500,
                         .data = {0x01, 0x01, 0x00, 0xFF, 0x00, 0x01, 0x00, 0x32}};
CAN_frame SUNGROW_501 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x501,
                         .data = {0xF0, 0x05, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_502 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x502,
                         .data = {0xA2, 0x05, 0x00, 0x00, 0x9B, 0x03, 0x00, 0x19}};
CAN_frame SUNGROW_503 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x503,
                         .data = {0x2A, 0x1D, 0x00, 0x00, 0xBF, 0x18, 0x00, 0x00}};
CAN_frame SUNGROW_504 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x504,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_505 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x505,
                         .data = {0x00, 0x02, 0x01, 0xE6, 0x20, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_506 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x506,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_512 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x512,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_700 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x700,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_701 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x701,
                         .data = {0xF0, 0x05, 0x20, 0x03, 0x2C, 0x01, 0x2C, 0x01}};
CAN_frame SUNGROW_702 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x702,
                         .data = {0xA2, 0x05, 0x10, 0x27, 0x9B, 0x03, 0x00, 0x19}};
CAN_frame SUNGROW_703 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x703,
                         .data = {0x2A, 0x1D, 0x00, 0x00, 0xBF, 0x18, 0x00, 0x00}};
CAN_frame SUNGROW_704 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x704,
                         .data = {0x27, 0x05, 0x00, 0x00, 0x24, 0x05, 0x08, 0x01}};
CAN_frame SUNGROW_705 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x705,
                         .data = {0x02, 0x00, 0x01, 0xE6, 0x20, 0x24, 0x05, 0x00}};
CAN_frame SUNGROW_706 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x706,
                         .data = {0x0E, 0x01, 0x01, 0x01, 0xDE, 0x0C, 0xD5, 0x0C}};
CAN_frame SUNGROW_713 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x713,
                         .data = {0x01, 0x01, 0x01, 0x02, 0x01, 0x02, 0x0E, 0x01}};
CAN_frame SUNGROW_714 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x714,
                         .data = {0x05, 0x01, 0xAC, 0x80, 0x10, 0x02, 0x57, 0x80}};
CAN_frame SUNGROW_715 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x715,
                         .data = {0x93, 0x80, 0xAC, 0x80, 0x57, 0x80, 0x93, 0x80}};
CAN_frame SUNGROW_716 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x716,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_717 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x717,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_718 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x718,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_719 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x719,
                         .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71A = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71A,
                         .data = {0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71B = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71B,
                         .data = {0xBE, 0x8F, 0x61, 0x01, 0xBE, 0x8F, 0x61, 0x01}};
CAN_frame SUNGROW_71C = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71C,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71D = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71D,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SUNGROW_71E = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x71E,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void SungrowCanInverter::
    update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the inverter CAN messages

  //Maxvoltage (eg 400.0V = 4000 , 16bits long)
  SUNGROW_701.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  SUNGROW_701.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  //Minvoltage (eg 300.0V = 3000 , 16bits long)
  SUNGROW_701.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  SUNGROW_701.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  //Vcharge request (Maxvoltage-X)
  SUNGROW_702.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV - 20) & 0x00FF);
  SUNGROW_702.data.u8[1] = ((datalayer.battery.info.max_design_voltage_dV - 20) >> 8);
  //SOH (100.00%)
  SUNGROW_702.data.u8[2] = (datalayer.battery.status.soh_pptt & 0x00FF);
  SUNGROW_702.data.u8[3] = (datalayer.battery.status.soh_pptt >> 8);
  //SOC (100.0%)
  SUNGROW_702.data.u8[4] = ((datalayer.battery.status.reported_soc / 10) & 0x00FF);
  SUNGROW_702.data.u8[5] = ((datalayer.battery.status.reported_soc / 10) >> 8);
  //Capacity max (Wh) TODO: Will overflow if larger than 32kWh
  SUNGROW_702.data.u8[6] = (datalayer.battery.info.total_capacity_Wh & 0x00FF);
  SUNGROW_702.data.u8[7] = (datalayer.battery.info.total_capacity_Wh >> 8);

  // Energy total charged (Wh)
  //SUNGROW_703.data.u8[0] =
  //SUNGROW_703.data.u8[1] =
  //SUNGROW_703.data.u8[2] =
  //SUNGROW_703.data.u8[3] =
  // Energy total discharged (Wh)
  //SUNGROW_703.data.u8[4] =
  //SUNGROW_703.data.u8[5] =
  //SUNGROW_703.data.u8[6] =
  //SUNGROW_703.data.u8[7] =

  //Vbat (eg 400.0V = 4000 , 16bits long)
  SUNGROW_704.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_704.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  //Temperature //TODO: Signed correctly? Also should be put AVG here?
  SUNGROW_704.data.u8[6] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_704.data.u8[7] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Status bytes?
  //SUNGROW_705.data.u8[0] =
  //SUNGROW_705.data.u8[1] =
  //SUNGROW_705.data.u8[2] =
  //SUNGROW_705.data.u8[3] =
  //Vbat, again (eg 400.0V = 4000 , 16bits long)
  SUNGROW_705.data.u8[5] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_705.data.u8[6] = (datalayer.battery.status.voltage_dV >> 8);

  //Temperature Max //TODO: Signed correctly?
  SUNGROW_706.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_706.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  //Temperature Min //TODO: Signed correctly?
  SUNGROW_706.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  SUNGROW_706.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
  //Cell voltage max
  SUNGROW_706.data.u8[4] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_706.data.u8[5] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  //Cell voltage min
  SUNGROW_706.data.u8[6] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SUNGROW_706.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //Temperature TODO: Signed correctly?
  SUNGROW_713.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_713.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  //Temperature TODO: Signed correctly?
  SUNGROW_713.data.u8[2] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_713.data.u8[3] = (datalayer.battery.status.temperature_max_dC >> 8);
  //Current module mA (Is whole current OK, or should it be divided/2?) Also signed OK? Scaling?
  SUNGROW_713.data.u8[4] = (datalayer.battery.status.current_dA * 10 & 0x00FF);
  SUNGROW_713.data.u8[5] = (datalayer.battery.status.current_dA * 10 >> 8);
  //Temperature TODO: Signed correctly?
  SUNGROW_713.data.u8[6] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_713.data.u8[7] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Temperature TODO: Signed correctly?
  SUNGROW_714.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_714.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  //Cell voltage
  SUNGROW_714.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_714.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  //Current module mA (Is whole current OK, or should it be divided/2?) Also signed OK? Scaling?
  SUNGROW_714.data.u8[4] = (datalayer.battery.status.current_dA * 10 & 0x00FF);
  SUNGROW_714.data.u8[5] = (datalayer.battery.status.current_dA * 10 >> 8);
  //Cell voltage
  SUNGROW_714.data.u8[6] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_714.data.u8[7] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

  //Cell voltage
  SUNGROW_715.data.u8[0] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  //Cell voltage
  SUNGROW_715.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  //Cell voltage
  SUNGROW_715.data.u8[4] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[5] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  //Cell voltage
  SUNGROW_715.data.u8[6] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[7] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

  //716-71A, reserved for 8 more modules

  //Copy 7## content to 0## messages
  for (int i = 0; i < 8; i++) {
    SUNGROW_001.data.u8[i] = SUNGROW_701.data.u8[i];
    SUNGROW_002.data.u8[i] = SUNGROW_702.data.u8[i];
    SUNGROW_003.data.u8[i] = SUNGROW_703.data.u8[i];
    SUNGROW_004.data.u8[i] = SUNGROW_704.data.u8[i];
    SUNGROW_005.data.u8[i] = SUNGROW_705.data.u8[i];
    SUNGROW_006.data.u8[i] = SUNGROW_706.data.u8[i];
    SUNGROW_013.data.u8[i] = SUNGROW_713.data.u8[i];
    SUNGROW_014.data.u8[i] = SUNGROW_714.data.u8[i];
    SUNGROW_015.data.u8[i] = SUNGROW_715.data.u8[i];
    SUNGROW_016.data.u8[i] = SUNGROW_716.data.u8[i];
    SUNGROW_017.data.u8[i] = SUNGROW_717.data.u8[i];
    SUNGROW_018.data.u8[i] = SUNGROW_718.data.u8[i];
    SUNGROW_019.data.u8[i] = SUNGROW_719.data.u8[i];
    SUNGROW_01A.data.u8[i] = SUNGROW_71A.data.u8[i];
    SUNGROW_01B.data.u8[i] = SUNGROW_71B.data.u8[i];
    SUNGROW_01C.data.u8[i] = SUNGROW_71C.data.u8[i];
    SUNGROW_01D.data.u8[i] = SUNGROW_71D.data.u8[i];
    SUNGROW_01E.data.u8[i] = SUNGROW_71E.data.u8[i];
  }

  //Copy 7## content to 5## messages
  for (int i = 0; i < 8; i++) {
    SUNGROW_501.data.u8[i] = SUNGROW_701.data.u8[i];
    SUNGROW_502.data.u8[i] = SUNGROW_702.data.u8[i];
    SUNGROW_503.data.u8[i] = SUNGROW_703.data.u8[i];
    SUNGROW_504.data.u8[i] = SUNGROW_704.data.u8[i];
    SUNGROW_505.data.u8[i] = SUNGROW_705.data.u8[i];
    SUNGROW_506.data.u8[i] = SUNGROW_706.data.u8[i];
  }

  //Status bytes (TODO: Unknown)
  //SUNGROW_100.data.u8[4] =
  //SUNGROW_100.data.u8[5] =
  //SUNGROW_100.data.u8[6] =
  //SUNGROW_100.data.u8[7] =

  //SUNGROW_500.data.u8[4] =
  //SUNGROW_500.data.u8[5] =
  //SUNGROW_500.data.u8[6] =
  //SUNGROW_500.data.u8[7] =

  //SUNGROW_400.data.u8[4] =
  //SUNGROW_400.data.u8[5] =
  //SUNGROW_400.data.u8[6] =
  //SUNGROW_400.data.u8[7] =

#ifdef DEBUG_VIA_USB
  if (inverter_sends_000) {
    Serial.println("Inverter sends 0x000");
  }
#endif
}

static void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //In here we need to respond to the inverter
    case 0x000:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_sends_000 = true;
      transmit_can_frame(&SUNGROW_001, can_config.inverter);
      transmit_can_frame(&SUNGROW_002, can_config.inverter);
      transmit_can_frame(&SUNGROW_003, can_config.inverter);
      transmit_can_frame(&SUNGROW_004, can_config.inverter);
      transmit_can_frame(&SUNGROW_005, can_config.inverter);
      transmit_can_frame(&SUNGROW_006, can_config.inverter);
      transmit_can_frame(&SUNGROW_013, can_config.inverter);
      transmit_can_frame(&SUNGROW_014, can_config.inverter);
      transmit_can_frame(&SUNGROW_015, can_config.inverter);
      transmit_can_frame(&SUNGROW_016, can_config.inverter);
      transmit_can_frame(&SUNGROW_017, can_config.inverter);
      transmit_can_frame(&SUNGROW_018, can_config.inverter);
      transmit_can_frame(&SUNGROW_019, can_config.inverter);
      transmit_can_frame(&SUNGROW_01A, can_config.inverter);
      transmit_can_frame(&SUNGROW_01B, can_config.inverter);
      transmit_can_frame(&SUNGROW_01C, can_config.inverter);
      transmit_can_frame(&SUNGROW_01D, can_config.inverter);
      transmit_can_frame(&SUNGROW_01E, can_config.inverter);
      break;
    case 0x100:  // SH10RS RUN
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x101:  // Both SH10RS / SH15T
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x102:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x103:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Version number byte1-7 (e.g. @23A229)
        version_char[0] = rx_frame.data.u8[1];
        version_char[1] = rx_frame.data.u8[2];
        version_char[2] = rx_frame.data.u8[3];
        version_char[3] = rx_frame.data.u8[4];
        version_char[4] = rx_frame.data.u8[5];
        version_char[5] = rx_frame.data.u8[6];
        version_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Version number byte1-7 continued (e.g 2795)
        version_char[7] = rx_frame.data.u8[1];
        version_char[8] = rx_frame.data.u8[2];
        version_char[9] = rx_frame.data.u8[3];
        version_char[10] = rx_frame.data.u8[4];
        version_char[11] = rx_frame.data.u8[5];
        version_char[12] = rx_frame.data.u8[6];
        version_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x104:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Manufacturer byte1-7 (e.g. SUNGROW)
        manufacturer_char[0] = rx_frame.data.u8[1];
        manufacturer_char[1] = rx_frame.data.u8[2];
        manufacturer_char[2] = rx_frame.data.u8[3];
        manufacturer_char[3] = rx_frame.data.u8[4];
        manufacturer_char[4] = rx_frame.data.u8[5];
        manufacturer_char[5] = rx_frame.data.u8[6];
        manufacturer_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Manufacturer byte1-7 continued (e.g )
        manufacturer_char[7] = rx_frame.data.u8[1];
        manufacturer_char[8] = rx_frame.data.u8[2];
        manufacturer_char[9] = rx_frame.data.u8[3];
        manufacturer_char[10] = rx_frame.data.u8[4];
        manufacturer_char[11] = rx_frame.data.u8[5];
        manufacturer_char[12] = rx_frame.data.u8[6];
        manufacturer_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x105:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Model byte1-7 (e.g. SH10RT)
        model_char[0] = rx_frame.data.u8[1];
        model_char[1] = rx_frame.data.u8[2];
        model_char[2] = rx_frame.data.u8[3];
        model_char[3] = rx_frame.data.u8[4];
        model_char[4] = rx_frame.data.u8[5];
        model_char[5] = rx_frame.data.u8[6];
        model_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Model byte1-7 continued (e.g )
        model_char[7] = rx_frame.data.u8[1];
        model_char[8] = rx_frame.data.u8[2];
        model_char[9] = rx_frame.data.u8[3];
        model_char[10] = rx_frame.data.u8[4];
        model_char[11] = rx_frame.data.u8[5];
        model_char[12] = rx_frame.data.u8[6];
        model_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x106:  // 250ms - SH10RS RUN
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x151:  //Only sent by SH15T (Inverter trying to use BYD CAN)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Manufacturer byte1-7 (e.g. SUNGROW)
        manufacturer_char[0] = rx_frame.data.u8[1];
        manufacturer_char[1] = rx_frame.data.u8[2];
        manufacturer_char[2] = rx_frame.data.u8[3];
        manufacturer_char[3] = rx_frame.data.u8[4];
        manufacturer_char[4] = rx_frame.data.u8[5];
        manufacturer_char[5] = rx_frame.data.u8[6];
        manufacturer_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Manufacturer byte1-7 continued (e.g )
        manufacturer_char[7] = rx_frame.data.u8[1];
        manufacturer_char[8] = rx_frame.data.u8[2];
        manufacturer_char[9] = rx_frame.data.u8[3];
        manufacturer_char[10] = rx_frame.data.u8[4];
        manufacturer_char[11] = rx_frame.data.u8[5];
        manufacturer_char[12] = rx_frame.data.u8[6];
        manufacturer_char[13] = rx_frame.data.u8[7];
      }
      break;
    case 0x191:  //Only sent by SH15T (Inverter trying to use BYD CAN)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x00004200:  //Only sent by SH15T (Inverter trying to use Pylon CAN)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x02007F00:  //Only sent by SH15T (Inverter trying to use Pylon CAN)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

static void transmit_can_inverter() {
  unsigned long currentMillis = millis();

  // Send 1s CAN Message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;
    //Flip flop between two sets, end result is 1s periodic rate
    if (alternate) {
      transmit_can_frame(&SUNGROW_512, can_config.inverter);
      transmit_can_frame(&SUNGROW_501, can_config.inverter);
      transmit_can_frame(&SUNGROW_502, can_config.inverter);
      transmit_can_frame(&SUNGROW_503, can_config.inverter);
      transmit_can_frame(&SUNGROW_504, can_config.inverter);
      transmit_can_frame(&SUNGROW_505, can_config.inverter);
      transmit_can_frame(&SUNGROW_506, can_config.inverter);
      transmit_can_frame(&SUNGROW_500, can_config.inverter);
      transmit_can_frame(&SUNGROW_400, can_config.inverter);
      alternate = false;
    } else {
      transmit_can_frame(&SUNGROW_700, can_config.inverter);
      transmit_can_frame(&SUNGROW_701, can_config.inverter);
      transmit_can_frame(&SUNGROW_702, can_config.inverter);
      transmit_can_frame(&SUNGROW_703, can_config.inverter);
      transmit_can_frame(&SUNGROW_704, can_config.inverter);
      transmit_can_frame(&SUNGROW_705, can_config.inverter);
      transmit_can_frame(&SUNGROW_706, can_config.inverter);
      transmit_can_frame(&SUNGROW_713, can_config.inverter);
      transmit_can_frame(&SUNGROW_714, can_config.inverter);
      transmit_can_frame(&SUNGROW_715, can_config.inverter);
      transmit_can_frame(&SUNGROW_716, can_config.inverter);
      transmit_can_frame(&SUNGROW_717, can_config.inverter);
      transmit_can_frame(&SUNGROW_718, can_config.inverter);
      transmit_can_frame(&SUNGROW_719, can_config.inverter);
      transmit_can_frame(&SUNGROW_71A, can_config.inverter);
      transmit_can_frame(&SUNGROW_71B, can_config.inverter);
      transmit_can_frame(&SUNGROW_71C, can_config.inverter);
      transmit_can_frame(&SUNGROW_71D, can_config.inverter);
      transmit_can_frame(&SUNGROW_71E, can_config.inverter);
      alternate = true;
    }
  }
}

void SungrowCanInverter::transmit_can() {
  transmit_can_inverter();
}

#endif
