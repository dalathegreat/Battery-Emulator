#include "../include.h"
#ifdef GEELY_GEOMETRY_C_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "GEELY-GEOMETRY-C-BATTERY.h"

/* TODO
- CAN log needed from complete H-CAN of Geely Geometry C vehicle. We are not sure what needs to be sent towards the battery yet to get contactor closing working
/*

/* Do not change code below unless you are sure what you are doing */

const unsigned char crctable[256] = {  // CRC8_SAE_J1850_ZER0 formula,0x2F Poly,initial value 0xFF,Final XOR value 0xFF
    0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2, 0xCD, 0x57, 0x78, 0x09, 0x26, 0xEB, 0xC4, 0xB5, 0x9A, 0xAE, 0x81, 0xF0,
    0xDF, 0x12, 0x3D, 0x4C, 0x63, 0xF9, 0xD6, 0xA7, 0x88, 0x45, 0x6A, 0x1B, 0x34, 0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0,
    0x91, 0xBE, 0x24, 0x0B, 0x7A, 0x55, 0x98, 0xB7, 0xC6, 0xE9, 0xDD, 0xF2, 0x83, 0xAC, 0x61, 0x4E, 0x3F, 0x10, 0x8A,
    0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47, 0xE6, 0xC9, 0xB8, 0x97, 0x5A, 0x75, 0x04, 0x2B, 0xB1, 0x9E, 0xEF, 0xC0,
    0x0D, 0x22, 0x53, 0x7C, 0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85, 0x1F, 0x30, 0x41, 0x6E, 0xA3, 0x8C, 0xFD,
    0xD2, 0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77, 0x58, 0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F, 0x3B, 0x14,
    0x65, 0x4A, 0x87, 0xA8, 0xD9, 0xF6, 0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E, 0xA1, 0xE3, 0xCC, 0xBD, 0x92, 0x5F,
    0x70, 0x01, 0x2E, 0xB4, 0x9B, 0xEA, 0xC5, 0x08, 0x27, 0x56, 0x79, 0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF, 0x80,
    0x1A, 0x35, 0x44, 0x6B, 0xA6, 0x89, 0xF8, 0xD7, 0x90, 0xBF, 0xCE, 0xE1, 0x2C, 0x03, 0x72, 0x5D, 0xC7, 0xE8, 0x99,
    0xB6, 0x7B, 0x54, 0x25, 0x0A, 0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD, 0xDC, 0xF3, 0x69, 0x46, 0x37, 0x18, 0xD5, 0xFA,
    0x8B, 0xA4, 0x05, 0x2A, 0x5B, 0x74, 0xB9, 0x96, 0xE7, 0xC8, 0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F, 0xAB,
    0x84, 0xF5, 0xDA, 0x17, 0x38, 0x49, 0x66, 0xFC, 0xD3, 0xA2, 0x8D, 0x40, 0x6F, 0x1E, 0x31, 0x76, 0x59, 0x28, 0x07,
    0xCA, 0xE5, 0x94, 0xBB, 0x21, 0x0E, 0x7F, 0x50, 0x9D, 0xB2, 0xC3, 0xEC, 0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A,
    0x15, 0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42};

/* Node descriptions, these can send CAN messages in the Geely Geometry C
DSCU (Drivers Seat Control Unit)
OBC (On Board Charger)
FRS (Front Radar System)
IPU (Integrated Power Unit Control)
EGSM (Electronic Gear Shifter)
MMI
T-BOX (Electrocar Communication Control Module)
IPK
FCS
FRS
TCM(SAS)
RPS
ESC
ACU(YRS)
DSCU
PEPS
ESCL
BCM (Body Control Module)
AC
BMSH (Battery Management System)
VCU (Vehicle Control Unit)
AVAS
IB
RSRS
RML

There are 4 CAN buses in the Geometry C, we are interested in the Hybrid CAN (HB-CAN)
- Hybrid, HB CAN: gateway, electronic shifter, VCU, T-BOX, BMS, high and low voltage charging system, integrated power controller
- Infotainent, IF CAN: gateway, diagnostic interface, combined instrument, controller, head-up display, audio host, T-BOX
- Comfort, CF CAN: gateway, diagnostic interface, low-speed alarm controller, thermal management control module, electronic 
steering column lock, BCM, seat module
- Chassis, CS CAN: gateway, steering wheel angle sensor, front monocular camera, VCU, millimeter wave radar probe, EPS,
smart booster, ESC, airbag control module, automatic parking module
*/

CAN_frame GEELY_191 = {.FD = false,  //PAS_APA_Status , 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x191,
                       .data = {0x00, 0x00, 0x81, 0x20, 0x00, 0x00, 0x00, 0x01}};
CAN_frame GEELY_2D2 = {.FD = false,  //DSCU 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x2D2,
                       .data = {0x60, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame GEELY_0A6 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0A6,
                       .data = {0xFA, 0x0F, 0xA0, 0x00, 0x00, 0xFA, 0x00, 0xE4}};
CAN_frame GEELY_160 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x160,
                       .data = {0x00, 0x01, 0x67, 0xF7, 0xC0, 0x19, 0x00, 0x20}};
CAN_frame GEELY_165 = {.FD = false,  //VCU_ModeControl 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x165,
                       .data = {0x00, 0x81, 0xA1, 0x00, 0x00, 0x1E, 0x00, 0xD6}};
CAN_frame GEELY_1A4 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1A4,
                       .data = {0x17, 0x73, 0x17, 0x70, 0x02, 0x1C, 0x00, 0x56}};
CAN_frame GEELY_162 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x162,
                       .data = {0x00, 0x05, 0x06, 0x81, 0x00, 0x09, 0x00, 0xC6}};
CAN_frame GEELY_1A5 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1A5,
                       .data = {0x17, 0x70, 0x24, 0x0B, 0x00, 0x00, 0x00, 0xF9}};
CAN_frame GEELY_1B2 = {.FD = false,  //??? 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1B2,
                       .data = {0x17, 0x70, 0x24, 0x0B, 0x00, 0x00, 0x00, 0xF9}};
CAN_frame GEELY_221 = {.FD = false,  //OBC 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x221,
                       .data = {0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00}};
CAN_frame GEELY_220 = {.FD = false,  //OBC 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x220,
                       .data = {0x0B, 0x43, 0x69, 0xF3, 0x3A, 0x10, 0x00, 0x31}};
CAN_frame GEELY_1A3 = {.FD = false,  //FRS 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1A3,
                       .data = {0xFF, 0x18, 0x20, 0x00, 0x00, 0x00, 0x00, 0x4F}};
CAN_frame GEELY_1A7 = {.FD = false,  //??? 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1A7,
                       .data = {0x00, 0x7F, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00}};
CAN_frame GEELY_0A8 = {.FD = false,  //IPU 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0A8,
                       .data = {0x00, 0x2E, 0xDC, 0x4E, 0x20, 0x00, 0x20, 0xA2}};
CAN_frame GEELY_1F2 = {.FD = false,  //??? 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1F2,
                       .data = {0x9B, 0xA3, 0x99, 0xA2, 0x41, 0x42, 0x41, 0x42}};
CAN_frame GEELY_222 = {.FD = false,  //OBC 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x222,
                       .data = {0x00, 0x00, 0x00, 0xFF, 0xF8, 0x00, 0x00, 0x00}};
CAN_frame GEELY_1A6 = {.FD = false,  //OBC 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1A6,
                       .data = {0x00, 0x7F, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00}};
CAN_frame GEELY_145 = {.FD = false,  //EGSM 20ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x145,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A}};
CAN_frame GEELY_0E0 = {.FD = false,  //IPU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0E0,
                       .data = {0xFF, 0x09, 0x00, 0xE0, 0x00, 0x8F, 0x00, 0x00}};
CAN_frame GEELY_0F9 = {.FD = false,  //??? 20ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0F9,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame GEELY_292 = {.FD = false,  //T-BOX 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x292,
                       .data = {0x00, 0x00, 0x00, 0x1F, 0xE7, 0xE7, 0x00, 0xBC}};
CAN_frame GEELY_0FA = {.FD = false,  //??? 20ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0FA,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame GEELY_197 = {.FD = false,  //??? 20ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x197,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A}};
CAN_frame GEELY_150 = {.FD = false,  //EPS 20ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x150,
                       .data = {0x7E, 0x00, 0x24, 0x00, 0x01, 0x01, 0x00, 0xA9}};
CAN_frame GEELY_POLL = {.FD = false,  //Polling frame
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x7E2,
                        .data = {0x03, 0x22, 0x4B, 0xDA, 0x00, 0x00, 0x00, 0x00}};
CAN_frame GEELY_ACK = {.FD = false,  //Ack frame
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x7E2,
                       .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
static uint16_t poll_pid = POLL_SOC;
static uint16_t incoming_poll = 0;
static uint8_t counter_10ms = 0;
static uint8_t counter_20ms = 0;
static uint8_t counter_50ms = 0;
static uint8_t counter_100ms = 0;
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis20 = 0;   // will store last time a 20ms CAN Message was sent
static unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was sent
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was sent
static uint8_t mux = 0;
static uint16_t battery_voltage = 3700;
static int16_t maximum_temperature = 0;
static int16_t minimum_temperature = 0;
static uint8_t HVIL_signal = 0;
static uint8_t serialnumbers[28] = {0};
static uint16_t maximum_cell_voltage = 3700;
static uint16_t discharge_power_allowed = 0;
static uint16_t poll_soc = 0;
static uint16_t poll_cc2_voltage = 0;
static uint16_t poll_cell_max_voltage_number = 0;
static uint16_t poll_cell_min_voltage_number = 0;
static uint16_t poll_amount_cells = 0;
static uint16_t poll_specificial_voltage = 0;
static uint16_t poll_unknown1 = 0;
static uint16_t poll_raw_soc_max = 0;
static uint16_t poll_raw_soc_min = 0;
static uint16_t poll_unknown4 = 0;
static uint16_t poll_unknown5 = 0;
static uint16_t poll_unknown6 = 0;
static uint16_t poll_unknown7 = 0;
static uint16_t poll_unknown8 = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.real_soc = poll_soc * 10;

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = discharge_power_allowed * 100;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC = minimum_temperature * 10;

  datalayer.battery.status.temperature_max_dC = maximum_temperature * 10;

  datalayer.battery.status.cell_min_voltage_mV = maximum_cell_voltage - 10;  //TODO: Fix once we have min value

  datalayer.battery.status.cell_max_voltage_mV = maximum_cell_voltage;

  if (HVIL_signal > 0) {
    set_event(EVENT_HVIL_FAILURE, HVIL_signal);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  //Update webserver more battery info page
  memcpy(datalayer_extended.geometryC.BatterySerialNumber, serialnumbers, sizeof(serialnumbers));
  datalayer_extended.geometryC.soc = poll_soc;
  datalayer_extended.geometryC.CC2voltage = poll_cc2_voltage;
  datalayer_extended.geometryC.cellMaxVoltageNumber = poll_cell_max_voltage_number;
  datalayer_extended.geometryC.cellMinVoltageNumber = poll_cell_min_voltage_number;
  datalayer_extended.geometryC.cellTotalAmount = poll_amount_cells;
  datalayer_extended.geometryC.specificialVoltage = poll_specificial_voltage;
  datalayer_extended.geometryC.unknown1 = poll_unknown1;
  datalayer_extended.geometryC.rawSOCmax = poll_raw_soc_max;
  datalayer_extended.geometryC.rawSOCmin = poll_raw_soc_min;
  datalayer_extended.geometryC.unknown4 = poll_unknown4;
  datalayer_extended.geometryC.unknown5 = poll_unknown5;
  datalayer_extended.geometryC.unknown6 = poll_unknown6;
  datalayer_extended.geometryC.unknown7 = poll_unknown7;
  datalayer_extended.geometryC.unknown8 = poll_unknown8;
}

bool is_message_corrupt(CAN_frame* rx_frame) {
  uint8_t crc = 0xFF;  // Initial value
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable[crc ^ rx_frame->data.u8[j]];
  }
  crc = (crc ^ 0xFF);  // Final XOR
  return crc != rx_frame->data.u8[7];
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x0B0:  //10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      //Contains:
      //HVIL Signal
      // - HVIL not connected: 000000B0 00 8 10 06 00 00 00 00 8E 31
      // - HVIL connected: 000000B0 00 8 10 00 00 00 00 00 82 0D
      // Based on this, the two HVIL is most likely frame1
      HVIL_signal = (rx_frame.data.u8[1] & 0x0F);
      //BatteryDchgSysFaultLevel
      //ChgFaultLevel
      //frame7, CRC
      //frame6, low byte counter 0-F
      break;
    case 0x178:  //10ms (64 13 88 00 0E 30 0A 85)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      battery_voltage = ((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5];
      //frame7, CRC
      //frame6, low byte counter 0-F
      break;
    case 0x179:  //20ms (3E 52 BA 5D A4 3F 0C D9)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      //2BA = 69.8 //Potentially charge power allowed
      //frame7, CRC
      //frame6, low byte counter 0-F
      break;
    case 0x17A:  //100ms (Battery 01 B4 52 28 4A 46 6E AE)
                 //(Car log 0A 3D EE F1 BD C6 67 F7)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      //frame7, CRC
      //frame6, low byte counter 0-F
      break;
    case 0x17B:  //20ms (00 00 10 00 0F FE 03 C9) (car is the same, static)
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      //frame7, CRC
      //frame6, low byte counter 0-F
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x210:  //100ms (38 04 3A 01 38 22 22 39)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      discharge_power_allowed = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2];  //TODO, not confirmed
      //43A = 108.2kW potentially discharge power allowed
      //frame7, CRC
      //frame6, counter 0 - 0x22
      break;
    case 0x211:  //100ms (00 D8 C6 00 00 00 0F 3A)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(&rx_frame)) {
        datalayer.battery2.status.CAN_error_counter++;
        break;  //Message content malformed, abort reading data from it
      }
      //frame7, CRC
      //frame6, low byte counter 0-F
      break;
    case 0x212:  //500ms (Completely empty)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x351:  //100ms (4A 31 71 B8 6E F8 84 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      maximum_temperature = ((rx_frame.data.u8[6] / 2) - 40);  //TODO, not confirmed
      break;
    case 0x352:  //500ms (76 78 00 00 82 FF FF 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      minimum_temperature = ((rx_frame.data.u8[4] / 2) - 40);  //TODO, not confirmed
      break;
    case 0x353:  //500ms (00 00 00 00 62 00 EA 5D) (car 00 00 00 00 00 00 E6 04)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x354:  //500ms (Completely empty)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x355:  //500ms (89 4A 03 5C 39 06 04 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x356:  //1s (6B 09 0C 69 0A F1 D3 86)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x357:  //20ms (18 17 6F 20 00 00 00 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Frame 0 and 1 seems to count something large
      break;
    case 0x358:  //1s (03 DF 10 3C DA 0E 20 DE)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x359:  //1s (1F 40 00 00 00 00 00 36)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Frame7 loops 01-21-22-36-01-21...
      break;
    case 0x35B:  //200ms (Serialnumbers)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      switch (mux) {
        case 0x01:
          serialnumbers[0] = rx_frame.data.u8[1];
          serialnumbers[1] = rx_frame.data.u8[2];
          serialnumbers[2] = rx_frame.data.u8[3];
          serialnumbers[3] = rx_frame.data.u8[4];
          serialnumbers[4] = rx_frame.data.u8[5];
          serialnumbers[5] = rx_frame.data.u8[6];
          serialnumbers[6] = rx_frame.data.u8[7];
          break;
        case 0x02:
          serialnumbers[7] = rx_frame.data.u8[1];
          serialnumbers[8] = rx_frame.data.u8[2];
          serialnumbers[9] = rx_frame.data.u8[3];
          serialnumbers[10] = rx_frame.data.u8[4];
          serialnumbers[11] = rx_frame.data.u8[5];
          serialnumbers[12] = rx_frame.data.u8[6];
          serialnumbers[13] = rx_frame.data.u8[7];
          break;
        case 0x03:
          serialnumbers[14] = rx_frame.data.u8[1];
          serialnumbers[15] = rx_frame.data.u8[2];
          serialnumbers[16] = rx_frame.data.u8[3];
          serialnumbers[17] = rx_frame.data.u8[4];
          serialnumbers[18] = rx_frame.data.u8[5];
          serialnumbers[19] = rx_frame.data.u8[6];
          serialnumbers[20] = rx_frame.data.u8[7];
          break;
        case 0x04:
          serialnumbers[21] = rx_frame.data.u8[1];
          serialnumbers[22] = rx_frame.data.u8[2];
          serialnumbers[23] = rx_frame.data.u8[3];
          serialnumbers[24] = rx_frame.data.u8[4];
          serialnumbers[25] = rx_frame.data.u8[5];
          serialnumbers[26] = rx_frame.data.u8[6];
          serialnumbers[27] = rx_frame.data.u8[7];
          break;
        default:
          break;
      }
      break;
    case 0x424:  //500ms (24 10 01 01 02 00 00 00)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EA:
      if (rx_frame.data.u8[0] == 0x10) {  //Multiframe response, send ACK
        transmit_can_frame(&GEELY_ACK, can_config.battery);
      }
      incoming_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

      switch (incoming_poll) {
        case POLL_SOC:
          poll_soc = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CC2_VOLTAGE:
          poll_cc2_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CELL_MAX_VOLTAGE_NUMBER:
          poll_cell_max_voltage_number = rx_frame.data.u8[4];
          break;
        case POLL_CELL_MIN_VOLTAGE_NUMBER:
          poll_cell_min_voltage_number = rx_frame.data.u8[4];
          break;
        case POLL_AMOUNT_CELLS:
          poll_amount_cells = rx_frame.data.u8[4];
          datalayer.battery.info.number_of_cells = poll_amount_cells;
          break;
        case POLL_SPECIFICIAL_VOLTAGE:
          poll_specificial_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_UNKNOWN_1:
          poll_unknown1 = rx_frame.data.u8[4];
          break;
        case POLL_RAW_SOC_MAX:
          poll_raw_soc_max = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_RAW_SOC_MIN:
          poll_raw_soc_min = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_UNKNOWN_4:
          poll_unknown4 = rx_frame.data.u8[4];
          break;
        case POLL_UNKNOWN_5:
          poll_unknown5 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_UNKNOWN_6:
          poll_unknown6 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_UNKNOWN_7:
          poll_unknown7 = rx_frame.data.u8[4];
          break;
        case POLL_UNKNOWN_8:
          poll_unknown8 = rx_frame.data.u8[4];
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

uint8_t calc_crc8_geely(CAN_frame* rx_frame) {
  uint8_t crc = 0xFF;  // Initial value

  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable[crc ^ rx_frame->data.u8[j]];
  }

  return crc ^ 0xFF;  // Final XOR
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    GEELY_191.data.u8[6] = ((GEELY_191.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_191.data.u8[7] = calc_crc8_geely(&GEELY_191);
    GEELY_0A6.data.u8[6] = ((GEELY_0A6.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_0A6.data.u8[7] = calc_crc8_geely(&GEELY_0A6);
    GEELY_165.data.u8[6] = ((GEELY_165.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_165.data.u8[7] = calc_crc8_geely(&GEELY_165);
    GEELY_1A4.data.u8[6] = ((GEELY_1A4.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_1A4.data.u8[7] = calc_crc8_geely(&GEELY_1A4);
    GEELY_162.data.u8[6] = ((GEELY_162.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_162.data.u8[7] = calc_crc8_geely(&GEELY_162);
    GEELY_1A5.data.u8[6] = ((GEELY_1A5.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_1A5.data.u8[7] = calc_crc8_geely(&GEELY_1A5);
    GEELY_220.data.u8[6] = ((GEELY_220.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_220.data.u8[7] = calc_crc8_geely(&GEELY_220);
    GEELY_0E0.data.u8[4] = ((GEELY_0E0.data.u8[4] & 0xF0) | counter_10ms);  //unique
    GEELY_0E0.data.u8[5] = calc_crc8_geely(&GEELY_0E0);                     //unique

    counter_10ms = (counter_10ms + 1) % 17;  // 0-1-...F-0-1 etc.

    transmit_can_frame(&GEELY_191, can_config.battery);
    transmit_can_frame(&GEELY_0A6, can_config.battery);
    transmit_can_frame(&GEELY_160, can_config.battery);
    transmit_can_frame(&GEELY_165, can_config.battery);
    transmit_can_frame(&GEELY_1A4, can_config.battery);
    transmit_can_frame(&GEELY_162, can_config.battery);  //CONFIRMED MANDATORY! VCU message
    transmit_can_frame(&GEELY_1A5, can_config.battery);
    transmit_can_frame(&GEELY_220, can_config.battery);  //CONFIRMED MANDATORY! OBC message
    transmit_can_frame(&GEELY_0E0, can_config.battery);
  }
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    GEELY_145.data.u8[6] = ((GEELY_145.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_145.data.u8[7] = calc_crc8_geely(&GEELY_145);
    GEELY_150.data.u8[6] = ((GEELY_150.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_150.data.u8[7] = calc_crc8_geely(&GEELY_150);

    counter_20ms = (counter_20ms + 1) % 17;  // 0-1-...F-0-1 etc.

    transmit_can_frame(&GEELY_145, can_config.battery);  //CONFIRMED MANDATORY! shifter
    transmit_can_frame(&GEELY_0F9, can_config.battery);  //CONFIRMED MANDATORY! shifter
    transmit_can_frame(&GEELY_0FA, can_config.battery);  //Might be unnecessary, not in workshop manual
    transmit_can_frame(&GEELY_197, can_config.battery);  //Might be unnecessary, not in workshop manual
    transmit_can_frame(&GEELY_150, can_config.battery);
  }
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    GEELY_1A3.data.u8[6] = ((GEELY_1A3.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_1A3.data.u8[7] = calc_crc8_geely(&GEELY_1A3);
    GEELY_0A8.data.u8[6] = ((GEELY_0A8.data.u8[6] & 0xF0) | counter_10ms);
    GEELY_0A8.data.u8[7] = calc_crc8_geely(&GEELY_0A8);

    counter_50ms = (counter_50ms + 1) % 17;  // 0-1-...F-0-1 etc.

    transmit_can_frame(&GEELY_1B2, can_config.battery);
    transmit_can_frame(&GEELY_221, can_config.battery);  //CONFIRMED MANDATORY! OBC message
    //transmit_can_frame(&GEELY_1A3, can_config.battery);  //Might be unnecessary, radar info
    //transmit_can_frame(&GEELY_1A7, can_config.battery);  //Might be unnecessary
    transmit_can_frame(&GEELY_0A8, can_config.battery);  //CONFIRMED MANDATORY! IPU message
    //transmit_can_frame(&GEELY_1F2, can_config.battery);  //Might be unnecessary
    //transmit_can_frame(&GEELY_1A6, can_config.battery);  //Might be unnecessary
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    GEELY_0A8.data.u8[6] = ((GEELY_0A8.data.u8[6] & 0x0F) | (counter_10ms << 4));  //unique bitshift
    GEELY_0A8.data.u8[7] = calc_crc8_geely(&GEELY_0A8);

    counter_100ms = (counter_100ms + 1) % 17;  // 0-1-...F-0-1 etc.

    transmit_can_frame(&GEELY_222, can_config.battery);  //CONFIRMED MANDATORY! OBC message
    //transmit_can_frame(&GEELY_2D2, can_config.battery);  //Might be unnecessary, seat info
    transmit_can_frame(&GEELY_292, can_config.battery);  //CONFIRMED MANDATORY! T-BOX
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (poll_pid) {
      case POLL_SOC:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_SOC >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_SOC;
        poll_pid = POLL_CC2_VOLTAGE;
        break;
      case POLL_CC2_VOLTAGE:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_CC2_VOLTAGE >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_CC2_VOLTAGE;
        poll_pid = POLL_CELL_MAX_VOLTAGE_NUMBER;
        break;
      case POLL_CELL_MAX_VOLTAGE_NUMBER:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_CELL_MAX_VOLTAGE_NUMBER >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_CELL_MAX_VOLTAGE_NUMBER;
        poll_pid = POLL_CELL_MIN_VOLTAGE_NUMBER;
        break;
      case POLL_CELL_MIN_VOLTAGE_NUMBER:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_CELL_MIN_VOLTAGE_NUMBER >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_CELL_MIN_VOLTAGE_NUMBER;
        poll_pid = POLL_AMOUNT_CELLS;
        break;
      case POLL_AMOUNT_CELLS:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_AMOUNT_CELLS >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_AMOUNT_CELLS;
        poll_pid = POLL_SPECIFICIAL_VOLTAGE;
        break;
      case POLL_SPECIFICIAL_VOLTAGE:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_SPECIFICIAL_VOLTAGE >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_SPECIFICIAL_VOLTAGE;
        poll_pid = POLL_UNKNOWN_1;
        break;
      case POLL_UNKNOWN_1:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_UNKNOWN_1 >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_UNKNOWN_1;
        poll_pid = POLL_RAW_SOC_MAX;
        break;
      case POLL_RAW_SOC_MAX:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_RAW_SOC_MAX >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_RAW_SOC_MAX;
        poll_pid = POLL_RAW_SOC_MIN;
        break;
      case POLL_RAW_SOC_MIN:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_RAW_SOC_MIN >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_RAW_SOC_MIN;
        poll_pid = POLL_UNKNOWN_4;
        break;
      case POLL_UNKNOWN_4:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_UNKNOWN_4 >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_UNKNOWN_4;
        poll_pid = POLL_UNKNOWN_5;
        break;
      case POLL_UNKNOWN_5:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_UNKNOWN_5 >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_UNKNOWN_5;
        poll_pid = POLL_UNKNOWN_6;
        break;
      case POLL_UNKNOWN_6:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_UNKNOWN_6 >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_UNKNOWN_6;
        poll_pid = POLL_UNKNOWN_7;
        break;
      case POLL_UNKNOWN_7:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_UNKNOWN_7 >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_UNKNOWN_7;
        poll_pid = POLL_UNKNOWN_8;
        break;
      case POLL_UNKNOWN_8:
        GEELY_POLL.data.u8[2] = (uint8_t)(POLL_UNKNOWN_8 >> 8);
        GEELY_POLL.data.u8[3] = (uint8_t)POLL_UNKNOWN_8;
        poll_pid = POLL_SOC;
        break;
      default:
        poll_pid = POLL_SOC;
        break;
    }

    transmit_can_frame(&GEELY_POLL, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Geely Geometry C", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 102;                           //70kWh pack has 102S, startup in this mode
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_70_DV;  //Startup in extreme ends
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_53_DV;  //Before pack size determined
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif
