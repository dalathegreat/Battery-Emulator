#include "SUNGROW-CAN.h"
#include <Arduino.h>
#include <cstring>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

/* TODO: 
This protocol is still under development. It can not be used yet for Sungrow inverters, 
see the Wiki for more info on how to use your Sungrow inverter */

namespace {
template <typename T, size_t N>
constexpr size_t array_len(const T (&)[N]) {
  return N;
}

// Some embedded libstdc++ variants don't provide std::strnlen.
// Provide a tiny local, constexpr-friendly replacement.
static inline size_t strnlen_compat(const char* s, size_t maxlen) {
  if (!s) {
    return 0;
  }
  size_t i = 0;
  while (i < maxlen && s && s[i]) {
    ++i;
  }
  return i;
}

template <size_t N>
static inline void copy_ascii(uint8_t (&dst)[N], const char* src, size_t visible_max = N > 0 ? N - 1 : 0) {
  std::memset(dst, 0, N);
  if (!src || N == 0) {
    return;
  }

  const size_t max_visible = (visible_max < N) ? visible_max : N;
  const size_t n = strnlen_compat(src, max_visible);

  for (size_t i = 0; i < n; ++i) {
    unsigned char c = static_cast<unsigned char>(src[i]);
    dst[i] = (c >= 0x20 && c <= 0x7E) ? c : static_cast<uint8_t>('0');
  }
}

template <size_t N>
static inline void append_seq(CAN_frame** out, uint16_t& len, CAN_frame* const (&in)[N]) {
  std::memcpy(out + len, in, N * sizeof(CAN_frame*));
  len += static_cast<uint16_t>(N);
}
}  // namespace

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
  SUNGROW_707.data.u8[4] = (NAMEPLATE_WH & 0x00FF);  // Nameplate capacity
  SUNGROW_707.data.u8[5] = (NAMEPLATE_WH >> 8);      // Nameplate capacity
  SUNGROW_707.data.u8[6] = 0x01;                     // Magic number. Num of modules?
  SUNGROW_707.data.u8[7] = 0x00;                     // Padding?

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
  SUNGROW_714.data.u8[2] = ((datalayer.battery.status.cell_max_voltage_mV * 10) & 0x00FF);
  SUNGROW_714.data.u8[3] = ((datalayer.battery.status.cell_max_voltage_mV * 10) >> 8);
  SUNGROW_714.data.u8[4] = 0x11;  // Enum???
  SUNGROW_714.data.u8[5] = 0x02;  // Magic number
  // Cell Voltage
  SUNGROW_714.data.u8[6] = ((datalayer.battery.status.cell_min_voltage_mV * 10) & 0x00FF);
  SUNGROW_714.data.u8[7] = ((datalayer.battery.status.cell_min_voltage_mV * 10) >> 8);

  // Module 1 Min Cell voltage
  SUNGROW_715.data.u8[0] = ((datalayer.battery.status.cell_min_voltage_mV * 10) & 0x00FF);
  SUNGROW_715.data.u8[1] = ((datalayer.battery.status.cell_min_voltage_mV * 10) >> 8);
  // Module 1 Max Cell voltage
  SUNGROW_715.data.u8[2] = ((datalayer.battery.status.cell_max_voltage_mV * 10) & 0x00FF);
  SUNGROW_715.data.u8[3] = ((datalayer.battery.status.cell_max_voltage_mV * 10) >> 8);
  // Module 2 Min Cell voltage
  SUNGROW_715.data.u8[4] = ((datalayer.battery.status.cell_min_voltage_mV * 10) & 0x00FF);
  SUNGROW_715.data.u8[5] = ((datalayer.battery.status.cell_min_voltage_mV * 10) >> 8);
  // Module 2 Max Cell voltage
  SUNGROW_715.data.u8[6] = ((datalayer.battery.status.cell_max_voltage_mV * 10) & 0x00FF);
  SUNGROW_715.data.u8[7] = ((datalayer.battery.status.cell_max_voltage_mV * 10) >> 8);

  // Module 3 Min Cell voltage
  SUNGROW_716.data.u8[0] = ((datalayer.battery.status.cell_min_voltage_mV * 10) & 0x00FF);
  SUNGROW_716.data.u8[1] = ((datalayer.battery.status.cell_min_voltage_mV * 10) >> 8);
  // Module 3 Max Cell voltage
  SUNGROW_716.data.u8[2] = ((datalayer.battery.status.cell_max_voltage_mV * 10) & 0x00FF);
  SUNGROW_716.data.u8[3] = ((datalayer.battery.status.cell_max_voltage_mV * 10) >> 8);

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
  switch (rx_frame.ID) {
    case 0x100:
      // SH10RS RUN
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x101:
      // SH10RS init @ 13,000ms (SH15T as well?)
      // System time as epoch in b0->b3
      // b4->b7 all 0x00
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      discovery_mode = true;
      break;
    case 0x102:
      // SH10RS init @ 13,000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x103:
      // SH10RS init @ 13,000ms
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
    case 0x104:
      // SH10RS init @ 13,000ms
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
    case 0x105:
      // SH10RS init @ 13,000ms
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
    case 0x106:
      // SH10RS init @ 13,000ms, or
      // SH10RS run @ 250ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x108:
      // SH10RS run @ 250ms
      discovery_mode = false;
      break;
    case 0x109:
      // SH10RS run @ 250ms
      discovery_mode = false;
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
  // ---- constants (local to function to keep header clean) ----
  static constexpr uint8_t SLICE_BUDGET = 16;  // ≤ 50% of 32-deep TX FIFO

  // init on first call
  if (epochStartMs == 0) {
    lastDiscoveryMode = discovery_mode;
    epochStartMs = currentMillis;
    lastSliceMs = currentMillis;
    // align overlay timers
    previousMillis1_5s = currentMillis;
    previousMillis10s = currentMillis;
    previousMillis60s = currentMillis;
  }

  if ((currentMillis - epochStartMs) >= INTERVAL_1_S) {
    epochStartMs += INTERVAL_1_S;  // keep phase steady
    cursor = 0;
  }

  // if discovery mode toggled mid-epoch, restart that list cleanly
  if (lastDiscoveryMode != discovery_mode) {
    lastDiscoveryMode = discovery_mode;
    cursor = 0;
  }

  // only act once per time slice
  if ((currentMillis - lastSliceMs) < INTERVAL_50_MS) {
    return;
  }

  lastSliceMs = currentMillis;

  // CAN sequence chunks
  static CAN_frame* const SEQ_HEAD[] = {&SUNGROW_500, &SUNGROW_400, &SUNGROW_401};

  static CAN_frame* const SEQ_TAIL[] = {&SUNGROW_512, &SUNGROW_501, &SUNGROW_502, &SUNGROW_503,
                                        &SUNGROW_504, &SUNGROW_505, &SUNGROW_506};

  static CAN_frame* const SEQ_RUN[] = {
      &SUNGROW_000,    &SUNGROW_001,    &SUNGROW_002,    &SUNGROW_003,    &SUNGROW_004,    &SUNGROW_005,
      &SUNGROW_006,    &SUNGROW_013,    &SUNGROW_014,    &SUNGROW_015,    &SUNGROW_016,    &SUNGROW_017,
      &SUNGROW_018,    &SUNGROW_019,    &SUNGROW_01A,    &SUNGROW_01B,    &SUNGROW_01C,    &SUNGROW_01D,
      &SUNGROW_01E,    &SUNGROW_700,    &SUNGROW_701,    &SUNGROW_702,    &SUNGROW_703,    &SUNGROW_704,
      &SUNGROW_705,    &SUNGROW_706,    &SUNGROW_713,    &SUNGROW_714,    &SUNGROW_715,    &SUNGROW_716,
      &SUNGROW_717,    &SUNGROW_718,    &SUNGROW_719,    &SUNGROW_70F_00, &SUNGROW_70F_01, &SUNGROW_70F_02,
      &SUNGROW_70F_03, &SUNGROW_70F_04, &SUNGROW_70F_05, &SUNGROW_70F_06, &SUNGROW_70F_07, &SUNGROW_71A,
      &SUNGROW_71B,    &SUNGROW_71C};

  static CAN_frame* const SEQ_DISCOVERY[] = {
      &SUNGROW_007,    &SUNGROW_008_00, &SUNGROW_008_01, &SUNGROW_009, &SUNGROW_00A_00,
      &SUNGROW_00A_01, &SUNGROW_00B,    &SUNGROW_00D,    &SUNGROW_00E,
  };

  static CAN_frame* const SEQ_10S_RUN[] = {&SUNGROW_707, &SUNGROW_708_00, &SUNGROW_708_01,
                                           &SUNGROW_709, &SUNGROW_70A_00, &SUNGROW_70A_01,
                                           &SUNGROW_70B, &SUNGROW_70D,    &SUNGROW_70E};

  static const uint8_t LEN_10S_RUN = (uint8_t)array_len(SEQ_10S_RUN);

  static CAN_frame* seq[SEQ_CAP];
  static bool seq_built = false;
  static bool seq_is_discovery = false;
  static uint16_t seq_len = 0;

  const bool want_discovery = discovery_mode;

  if (!seq_built || seq_is_discovery != want_discovery) {
    uint16_t LEN = 0;
    append_seq(seq, LEN, SEQ_HEAD);
    if (want_discovery) {
      append_seq(seq, LEN, SEQ_DISCOVERY);
    } else {
      append_seq(seq, LEN, SEQ_RUN);
    }
    append_seq(seq, LEN, SEQ_TAIL);

    static_assert(array_len(SEQ_HEAD) + array_len(SEQ_RUN) + array_len(SEQ_TAIL) <= SEQ_CAP,
                  "SEQ_CAP too small for RUN");
    static_assert(array_len(SEQ_HEAD) + array_len(SEQ_DISCOVERY) + array_len(SEQ_TAIL) <= SEQ_CAP,
                  "SEQ_CAP too small for DISCOVERY");

    seq_len = LEN;
    seq_is_discovery = want_discovery;
    seq_built = true;

    // Reset cursor when the mode flips
    if (lastDiscoveryMode != want_discovery)
      cursor = 0;
  }

  // 1,000ms messages
  uint8_t sent = 0;
  while (cursor < seq_len && sent < SLICE_BUDGET) {
    transmit_can_frame(seq[cursor++]);
    ++sent;
  }

  // overlays — only when not in discovery
  if (!discovery_mode) {
    // ---- 1,500ms messages ----
    if ((int32_t)(currentMillis - previousMillis1_5s) >= 0 && (SLICE_BUDGET - sent) >= 3) {
      transmit_can_frame(&SUNGROW_1E0_00);
      transmit_can_frame(&SUNGROW_1E0_01);
      transmit_can_frame(&SUNGROW_1E0_02);
      sent += 3;
      do {
        previousMillis1_5s += INTERVAL_1_5_S;
      } while ((int32_t)(currentMillis - previousMillis1_5s) >= 0);
    }

    // ---- 10,000ms messages ----
    if ((int32_t)(currentMillis - previousMillis10s) >= 0) {
      g10_pending = true;
      g10_idx = 0;
      do {
        previousMillis10s += INTERVAL_10_S;
      } while ((int32_t)(currentMillis - previousMillis10s) >= 0);
    }

    // ---- drain 10s group within slice budget ----
    if (g10_pending && (SLICE_BUDGET - sent) > 0) {
      while (g10_idx < LEN_10S_RUN && sent < SLICE_BUDGET) {
        transmit_can_frame(SEQ_10S_RUN[g10_idx++]);
        ++sent;
      }
      if (g10_idx >= LEN_10S_RUN) {
        g10_pending = false;
      }
    }

    // ---- 60,000ms messages ----
    if ((int32_t)(currentMillis - previousMillis60s) >= 0) {
      g60_pending = true;
      g60_idx = 0;
      do {
        previousMillis60s += INTERVAL_60_S;
      } while ((int32_t)(currentMillis - previousMillis60s) >= 0);
    }

    // ---- drain 60s group within slice budget ----
    if (g60_pending && (SLICE_BUDGET - sent) > 0) {
      while (g60_pending && sent < SLICE_BUDGET) {
        const uint8_t mod = (uint8_t)(g60_idx / 3);  // 0..2
        const uint8_t ch = (uint8_t)(g60_idx % 3);   // 0..2
        transmit_can_frame(&SUNGROW_71F_MC[mod][ch]);
        ++g60_idx;
        ++sent;
        if (g60_idx >= 9)
          g60_pending = false;
      }
    }
  }
}

void SungrowInverter::rebuild_serial_frames() {
  // Build 0x708 mux 0
  SUNGROW_708_00.DLC = 8;
  SUNGROW_708_00.ID = 0x708;
  SUNGROW_708_00.ext_ID = false;
  SUNGROW_708_00.FD = false;
  SUNGROW_708_00.data.u8[0] = 0x00;
  for (int i = 0; i < 7; ++i) {
    SUNGROW_708_00.data.u8[1 + i] = serial_number_char[i];
  }
  // Build 0x708 mux 1
  SUNGROW_708_01.DLC = 8;
  SUNGROW_708_01.ID = 0x708;
  SUNGROW_708_01.ext_ID = false;
  SUNGROW_708_01.FD = false;
  SUNGROW_708_01.data.u8[0] = 0x01;

  // bytes 7..12 (six bytes), last byte (index 7) is sentinel
  for (int i = 0; i < 6; ++i) {
    SUNGROW_708_01.data.u8[1 + i] = serial_number_char[7 + i];
  }

  SUNGROW_708_01.data.u8[7] = 0x53;  // observed trailing sentinel
}

void SungrowInverter::rebuild_module_serial_frames() {
  // Build 0x71F frames for 3 modules × 3 chunks (6 bytes/chunk)
  for (int mod = 0; mod < 3; ++mod) {
    for (int chunk = 0; chunk < 3; ++chunk) {
      CAN_frame& f = SUNGROW_71F_MC[mod][chunk];
      f.DLC = 8;
      f.ID = 0x71F;
      f.ext_ID = false;
      f.FD = false;
      std::memset(f.data.u8, 0, 8);
      f.data.u8[0] = static_cast<uint8_t>(mod + 1);    // module index 1..3
      f.data.u8[1] = static_cast<uint8_t>(chunk + 1);  // chunk 1..3
      const int base = chunk * 6;                      // 0, 6, 12
      for (int i = 0; i < 6; ++i) {
        f.data.u8[2 + i] = module_serial_char[mod][base + i];  // 6 bytes slice
      }
    }
  }
}

bool SungrowInverter::setup() {
  // 10-digit unique from eFuse MAC
  uint64_t mac = ESP.getEfuseMac();
  uint32_t folded = (uint32_t)((mac >> 32) ^ (mac & 0xFFFFFFFFULL));
  uint64_t unique10 = (uint64_t)folded % 10000000000ULL;  // 0..9,999,999,999

  char uid10[11];
  snprintf(uid10, sizeof(uid10), "%010llu", (unsigned long long)unique10);

  // --- Serial number: "S" + 10-digit (e.g., S1234567890) ---
  char serial_buf[16];
  snprintf(serial_buf, sizeof(serial_buf), "S%s", uid10);
  copy_ascii(serial_number_char, serial_buf);
  rebuild_serial_frames();

  // --- Module serials: "EM032D" + 10-digit (last digit + i) + "DF" ---
  for (int i = 0; i < 3; ++i) {
    char mod_digits[11];
    memcpy(mod_digits, uid10, 10);
    mod_digits[10] = '\0';
    // bump last digit by i (wrap 0-9)
    mod_digits[9] = char(((mod_digits[9] - '0' + i) % 10) + '0');

    char module_buf[32];
    snprintf(module_buf, sizeof(module_buf), "EM032D%sDF", mod_digits);
    copy_ascii(module_serial_char[i], module_buf);
  }
  rebuild_module_serial_frames();

  return true;
}
