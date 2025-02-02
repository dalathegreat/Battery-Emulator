#include "../include.h"
#ifdef NISSANLEAF_CHARGER
#include "../datalayer/datalayer.h"
#include "NISSAN-LEAF-CHARGER.h"

/* This implements Nissan LEAF PDM charger support. 2013-2024 Gen2/3 PDMs are supported
 *
 * This code is intended to facilitate standalone battery charging, 
 * for instance for troubleshooting purposes or emergency generator usage
 *
 * Credits go to:
 *   Damien Maguire (evmbw.org, https://github.com/damienmaguire/Stm32-vcu)
 *
 * The code functions a bit differently incase a Nissan LEAF battery is used. Almost
 * all CAN messages are already sent from the battery in that case, so the charger
 * implementation can focus on only controlling the charger, spoofing battery messages
 * is not needed.
 *
 * Incase another battery type is used, the code ofcourse emulates a complete Nissan LEAF
 * battery onto the CAN bus. 
*/

/* CAN cycles and timers */
static unsigned long previousMillis10ms = 0;
static unsigned long previousMillis100ms = 0;

/* LEAF charger/battery parameters */
enum OBC_MODES : uint8_t {
  IDLE_OR_QC = 1,
  FINISHED = 2,
  CHARGING_OR_INTERRUPTED = 4,
  IDLE1 = 8,
  IDLE2 = 9,
  PLUGGED_IN_WAITING_ON_TIMER
};
enum OBC_VOLTAGES : uint8_t { NO_SIGNAL = 0, AC110 = 1, AC230 = 2, ABNORMAL_WAVE = 3 };
static uint16_t OBC_Charge_Power = 0;  // Actual charger output
static uint8_t mprun100 = 0;           // Counter 0-3
static uint8_t mprun10 = 0;            // Counter 0-3
static uint8_t OBC_Charge_Status = IDLE_OR_QC;
static uint8_t OBC_Status_AC_Voltage = 0;  //1=110V, 2=230V
static uint8_t OBCpowerSetpoint = 0;
static uint8_t OBCpower = 0;
static bool PPStatus = false;
static bool OBCwakeup = false;

//Actual content messages
static CAN_frame LEAF_1DB = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x1DB,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00}};
static CAN_frame LEAF_1DC = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x1DC,
                             .data = {0x6E, 0x0A, 0x05, 0xD5, 0x00, 0x00, 0x00, 0x00}};
static CAN_frame LEAF_1F2 = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x1F2,
                             .data = {0x30, 0x00, 0x20, 0xAC, 0x00, 0x3C, 0x00, 0x8F}};
static CAN_frame LEAF_50B = {.FD = false,
                             .ext_ID = false,
                             .DLC = 7,
                             .ID = 0x50B,
                             .data = {0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x00}};
static CAN_frame LEAF_55B = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x55B,
                             .data = {0xA4, 0x40, 0xAA, 0x00, 0xDF, 0xC0, 0x10, 0x00}};
static CAN_frame LEAF_5BC = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x5BC,
                             .data = {0x3D, 0x80, 0xF0, 0x64, 0xB0, 0x01, 0x00, 0x32}};

static CAN_frame LEAF_59E = {.FD = false,
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x59E,
                             .data = {0x00, 0x00, 0x0C, 0x76, 0x18, 0x00, 0x00, 0x00}};

static uint8_t crctable[256] = {
    0,   133, 143, 10,  155, 30,  20,  145, 179, 54,  60,  185, 40,  173, 167, 34,  227, 102, 108, 233, 120, 253,
    247, 114, 80,  213, 223, 90,  203, 78,  68,  193, 67,  198, 204, 73,  216, 93,  87,  210, 240, 117, 127, 250,
    107, 238, 228, 97,  160, 37,  47,  170, 59,  190, 180, 49,  19,  150, 156, 25,  136, 13,  7,   130, 134, 3,
    9,   140, 29,  152, 146, 23,  53,  176, 186, 63,  174, 43,  33,  164, 101, 224, 234, 111, 254, 123, 113, 244,
    214, 83,  89,  220, 77,  200, 194, 71,  197, 64,  74,  207, 94,  219, 209, 84,  118, 243, 249, 124, 237, 104,
    98,  231, 38,  163, 169, 44,  189, 56,  50,  183, 149, 16,  26,  159, 14,  139, 129, 4,   137, 12,  6,   131,
    18,  151, 157, 24,  58,  191, 181, 48,  161, 36,  46,  171, 106, 239, 229, 96,  241, 116, 126, 251, 217, 92,
    86,  211, 66,  199, 205, 72,  202, 79,  69,  192, 81,  212, 222, 91,  121, 252, 246, 115, 226, 103, 109, 232,
    41,  172, 166, 35,  178, 55,  61,  184, 154, 31,  21,  144, 1,   132, 142, 11,  15,  138, 128, 5,   148, 17,
    27,  158, 188, 57,  51,  182, 39,  162, 168, 45,  236, 105, 99,  230, 119, 242, 248, 125, 95,  218, 208, 85,
    196, 65,  75,  206, 76,  201, 195, 70,  215, 82,  88,  221, 255, 122, 112, 245, 100, 225, 235, 110, 175, 42,
    32,  165, 52,  177, 187, 62,  28,  153, 147, 22,  135, 2,   8,   141};

static uint8_t calculate_CRC_Nissan(CAN_frame* frame) {
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable[(crc ^ static_cast<uint8_t>(frame->data.u8[j])) % 256];
  }
  return crc;
}

static uint8_t calculate_checksum_nibble(CAN_frame* frame) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 7; i++) {
    sum += frame->data.u8[i] >> 4;
    sum += frame->data.u8[i] & 0xF;
  }
  sum = (sum + 2) & 0xF;
  return sum;
}

void map_can_frame_to_variable_charger(CAN_frame rx_frame) {

  switch (rx_frame.ID) {
    case 0x679:  // This message fires once when charging cable is plugged in
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      OBCwakeup = true;
      datalayer.charger.charger_aux12V_enabled = true;  //Not possible to turn off 12V charging on LEAF PDM
      // Startout with default values, so that charging can begin right when user plugs in cable
      datalayer.charger.charger_HV_enabled = true;
      datalayer.charger.charger_setpoint_HV_IDC = 16;   // Ampere
      datalayer.charger.charger_setpoint_HV_VDC = 400;  // Target voltage
      break;
    case 0x390:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      OBC_Charge_Status = ((rx_frame.data.u8[5] & 0x7E) >> 1);
      if (OBC_Charge_Status == PLUGGED_IN_WAITING_ON_TIMER || CHARGING_OR_INTERRUPTED) {
        PPStatus = true;  //plug inserted
      } else {
        PPStatus = false;  //plug not inserted
      }
      OBC_Status_AC_Voltage = ((rx_frame.data.u8[3] & 0x18) >> 3);
      if (OBC_Status_AC_Voltage == AC110) {
        datalayer.charger.charger_stat_ACvol = 110;
      }
      if (OBC_Status_AC_Voltage == AC230) {
        datalayer.charger.charger_stat_ACvol = 230;
      }
      if (OBC_Status_AC_Voltage == ABNORMAL_WAVE) {
        datalayer.charger.charger_stat_ACvol = 1;
      }

      OBC_Charge_Power = ((rx_frame.data.u8[0] & 0x01) << 8) | (rx_frame.data.u8[1]);
      datalayer.charger.charger_stat_HVcur = OBC_Charge_Power;
      break;
    case 0x393:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      break;
    default:
      break;
  }
}

void transmit_can_charger() {
  unsigned long currentMillis = millis();

  /* Send keepalive with mode every 10ms */
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...

/* 1DB is the main control message. If LEAF battery is used, the battery controls almost everything */
// Only send these messages if Nissan LEAF battery is not used
#ifndef NISSAN_LEAF_BATTERY

    // VCM message, containing info if battery should sleep or stay awake
    transmit_can_frame(&LEAF_50B, can_config.charger);  // HCM_WakeUpSleepCommand == 11b == WakeUp, and CANMASK = 1

    LEAF_1DB.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DB);
    transmit_can_frame(&LEAF_1DB, can_config.charger);

    LEAF_1DC.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DC);
    transmit_can_frame(&LEAF_1DC, can_config.charger);
#endif

    OBCpowerSetpoint = ((datalayer.charger.charger_setpoint_HV_IDC * 4) + 0x64);

    // convert power setpoint to PDM format:
    //    0xA0 = 15A (60x)
    //    0x70 = 3 amps ish (12x)
    //    0x6a = 1.4A (6x)
    //    0x66 = 0.5A (2x)
    //    0x65 = 0.3A (1x)
    //    0x64 = no chg
    //    so 0x64=100. 0xA0=160. so 60 decimal steps. 1 step=100W???

    // This line controls if power should flow or not
    if (PPStatus &&
        datalayer.charger
            .charger_HV_enabled) {  //Charging starts when cable plugged in and User has requested charging to start via WebUI
      // clamp min and max values
      if (OBCpowerSetpoint > 0xA0) {  //15A TODO, raise once cofirmed how to map bits into frame0 and frame1
        OBCpowerSetpoint = 0xA0;
      } else if (OBCpowerSetpoint <= 0x64) {
        OBCpowerSetpoint = 0x64;  // 100W? stuck at 100 in drive mode (no charging)
      }

      // if actual battery_voltage is less than setpoint got to max power set from web ui
      if (datalayer.battery.status.voltage_dV <
          (CHARGER_SET_HV * 10)) {  //datalayer.battery.status.voltage_dV = V+1,  0-500.0 (0-5000)
        OBCpower = OBCpowerSetpoint;
      }

      // decrement charger power if volt setpoint is reached
      if (datalayer.battery.status.voltage_dV >= (CHARGER_SET_HV * 10)) {
        if (OBCpower > 0x64) {
          OBCpower--;
        }
      }
    } else {
      // set power to 0 if charge control is set to off or not in charge mode
      OBCpower = 0x64;
    }

    LEAF_1F2.data.u8[1] = OBCpower;
    LEAF_1F2.data.u8[6] = mprun10;
    LEAF_1F2.data.u8[7] = calculate_checksum_nibble(&LEAF_1F2);

    transmit_can_frame(
        &LEAF_1F2,
        can_config.charger);  // Sending of 1F2 message is halted in LEAF-BATTERY function incase used here
  }

  /* Send messages every 100ms here */
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    mprun100 = (mprun100 + 1) % 4;  // mprun100 cycles between 0-1-2-3-0-1...

// Only send these messages if Nissan LEAF battery is not used
#ifndef NISSAN_LEAF_BATTERY

    LEAF_55B.data.u8[6] = ((0x1 << 4) | (mprun100));

    LEAF_55B.data.u8[7] = calculate_CRC_Nissan(&LEAF_55B);
    transmit_can_frame(&LEAF_55B, can_config.charger);

    transmit_can_frame(&LEAF_59E, can_config.charger);

    transmit_can_frame(&LEAF_5BC, can_config.charger);
#endif
  }
}
#endif
