#include "SUNGROW-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

/* TODO: 
This protocol is still under development. It can not be used yet for Sungrow inverters, 
see the Wiki for more info on how to use your Sungrow inverter */

void SungrowInverter::update_values() {
  // Actual SoC
  SUNGROW_400.data.u8[1] = (datalayer.battery.status.real_soc & 0x00FF);
  SUNGROW_400.data.u8[2] = (datalayer.battery.status.real_soc >> 8);
  // Magic number
  SUNGROW_400.data.u8[3] = 0x01;

  // BMS init message
  SUNGROW_500.data.u8[0] = 0x01;                                           // Magic number
  SUNGROW_500.data.u8[1] = 0x01;                                           // Magic number
  SUNGROW_500.data.u8[2] = 0x03;                                           // Number of modules?
  SUNGROW_500.data.u8[3] = 0xFF;                                           // Magic number
  SUNGROW_500.data.u8[5] = 0x01;                                           // Magic number
  SUNGROW_500.data.u8[7] = (datalayer.battery.status.reported_soc / 100);  // SoC as a int

  //Maxvoltage (eg 400.0V = 4000 , 16bits long)
  SUNGROW_701.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  SUNGROW_701.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  //Minvoltage (eg 300.0V = 3000 , 16bits long)
  SUNGROW_701.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  SUNGROW_701.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  // Max Charging Current
  SUNGROW_701.data.u8[4] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  SUNGROW_701.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);
  // Max Discharging Current
  SUNGROW_701.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  SUNGROW_701.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);

  //SOC (100.0%)
  SUNGROW_702.data.u8[0] = (datalayer.battery.status.reported_soc & 0x00FF);
  SUNGROW_702.data.u8[1] = (datalayer.battery.status.reported_soc >> 8);
  //SOH (100.00%)
  SUNGROW_702.data.u8[2] = (datalayer.battery.status.soh_pptt & 0x00FF);
  SUNGROW_702.data.u8[3] = (datalayer.battery.status.soh_pptt >> 8);
  // Energy Remaining (Wh), clamped to 16-bit to avoid overflow on large packs
  remaining_wh = datalayer.battery.status.reported_remaining_capacity_Wh;
  if (remaining_wh > 0xFFFFu)
    remaining_wh = 0xFFFFu;
  SUNGROW_702.data.u8[4] = static_cast<uint8_t>(remaining_wh & 0x00FF);
  SUNGROW_702.data.u8[5] = static_cast<uint8_t>((remaining_wh >> 8) & 0x00FF);
  // Capacity max (Wh), clamped to 16-bit to avoid overflow on large packs
  capacity_wh = datalayer.battery.info.reported_total_capacity_Wh;
  if (capacity_wh > 0xFFFFu)
    capacity_wh = 0xFFFFu;
  SUNGROW_702.data.u8[6] = static_cast<uint8_t>(capacity_wh & 0x00FF);
  SUNGROW_702.data.u8[7] = static_cast<uint8_t>((capacity_wh >> 8) & 0x00FF);

  // Energy total charged (Wh)
  SUNGROW_703.data.u8[0] = (datalayer.battery.status.total_charged_battery_Wh & 0x00FF);
  SUNGROW_703.data.u8[1] = ((datalayer.battery.status.total_charged_battery_Wh >> 8) & 0x00FF);
  SUNGROW_703.data.u8[2] = ((datalayer.battery.status.total_charged_battery_Wh >> 16) & 0x00FF);
  SUNGROW_703.data.u8[3] = ((datalayer.battery.status.total_charged_battery_Wh >> 24) & 0x00FF);
  // Energy total discharged (Wh)
  SUNGROW_703.data.u8[4] = (datalayer.battery.status.total_discharged_battery_Wh & 0x00FF);
  SUNGROW_703.data.u8[5] = ((datalayer.battery.status.total_discharged_battery_Wh >> 8) & 0x00FF);
  SUNGROW_703.data.u8[6] = ((datalayer.battery.status.total_discharged_battery_Wh >> 16) & 0x00FF);
  SUNGROW_703.data.u8[7] = ((datalayer.battery.status.total_discharged_battery_Wh >> 24) & 0x00FF);

  //Vbat (eg 400.0V = 4000 , 16bits long)
  SUNGROW_704.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_704.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  // Current
  SUNGROW_704.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  SUNGROW_704.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  // Another voltage. Different but similar
  SUNGROW_704.data.u8[4] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_704.data.u8[5] = (datalayer.battery.status.voltage_dV >> 8);
  //Temperature //TODO: Signed correctly? Also should be put AVG here?
  SUNGROW_704.data.u8[6] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SUNGROW_704.data.u8[7] = (datalayer.battery.status.temperature_max_dC >> 8);

  //Status bytes?
  SUNGROW_705.data.u8[0] = 0x02;  // Magic number
  SUNGROW_705.data.u8[1] = 0x00;  // Magic number
  SUNGROW_705.data.u8[2] = 0x01;  // Magic number
  SUNGROW_705.data.u8[3] = 0xE7;  // Magic number
  SUNGROW_705.data.u8[4] = 0x20;  // Magic number
  //Vbat, again (eg 400.0V = 4000 , 16bits long)
  SUNGROW_705.data.u8[5] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_705.data.u8[6] = (datalayer.battery.status.voltage_dV >> 8);
  // Padding?
  SUNGROW_705.data.u8[7] = 0x00;  // Magic number

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

  // Battery Configuration
  SUNGROW_707.data.u8[0] = 0x26;                     // Magic number
  SUNGROW_707.data.u8[1] = 0x00;                     // Magic number
  SUNGROW_707.data.u8[2] = 0x00;                     // Magic number
  SUNGROW_707.data.u8[3] = 0x01;                     // Magic number. Num of stacks?
  SUNGROW_707.data.u8[4] = (nameplate_wh & 0x00FF);  // Nameplate capacity
  SUNGROW_707.data.u8[5] = (nameplate_wh >> 8);      // Nameplate capacity
  SUNGROW_707.data.u8[6] = 0x01;                     // Magic number. Num of modules?
  SUNGROW_707.data.u8[7] = 0x00;                     // Padding?

  // ---- Serial number (ASCII) → 0x008 mux 0/1 ----
  {
    const char* sn = serial_number;  // up to 12 chars
    SUNGROW_708_00.data.u8[0] = 0x00;
    for (int i = 0; i < 7; ++i) {
      SUNGROW_708_00.data.u8[1 + i] = sn[i] ? static_cast<uint8_t>(sn[i]) : 0x00;
    }
    SUNGROW_708_01.data.u8[0] = 0x01;
    for (int i = 0; i < 7; ++i) {
      SUNGROW_708_01.data.u8[1 + i] = sn[7 + i] ? static_cast<uint8_t>(sn[7 + i]) : 0x00;
    }
    SUNGROW_708_01.data.u8[7] = 0x53;
    // Example for "S2310131889":
    // 0x008 mux0 -> 00 53 32 33 31 30 31 33
    // 0x008 mux1 -> 01 31 38 38 39 00 00 53
  }

  // ---- 0x70F: two muxes (b0 = 0x00..0x07, b1 = 0x00) ----
  SUNGROW_70F_00.data.u8[2] = 0x88;  // Magic number
  SUNGROW_70F_00.data.u8[3] = 0x13;  // Magic number
  SUNGROW_70F_00.data.u8[4] = 0x80;  // Magic number
  SUNGROW_70F_00.data.u8[5] = 0x16;  // Magic number
  SUNGROW_70F_00.data.u8[6] = 0x80;  // Magic number
  SUNGROW_70F_00.data.u8[7] = 0x16;  // Magic number

  // Charge counter??
  SUNGROW_70F_02.data.u8[2] = 0x70;  // Magic number
  SUNGROW_70F_02.data.u8[3] = 0x20;  // Magic number
  // Unknown
  SUNGROW_70F_02.data.u8[6] = 0x92;  // Magic number
  SUNGROW_70F_02.data.u8[7] = 0x09;  // Magic number

  // Discharge counter??
  SUNGROW_70F_03.data.u8[2] = 0xFD;
  SUNGROW_70F_03.data.u8[3] = 0x1D;
  // Unknown
  SUNGROW_70F_03.data.u8[6] = 0xCE;
  SUNGROW_70F_03.data.u8[7] = 0x26;

  // Unknown
  SUNGROW_70F_04.data.u8[2] = 0x0C;
  SUNGROW_70F_04.data.u8[3] = 0x06;

  // Module 1 SoC
  SUNGROW_70F_05.data.u8[2] = (datalayer.battery.status.real_soc & 0xFF);
  SUNGROW_70F_05.data.u8[3] = (datalayer.battery.status.real_soc >> 8);
  // Module 2 SoC
  SUNGROW_70F_05.data.u8[4] = (datalayer.battery.status.real_soc & 0xFF);
  SUNGROW_70F_05.data.u8[5] = (datalayer.battery.status.real_soc >> 8);
  // Module 3 SoC
  SUNGROW_70F_05.data.u8[6] = (datalayer.battery.status.real_soc & 0xFF);
  SUNGROW_70F_05.data.u8[7] = (datalayer.battery.status.real_soc >> 8);

  //Status bytes?
  SUNGROW_713.data.u8[0] = 0x02;  // Magic number
  SUNGROW_713.data.u8[1] = 0x01;  // Magic number
  SUNGROW_713.data.u8[2] = 0x77;  // Magic number
  SUNGROW_713.data.u8[3] = 0x00;  // Magic number
  SUNGROW_713.data.u8[4] = 0x01;  // Magic number
  SUNGROW_713.data.u8[5] = 0x02;  // Magic number
  SUNGROW_713.data.u8[6] = 0x89;  // Magic number
  SUNGROW_713.data.u8[7] = 0x00;  // Magic number

  // Overview - Stack overview?
  SUNGROW_714.data.u8[0] = 0x05;  // Enum??
  SUNGROW_714.data.u8[1] = 0x01;  // Magic number
  // Cell Voltage
  SUNGROW_714.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_714.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  SUNGROW_714.data.u8[4] = 0x11;  // Enum???
  SUNGROW_714.data.u8[5] = 0x02;  // Magic number
  // Cell Voltage
  SUNGROW_714.data.u8[6] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SUNGROW_714.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  // Module 1 Min Cell voltage
  SUNGROW_715.data.u8[0] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[1] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  // Module 1 Max Cell voltage
  SUNGROW_715.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  // Module 2 Min Cell voltage
  SUNGROW_715.data.u8[4] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[5] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  // Module 2 Max Cell voltage
  SUNGROW_715.data.u8[6] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_715.data.u8[7] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

  // Module 3 Min Cell voltage
  SUNGROW_716.data.u8[0] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SUNGROW_716.data.u8[1] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  // Module 3 Max Cell voltage
  SUNGROW_716.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_716.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

  SUNGROW_717.data.u8[0] = 0x00;

  // Status flags?
  SUNGROW_719.data.u8[0] = 0x02;

  // Battery flags?
  SUNGROW_71A.data.u8[0] = 0x44;  // Magic number
  SUNGROW_71A.data.u8[1] = 0x44;  // Magic number
  SUNGROW_71A.data.u8[2] = 0x44;  // Magic number

  // Module 1 + 2 data?
  SUNGROW_71B.data.u8[0] = 0x3F;  // Magic number
  SUNGROW_71B.data.u8[1] = 0x7A;  // Magic number
  SUNGROW_71B.data.u8[2] = 0x6F;  // Magic number
  SUNGROW_71B.data.u8[3] = 0x01;  // Magic number
  SUNGROW_71B.data.u8[4] = 0x3F;  // Magic number
  SUNGROW_71B.data.u8[5] = 0x7A;  // Magic number
  SUNGROW_71B.data.u8[6] = 0x6F;  // Magic number
  SUNGROW_71B.data.u8[7] = 0x01;  // Magic number

  // Module 3 + 4 data?
  SUNGROW_71C.data.u8[0] = 0x3F;  // Magic number
  SUNGROW_71C.data.u8[1] = 0x7A;  // Magic number
  SUNGROW_71C.data.u8[2] = 0x6F;  // Magic number
  SUNGROW_71C.data.u8[3] = 0x01;  // Magic number

  //Copy 7## content to 0## messages
  for (int i = 0; i < 8; i++) {
    SUNGROW_001.data.u8[i] = SUNGROW_701.data.u8[i];
    SUNGROW_002.data.u8[i] = SUNGROW_702.data.u8[i];
    SUNGROW_003.data.u8[i] = SUNGROW_703.data.u8[i];
    SUNGROW_004.data.u8[i] = SUNGROW_704.data.u8[i];
    SUNGROW_005.data.u8[i] = SUNGROW_705.data.u8[i];
    SUNGROW_006.data.u8[i] = SUNGROW_706.data.u8[i];
    SUNGROW_008_00.data.u8[i] = SUNGROW_708_00.data.u8[i];
    SUNGROW_008_01.data.u8[i] = SUNGROW_708_01.data.u8[i];
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
  // 0x504 cannot be a straight copy: current must be signed (two's complement)
  {
    SUNGROW_504.data.u8[2] = (datalayer.battery.status.current_dA & 0xFF);
    SUNGROW_504.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  }

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
      //transmit_can_frame(&SUNGROW_001);
      //transmit_can_frame(&SUNGROW_002);
      //transmit_can_frame(&SUNGROW_003);
      //transmit_can_frame(&SUNGROW_004);
      //transmit_can_frame(&SUNGROW_005);
      //transmit_can_frame(&SUNGROW_006);
      //transmit_can_frame(&SUNGROW_013);
      //transmit_can_frame(&SUNGROW_014);
      //transmit_can_frame(&SUNGROW_015);
      //transmit_can_frame(&SUNGROW_016);
      //transmit_can_frame(&SUNGROW_017);
      //transmit_can_frame(&SUNGROW_018);
      //transmit_can_frame(&SUNGROW_019);
      //transmit_can_frame(&SUNGROW_01A);
      //transmit_can_frame(&SUNGROW_01B);
      //transmit_can_frame(&SUNGROW_01C);
      //transmit_can_frame(&SUNGROW_01D);
      //transmit_can_frame(&SUNGROW_01E);
      break;
    case 0x100:  // SH10RS RUN
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x101:  // Both SH10RS / SH15T
      // System time
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x102:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x103:  // 250ms - SH10RS init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        // Serial number byte1-7 (e.g. @23A229)
        version_char[0] = rx_frame.data.u8[1];
        version_char[1] = rx_frame.data.u8[2];
        version_char[2] = rx_frame.data.u8[3];
        version_char[3] = rx_frame.data.u8[4];
        version_char[4] = rx_frame.data.u8[5];
        version_char[5] = rx_frame.data.u8[6];
        version_char[6] = rx_frame.data.u8[7];
      }
      if (mux == 1) {
        // Serial number byte1-7 continued (e.g 2795)
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
    case 0x108:  // Discovery secret handshake accepted
      discovery_mode = false;
      break;
    case 0x191:  //Only sent by SH15T (Inverter trying to use BYD CAN)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      discovery_mode = true;  // We only see 0x191 when the inverter is searching for a battery.
                              // AU inverter sends this but does not want a BYD battery.
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
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    transmit_can_frame(&SUNGROW_500);
    transmit_can_frame(&SUNGROW_400);
    transmit_can_frame(&SUNGROW_401);

    //Flip flop between two sets, end result is 1s periodic rate
    if (alternate) {
      alternate = false;
    } else {
      alternate = true;
    }

    if (discovery_mode) {
      transmit_can_frame(&SUNGROW_007);
      transmit_can_frame(&SUNGROW_008_00);
      transmit_can_frame(&SUNGROW_008_01);
      transmit_can_frame(&SUNGROW_009);
      transmit_can_frame(&SUNGROW_00A_00);
      transmit_can_frame(&SUNGROW_00A_01);
      transmit_can_frame(&SUNGROW_00B);
      transmit_can_frame(&SUNGROW_00D);
      transmit_can_frame(&SUNGROW_00E);
    } else {
      // This is the for-real messages as observed
      transmit_can_frame(&SUNGROW_000);
      transmit_can_frame(&SUNGROW_001);
      transmit_can_frame(&SUNGROW_002);
      transmit_can_frame(&SUNGROW_003);
      transmit_can_frame(&SUNGROW_004);
      transmit_can_frame(&SUNGROW_005);
      transmit_can_frame(&SUNGROW_006);
      transmit_can_frame(&SUNGROW_013);
      transmit_can_frame(&SUNGROW_014);
      transmit_can_frame(&SUNGROW_015);
      transmit_can_frame(&SUNGROW_016);
      transmit_can_frame(&SUNGROW_017);
      transmit_can_frame(&SUNGROW_018);
      transmit_can_frame(&SUNGROW_019);
      transmit_can_frame(&SUNGROW_01A);
      transmit_can_frame(&SUNGROW_01B);
      transmit_can_frame(&SUNGROW_01C);
      transmit_can_frame(&SUNGROW_01D);
      transmit_can_frame(&SUNGROW_01E);
      transmit_can_frame(&SUNGROW_700);
      transmit_can_frame(&SUNGROW_701);
      transmit_can_frame(&SUNGROW_702);
      transmit_can_frame(&SUNGROW_703);
      transmit_can_frame(&SUNGROW_704);
      transmit_can_frame(&SUNGROW_705);
      transmit_can_frame(&SUNGROW_706);
      transmit_can_frame(&SUNGROW_713);
      transmit_can_frame(&SUNGROW_714);
      transmit_can_frame(&SUNGROW_715);
      transmit_can_frame(&SUNGROW_716);
      transmit_can_frame(&SUNGROW_717);
      transmit_can_frame(&SUNGROW_718);
      transmit_can_frame(&SUNGROW_719);
      transmit_can_frame(&SUNGROW_70F_00);
      transmit_can_frame(&SUNGROW_70F_01);
      transmit_can_frame(&SUNGROW_70F_02);
      transmit_can_frame(&SUNGROW_70F_03);
      transmit_can_frame(&SUNGROW_70F_04);
      transmit_can_frame(&SUNGROW_70F_05);
      transmit_can_frame(&SUNGROW_70F_06);
      transmit_can_frame(&SUNGROW_70F_07);
      // Every other time... Alternate??? Check timing.
      transmit_can_frame(&SUNGROW_71A);
      transmit_can_frame(&SUNGROW_71B);
      transmit_can_frame(&SUNGROW_71C);
      //transmit_can_frame(&SUNGROW_71D); // Not seen
      //transmit_can_frame(&SUNGROW_71E); // Not seen
    }
    transmit_can_frame(&SUNGROW_512);
    transmit_can_frame(&SUNGROW_501);
    transmit_can_frame(&SUNGROW_502);
    transmit_can_frame(&SUNGROW_503);
    transmit_can_frame(&SUNGROW_504);
    transmit_can_frame(&SUNGROW_505);
    transmit_can_frame(&SUNGROW_506);
  }

  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;
    transmit_can_frame(&SUNGROW_707);
    transmit_can_frame(&SUNGROW_708_00);
    transmit_can_frame(&SUNGROW_708_01);
    transmit_can_frame(&SUNGROW_709);
    transmit_can_frame(&SUNGROW_70A_00);
    transmit_can_frame(&SUNGROW_70A_01);
    transmit_can_frame(&SUNGROW_70B);
    transmit_can_frame(&SUNGROW_70D);
    transmit_can_frame(&SUNGROW_70E);
  }

  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;
    // 0x71F: module serial numbers (3 modules × 3 chunks of 6 ASCII bytes)
    for (int mod = 0; mod < 3; ++mod) {
      const char* sn = module_serial[mod];  // up to 18 chars
      for (int chunk = 0; chunk < 3; ++chunk) {
        SUNGROW_71F.data.u8[0] = static_cast<uint8_t>(mod + 1);    // module index 1..3
        SUNGROW_71F.data.u8[1] = static_cast<uint8_t>(chunk + 1);  // chunk index 1..3
        const int base = chunk * 6;
        for (int i = 0; i < 6; ++i) {
          char ch = sn[base + i];
          SUNGROW_71F.data.u8[2 + i] = ch ? static_cast<uint8_t>(ch) : 0x00;
        }
        transmit_can_frame(&SUNGROW_71F);
      }
    }
  }
}
