#include "CHEVY-VOLT-CHARGER.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/logging.h"

/* This implements Chevy Volt / Ampera charger support (2011-2015 model years).
 *
 * This code is intended to facilitate battery charging while repurposing inverters
 *  that lack embedded charging features, to facilitate standalone charging, etc.
 *
 * Key influence and inspiration informed by prior work by those such as:
 *   Damien Maguire (evmbw.org, github.com/damienmaguire/AmperaCharger)
 *   Tom deBree, Arber Kramer, Colin Kidder, EVTV, etc
 * Various implementation details aided by openinverter forum discussion/CAN logs
 *
 * It is very likely that Lear charger support could be added to this with minimal effort
 *  (similar hardware, different firmware and CAN messages).
 *
 * 2024 smaresca
 */

/* We are mostly sending out not receiving */
void ChevyVoltCharger::map_can_frame_to_variable(CAN_frame rx_frame) {
  uint16_t charger_stat_HVcur_temp = 0;
  uint16_t charger_stat_HVvol_temp = 0;
  uint16_t charger_stat_LVcur_temp = 0;
  uint16_t charger_stat_LVvol_temp = 0;
  uint16_t charger_stat_ACcur_temp = 0;
  uint16_t charger_stat_ACvol_temp = 0;

  switch (rx_frame.ID) {
    //ID 0x212 conveys instantaneous DC charger stats
    case 0x212:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      charger_stat_HVcur_temp = (uint16_t)(rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1]);
      datalayer.charger.charger_stat_HVcur = (float)(charger_stat_HVcur_temp >> 3) * 0.05;

      charger_stat_HVvol_temp = (uint16_t)((((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[2])) >> 1) & 0x3ff);
      datalayer.charger.charger_stat_HVvol = (float)(charger_stat_HVvol_temp) * .5;

      charger_stat_LVcur_temp = (uint16_t)(((rx_frame.data.u8[2] << 8 | rx_frame.data.u8[3]) >> 1) & 0x00ff);
      datalayer.charger.charger_stat_LVcur = (float)(charger_stat_LVcur_temp) * .2;

      charger_stat_LVvol_temp = (uint16_t)(((rx_frame.data.u8[3] << 8 | rx_frame.data.u8[4]) >> 1) & 0x00ff);
      datalayer.charger.charger_stat_LVvol = (float)(charger_stat_LVvol_temp) * .1;

      break;

    //ID 0x30A conveys instantaneous AC charger stats
    case 0x30A:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      charger_stat_ACcur_temp = (uint16_t)((rx_frame.data.u8[0] << 8 | rx_frame.data.u8[1]) >> 4);
      datalayer.charger.charger_stat_ACcur = (float)(charger_stat_ACcur_temp) * 0.2;

      charger_stat_ACvol_temp = (uint16_t)(((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[2]) >> 4) & 0x00ff);
      datalayer.charger.charger_stat_ACvol = (float)(charger_stat_ACvol_temp) * 2;

      break;

    //ID 0x266, 0x268, and 0x308 are regularly emitted by the charger but content is unknown
    // 0x266 and 0x308 are len 5
    // 0x268 may be temperature data (len 8). Could resemble the Lear charger equivalent TODO
    case 0x266:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      break;
    case 0x268:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      break;
    case 0x308:
      datalayer.charger.CAN_charger_still_alive = CAN_STILL_ALIVE;  // Let system know charger is sending CAN
      break;
    default:
#ifdef DEBUG_LOG
      logging.printf("CAN Rcv unknown frame MsgID=%x\n", rx_frame.ID);
#endif
      break;
  }
}

void ChevyVoltCharger::transmit_can(unsigned long currentMillis) {
  uint16_t Vol_temp = 0;

  uint16_t setpoint_HV_VDC = floor(datalayer.charger.charger_setpoint_HV_VDC);
  uint16_t setpoint_HV_IDC = floor(datalayer.charger.charger_setpoint_HV_IDC);
  uint16_t setpoint_HV_IDC_END = floor(datalayer.charger.charger_setpoint_HV_IDC_END);
  uint8_t charger_mode = MODE_DISABLED;

  /* Send keepalive with mode every 30ms */
  if (currentMillis - previousMillis30ms >= INTERVAL_30_MS) {
    previousMillis30ms = currentMillis;

    if (datalayer.charger.charger_HV_enabled) {
      charger_mode += MODE_HV;

      /* disable HV if end amperage reached
       * TODO - integration opportunity with battery/inverter code
      if (setpoint_HV_IDC_END > 0 && charger_stat_HVcur > setpoint_HV_IDC_END) {
        charger_mode -= MODE_HV;
      }
      */
    }

    if (datalayer.charger.charger_aux12V_enabled)
      charger_mode += MODE_LV;

    charger_keepalive_frame.data.u8[0] = charger_mode;

    transmit_can_frame(&charger_keepalive_frame);
  }

  /* Send current targets every 200ms */
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    /* These values should be and are validated elsewhere, but adjust if needed
     * to stay within limits of hardware and user-supplied settings
     */
    if (setpoint_HV_VDC > CHARGER_MAX_HV) {
      setpoint_HV_VDC = CHEVYVOLT_MAX_HVDC;
    }

    if (setpoint_HV_VDC < CHARGER_MIN_HV && setpoint_HV_VDC > 0) {
      setpoint_HV_VDC = CHEVYVOLT_MIN_HVDC;
    }

    if (setpoint_HV_IDC > CHARGER_MAX_A) {
      setpoint_HV_VDC = CHEVYVOLT_MAX_AMP;
    }

    /* if power overcommitted, back down to just below while maintaining voltage target */
    if (setpoint_HV_IDC * setpoint_HV_VDC > CHARGER_MAX_POWER) {
      setpoint_HV_IDC = floor(CHARGER_MAX_POWER / setpoint_HV_VDC);
    }

    /* current setting */
    charger_set_targets.data.u8[1] = setpoint_HV_IDC * 20;
    Vol_temp = setpoint_HV_VDC * 2;

    /* first 2 bits are MSB of the voltage command */
    charger_set_targets.data.u8[2] = highByte(Vol_temp);

    /* LSB of the voltage command. Then MSB LSB is divided by 2 */
    charger_set_targets.data.u8[3] = lowByte(Vol_temp);

    transmit_can_frame(&charger_set_targets);
  }

#ifdef DEBUG_LOG
  /* Serial echo every 5s of charger stats */
  if (currentMillis - previousMillis5000ms >= INTERVAL_5_S) {
    previousMillis5000ms = currentMillis;
    logging.printf("Charger AC in IAC=%fA VAC=%fV\n", AC_input_current(), AC_input_voltage());
    logging.printf("Charger HV out IDC=%fA VDC=%fV\n", HVDC_output_current(), HVDC_output_voltage());
    logging.printf("Charger LV out IDC=%fA VDC=%fV\n", LVDC_output_current(), LVDC_output_voltage());
    logging.printf("Charger mode=%s\n", (charger_mode > MODE_DISABLED) ? "Enabled" : "Disabled");
    logging.printf("Charger HVset=%uV,%uA finishCurrent=%uA\n", setpoint_HV_VDC, setpoint_HV_IDC, setpoint_HV_IDC_END);
  }
#endif
}
