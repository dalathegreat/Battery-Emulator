#include "SUNGROW-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

/* TODO: 
This protocol is still under development. It can not be used yet for Sungrow inverters, 
see the Wiki for more info on how to use your Sungrow inverter */

void SungrowInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the inverter CAN messages

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

void SungrowInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
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

void SungrowInverter::transmit_can(unsigned long currentMillis) {
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

void SungrowInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
