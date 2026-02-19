#include "NISSAN-LEAF-CHARGER.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "CHARGERS.h"

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

static uint8_t calculate_CRC_Nissan(CAN_frame* frame) {
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable_nissan_leaf[(crc ^ static_cast<uint8_t>(frame->data.u8[j])) % 256];
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

void NissanLeafCharger::map_can_frame_to_variable(CAN_frame rx_frame) {

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
      if ((OBC_Charge_Status == PLUGGED_IN_WAITING_ON_TIMER) || (OBC_Charge_Status == CHARGING_OR_INTERRUPTED)) {
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

void NissanLeafCharger::transmit_can(unsigned long currentMillis) {

  /* Send keepalive with mode every 10ms */
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...

/* 1DB is the main control message. If LEAF battery is used, the battery controls almost everything */
// Only send these messages if Nissan LEAF battery is not used
#ifndef NISSAN_LEAF_BATTERY

    // VCM message, containing info if battery should sleep or stay awake
    transmit_can_frame(&LEAF_50B);  // HCM_WakeUpSleepCommand == 11b == WakeUp, and CANMASK = 1

    LEAF_1DB.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DB);
    transmit_can_frame(&LEAF_1DB);

    LEAF_1DC.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DC);
    transmit_can_frame(&LEAF_1DC);
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

    transmit_can_frame(&LEAF_1F2);  // Sending of 1F2 message is halted in LEAF-BATTERY function incase used here
  }

  /* Send messages every 100ms here */
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    mprun100 = (mprun100 + 1) % 4;  // mprun100 cycles between 0-1-2-3-0-1...

// Only send these messages if Nissan LEAF battery is not used
#ifndef NISSAN_LEAF_BATTERY

    LEAF_55B.data.u8[6] = ((0x1 << 4) | (mprun100));

    LEAF_55B.data.u8[7] = calculate_CRC_Nissan(&LEAF_55B);
    transmit_can_frame(&LEAF_55B);

    transmit_can_frame(&LEAF_59E);

    transmit_can_frame(&LEAF_5BC);
#endif
  }
}
