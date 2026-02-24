#include "SUNGROW-CAN.h"

#include <cstring>

#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "INVERTERS.h"

/* TODO: 
This protocol is still under development and should be considered beta quality.
It can be used with caution.
see the Wiki for more info on how to use your Sungrow inverter */

/*
Device Model: SBR096
Current Version: SBRBCU-S_22011.01.26
Battery Protocol: AA10
Battery Protocol Version: V1.0.6

Device S/N: S2310123456
S/N of Module 1: EM032D2310123461DF
S/N of Module 2: EM032D2310123462DF
S/N of Module 3: EM032D2310123463DF
*/

// ---- Internal Modbus helper namespace for SBRXXX threshold block ----
namespace {

// Modbus RTU constants for the SBRXXX battery on CAN ID 0x1E0.

// SOC thresholds in percent * 100 (pptt).
constexpr uint16_t MODBUS_SOC_CUTOFF_PPTT = 500;        // 5.00 %
constexpr uint16_t MODBUS_SOC_EMERGENCY_PPTT = 300;     // 3.00 %
constexpr uint16_t MODBUS_SOC_IDLE_TRIGGER_PPTT = 200;  // 2.00 %

// Modbus function codes we care about.
constexpr uint8_t MODBUS_FUNC_READ_HOLDING_REGS = 0x03;
constexpr uint8_t MODBUS_FUNC_READ_INPUT_REGS = 0x04;

// Backing store for 6 registers starting at 0x4DE2:
//   Reg0: cutoff SOC of discharge (pptt)
//   Reg1: reserved
//   Reg2: emergency charging SOC (pptt)
//   Reg3: reserved
//   Reg4: trigger SOC of idle mode (pptt)
//   Reg5: reserved
constexpr uint16_t MODBUS_REGISTER_VALUES[SungrowInverter::MODBUS_REGISTER_QTY] = {
    MODBUS_SOC_CUTOFF_PPTT, 0, MODBUS_SOC_EMERGENCY_PPTT, 0, MODBUS_SOC_IDLE_TRIGGER_PPTT, 0};

// Maximum number of Modbus data bytes based on the virtual register window.
constexpr uint8_t MODBUS_MAX_DATA_BYTES = static_cast<uint8_t>(SungrowInverter::MODBUS_REGISTER_QTY * 2);

// Minimal Modbus RTU CRC16 (poly 0xA001, little-endian output).
uint16_t modbus_crc(const uint8_t* data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

}  // namespace

bool SungrowInverter::setup() {
  // Stored value is model (0-6), get_config_for_model handles default
  battery_config = get_config_for_model(user_selected_inverter_sungrow_type);

  // Cell type for each active module
  // 0x42 = IEC
  // 0x44 = Non-IEC
  for (uint8_t i = 0; i < battery_config.module_count && i < 8; i++) {
    SUNGROW_71A.data.u8[i] = 0x44;
  }

  // Production date for active modules
  // 0x71B: modules 1+2, 0x71C: modules 3+4, 0x71D: modules 5+6, 0x71E: modules 7+8
  const uint8_t prod_date[4] = {0x3F, 0x7A, 0x6F, 0x01};
  CAN_frame* prod_date_frames[4] = {&SUNGROW_71B, &SUNGROW_71C, &SUNGROW_71D, &SUNGROW_71E};
  for (uint8_t m = 0; m < battery_config.module_count && m < 8; m++) {
    uint8_t frame_idx = m / 2;     // 0-1->0, 2-3->1, 4-5->2, 6-7->3
    uint8_t offset = (m % 2) * 4;  // even->0, odd->4
    memcpy(&prod_date_frames[frame_idx]->data.u8[offset], prod_date, 4);
  }

  // Serial numbers: base serial with incrementing module number
  // Format: "EM03XY" + "231012" + "346NDF" where X=cell type, N=module number
  // Fragment 1 byte 7 = cell type (same as 0x71A value)
  CAN_frame* serial_frames[8][3] = {{&SUNGROW_71F_01_01, &SUNGROW_71F_01_02, &SUNGROW_71F_01_03},
                                    {&SUNGROW_71F_02_01, &SUNGROW_71F_02_02, &SUNGROW_71F_02_03},
                                    {&SUNGROW_71F_03_01, &SUNGROW_71F_03_02, &SUNGROW_71F_03_03},
                                    {&SUNGROW_71F_04_01, &SUNGROW_71F_04_02, &SUNGROW_71F_04_03},
                                    {&SUNGROW_71F_05_01, &SUNGROW_71F_05_02, &SUNGROW_71F_05_03},
                                    {&SUNGROW_71F_06_01, &SUNGROW_71F_06_02, &SUNGROW_71F_06_03},
                                    {&SUNGROW_71F_07_01, &SUNGROW_71F_07_02, &SUNGROW_71F_07_03},
                                    {&SUNGROW_71F_08_01, &SUNGROW_71F_08_02, &SUNGROW_71F_08_03}};

  for (uint8_t m = 0; m < battery_config.module_count && m < 8; m++) {
    // Fragment 1: "EM032" + cell_type (byte 7 special)
    serial_frames[m][0]->data.u8[2] = 0x45;                    // 'E'
    serial_frames[m][0]->data.u8[3] = 0x4D;                    // 'M'
    serial_frames[m][0]->data.u8[4] = 0x30;                    // '0'
    serial_frames[m][0]->data.u8[5] = 0x33;                    // '3'
    serial_frames[m][0]->data.u8[6] = 0x32;                    // '2'
    serial_frames[m][0]->data.u8[7] = SUNGROW_71A.data.u8[m];  // cell type

    // Fragment 2: "231012"
    serial_frames[m][1]->data.u8[2] = 0x32;  // '2'
    serial_frames[m][1]->data.u8[3] = 0x33;  // '3'
    serial_frames[m][1]->data.u8[4] = 0x31;  // '1'
    serial_frames[m][1]->data.u8[5] = 0x30;  // '0'
    serial_frames[m][1]->data.u8[6] = 0x31;  // '1'
    serial_frames[m][1]->data.u8[7] = 0x32;  // '2'

    // Fragment 3: "346NDF" where N = module number ('1'-'8')
    serial_frames[m][2]->data.u8[2] = 0x33;     // '3'
    serial_frames[m][2]->data.u8[3] = 0x34;     // '4'
    serial_frames[m][2]->data.u8[4] = 0x36;     // '6'
    serial_frames[m][2]->data.u8[5] = '1' + m;  // '1', '2', '3', ... '8'
    serial_frames[m][2]->data.u8[6] = 0x44;     // 'D'
    serial_frames[m][2]->data.u8[7] = 0x46;     // 'F'
  }

  return true;
}

void SungrowInverter::update_values() {
  current_dA = datalayer.battery.status.reported_current_dA;

  // Actual SoC
  SUNGROW_400.data.u8[1] = (datalayer.battery.status.real_soc & 0x00FF);
  SUNGROW_400.data.u8[2] = (datalayer.battery.status.real_soc >> 8);
  // Magic number
  SUNGROW_400.data.u8[3] = 0x01;

  // BMS init message
  SUNGROW_500.data.u8[0] = 0x01;                                           // Magic number
  SUNGROW_500.data.u8[1] = 0x01;                                           // Magic number
  SUNGROW_500.data.u8[2] = 0x03;                                           // Magic number
  SUNGROW_500.data.u8[3] = 0xFF;                                           // Magic number
  SUNGROW_500.data.u8[5] = 0x01;                                           // Magic number
  SUNGROW_500.data.u8[7] = (datalayer.battery.status.reported_soc / 100);  // SoC as a int

  // Max voltage (eg 400.0V = 4000 , 16bits long)
  SUNGROW_701.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  SUNGROW_701.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  // Min voltage (eg 300.0V = 3000 , 16bits long)
  SUNGROW_701.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  SUNGROW_701.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  // Max Charging Current
  SUNGROW_701.data.u8[4] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  SUNGROW_701.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);
  // Max Discharging Current
  SUNGROW_701.data.u8[6] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  SUNGROW_701.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);

  // SOC (100.0%)
  SUNGROW_702.data.u8[0] = (datalayer.battery.status.reported_soc & 0x00FF);
  SUNGROW_702.data.u8[1] = (datalayer.battery.status.reported_soc >> 8);
  // SOH (100.00%)
  SUNGROW_702.data.u8[2] = (datalayer.battery.status.soh_pptt & 0x00FF);
  SUNGROW_702.data.u8[3] = (datalayer.battery.status.soh_pptt >> 8);
  // Energy Remaining (Wh), clamped to 16-bit to avoid overflow on large packs
  remaining_wh = datalayer.battery.status.reported_remaining_capacity_Wh;
  if (remaining_wh > 0xFFFFu)
    remaining_wh = 0xFFFFu;
  SUNGROW_702.data.u8[4] = (remaining_wh & 0x00FF);
  SUNGROW_702.data.u8[5] = (remaining_wh >> 8);
  // Capacity max (Wh), clamped to 16-bit to avoid overflow on large packs
  capacity_wh = datalayer.battery.info.reported_total_capacity_Wh;
  if (capacity_wh > 0xFFFFu)
    capacity_wh = 0xFFFFu;
  SUNGROW_702.data.u8[6] = (capacity_wh & 0x00FF);
  SUNGROW_702.data.u8[7] = (capacity_wh >> 8);

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
  SUNGROW_704.data.u8[2] = (current_dA & 0xFF);
  SUNGROW_704.data.u8[3] = ((current_dA >> 8) & 0xFF);
  // Another voltage. Different but similar
  SUNGROW_704.data.u8[4] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_704.data.u8[5] = (datalayer.battery.status.voltage_dV >> 8);
  // Temperature (signed int16_t in 0.1°C units)
  SUNGROW_704.data.u8[6] = (datalayer.battery.status.temperature_max_dC & 0xFF);
  SUNGROW_704.data.u8[7] = ((datalayer.battery.status.temperature_max_dC >> 8) & 0xFF);

  // Battery status: 0=Unplugged, 1=Standby, 2=Run
  SUNGROW_705.data.u8[0] = 0x02;                                   // Always "Run"
  SUNGROW_705.data.u8[1] = 0x00;                                   // Magic number
  SUNGROW_705.data.u8[2] = 0x01;                                   // Magic number
  uint16_t battery_model_id = 8420 + battery_config.module_count;  // SBR064=8422, SBR096=8423, etc.
  SUNGROW_705.data.u8[3] = (battery_model_id & 0xFF);
  SUNGROW_705.data.u8[4] = (battery_model_id >> 8);
  //Vbat, again (eg 400.0V = 4000 , 16bits long)
  SUNGROW_705.data.u8[5] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SUNGROW_705.data.u8[6] = (datalayer.battery.status.voltage_dV >> 8);
  // Padding?
  SUNGROW_705.data.u8[7] = 0x00;  // Magic number

  // Temperature Max (signed int16_t in 0.1°C units)
  SUNGROW_706.data.u8[0] = (datalayer.battery.status.temperature_max_dC & 0xFF);
  SUNGROW_706.data.u8[1] = ((datalayer.battery.status.temperature_max_dC >> 8) & 0xFF);
  // Temperature Min (signed int16_t in 0.1°C units)
  SUNGROW_706.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0xFF);
  SUNGROW_706.data.u8[3] = ((datalayer.battery.status.temperature_min_dC >> 8) & 0xFF);
  // Cell voltage max
  SUNGROW_706.data.u8[4] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  SUNGROW_706.data.u8[5] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  // Cell voltage min
  SUNGROW_706.data.u8[6] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  SUNGROW_706.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  // Battery Configuration
  SUNGROW_707.data.u8[0] = 0x26;                                    // Magic number
  SUNGROW_707.data.u8[1] = 0x00;                                    // Magic number
  SUNGROW_707.data.u8[2] = 0x00;                                    // Magic number
  SUNGROW_707.data.u8[3] = 0x01;                                    // Magic number. Num of stacks?
  SUNGROW_707.data.u8[4] = (battery_config.nameplate_wh & 0x00FF);  // Nameplate capacity
  SUNGROW_707.data.u8[5] = (battery_config.nameplate_wh >> 8);      // Nameplate capacity
  SUNGROW_707.data.u8[6] = battery_config.module_count;             // Num of modules
  SUNGROW_707.data.u8[7] = 0x00;                                    // Padding?

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

  // Populate 0x70F_05/06/07 with module SOC based on module_count
  // 0x70F_05: modules 1-3, 0x70F_06: modules 4-6, 0x70F_07: modules 7-8
  {
    CAN_frame* soc_frames[3] = {&SUNGROW_70F_05, &SUNGROW_70F_06, &SUNGROW_70F_07};
    for (uint8_t m = 0; m < battery_config.module_count && m < 8; m++) {
      uint8_t frame_idx = m / 3;         // 0-2->0, 3-5->1, 6-7->2
      uint8_t offset = (m % 3) * 2 + 2;  // 0->2, 1->4, 2->6
      soc_frames[frame_idx]->data.u8[offset] = (datalayer.battery.status.real_soc & 0xFF);
      soc_frames[frame_idx]->data.u8[offset + 1] = (datalayer.battery.status.real_soc >> 8);
    }
  }

  // 0x713 - Battery Cell Temperature Overview
  // Cell position with minimum temperature (cell/module addressing)
  SUNGROW_713.data.u8[0] = 0x02;  // Cell location with minimum temperature
  SUNGROW_713.data.u8[1] = 0x01;  // Module location with minimum temperature
  // Minimum cell temperature (signed int16_t in 0.1°C units)
  SUNGROW_713.data.u8[2] = (datalayer.battery.status.temperature_min_dC & 0xFF);
  SUNGROW_713.data.u8[3] = ((datalayer.battery.status.temperature_min_dC >> 8) & 0xFF);
  // Cell position with maximum temperature (cell/module addressing)
  SUNGROW_713.data.u8[4] = 0x01;  // Cell location with maximum temperature
  SUNGROW_713.data.u8[5] = 0x02;  // Module location with maximum temperature
  // Maximum cell temperature (signed int16_t in 0.1°C units)
  SUNGROW_713.data.u8[6] = (datalayer.battery.status.temperature_max_dC & 0xFF);
  SUNGROW_713.data.u8[7] = ((datalayer.battery.status.temperature_max_dC >> 8) & 0xFF);

  // 0x714 - Battery Cell Voltage Overview
  // Cell position with maximum voltage (cell/module addressing)
  SUNGROW_714.data.u8[0] = 0x05;  // Cell location with minimum voltage
  SUNGROW_714.data.u8[1] = 0x01;  // Module location with minimum voltage
  // Maximum cell voltage (0.1 mV units = cell_max_voltage_mV * 10)
  SUNGROW_714.data.u8[2] = ((datalayer.battery.status.cell_max_voltage_mV * 10) & 0x00FF);
  SUNGROW_714.data.u8[3] = ((datalayer.battery.status.cell_max_voltage_mV * 10) >> 8);
  // Cell position with minimum voltage (cell/module addressing)
  SUNGROW_714.data.u8[4] = 0x11;  // Cell location with maximum voltage
  SUNGROW_714.data.u8[5] = 0x02;  // Module location with maximum voltage
  // Minimum cell voltage (0.1 mV units = cell_min_voltage_mV * 10)
  SUNGROW_714.data.u8[6] = ((datalayer.battery.status.cell_min_voltage_mV * 10) & 0x00FF);
  SUNGROW_714.data.u8[7] = ((datalayer.battery.status.cell_min_voltage_mV * 10) >> 8);

  // 0x715-0x718 - Module Cell Voltage Overview (min/max per module)
  // Each frame holds 2 modules: [min1, max1, min2, max2] in 0.1mV units
  // 0x715: modules 1+2, 0x716: modules 3+4, 0x717: modules 5+6, 0x718: modules 7+8
  {
    const uint16_t cell_min_01mV = datalayer.battery.status.cell_min_voltage_mV * 10;
    const uint16_t cell_max_01mV = datalayer.battery.status.cell_max_voltage_mV * 10;
    CAN_frame* voltage_frames[4] = {&SUNGROW_715, &SUNGROW_716, &SUNGROW_717, &SUNGROW_718};

    for (uint8_t m = 0; m < battery_config.module_count && m < 8; m++) {
      uint8_t frame_idx = m / 2;     // 0-1->0, 2-3->1, 4-5->2, 6-7->3
      uint8_t offset = (m % 2) * 4;  // even->0, odd->4
      voltage_frames[frame_idx]->data.u8[offset + 0] = (cell_min_01mV & 0xFF);
      voltage_frames[frame_idx]->data.u8[offset + 1] = (cell_min_01mV >> 8);
      voltage_frames[frame_idx]->data.u8[offset + 2] = (cell_max_01mV & 0xFF);
      voltage_frames[frame_idx]->data.u8[offset + 3] = (cell_max_01mV >> 8);
    }
  }

  // 0x719 - Status and Module Fault???
  // Possibly relates to Modbus register 3_10789 and 3_10790
  SUNGROW_719.data.u8[0] = 0x02;

  //Copy 7## content to 0## messages
  // SUNGROW_000 all bytes 0x00
  memcpy(SUNGROW_001.data.u8, SUNGROW_701.data.u8, 8);
  memcpy(SUNGROW_002.data.u8, SUNGROW_702.data.u8, 8);
  memcpy(SUNGROW_003.data.u8, SUNGROW_703.data.u8, 8);
  memcpy(SUNGROW_004.data.u8, SUNGROW_704.data.u8, 8);
  memcpy(SUNGROW_005.data.u8, SUNGROW_705.data.u8, 8);
  memcpy(SUNGROW_006.data.u8, SUNGROW_706.data.u8, 8);
  memcpy(SUNGROW_007.data.u8, SUNGROW_707.data.u8, 8);
  memcpy(SUNGROW_008_00.data.u8, SUNGROW_708_00.data.u8, 8);
  memcpy(SUNGROW_008_01.data.u8, SUNGROW_708_01.data.u8, 8);
  // SUNGROW_009 all bytes 0x00
  memcpy(SUNGROW_00A_00.data.u8, SUNGROW_70A_00.data.u8, 8);
  memcpy(SUNGROW_00A_01.data.u8, SUNGROW_70A_01.data.u8, 8);
  memcpy(SUNGROW_00B.data.u8, SUNGROW_70B.data.u8, 8);
  memcpy(SUNGROW_00D.data.u8, SUNGROW_70D.data.u8, 8);
  memcpy(SUNGROW_00E.data.u8, SUNGROW_70E.data.u8, 8);
  memcpy(SUNGROW_013.data.u8, SUNGROW_713.data.u8, 8);
  memcpy(SUNGROW_014.data.u8, SUNGROW_714.data.u8, 8);
  memcpy(SUNGROW_015.data.u8, SUNGROW_715.data.u8, 8);
  memcpy(SUNGROW_016.data.u8, SUNGROW_716.data.u8, 8);
  memcpy(SUNGROW_017.data.u8, SUNGROW_717.data.u8, 8);
  memcpy(SUNGROW_018.data.u8, SUNGROW_718.data.u8, 8);
  memcpy(SUNGROW_019.data.u8, SUNGROW_719.data.u8, 8);
  memcpy(SUNGROW_01A.data.u8, SUNGROW_71A.data.u8, 8);
  memcpy(SUNGROW_01B.data.u8, SUNGROW_71B.data.u8, 8);
  memcpy(SUNGROW_01C.data.u8, SUNGROW_71C.data.u8, 8);
  memcpy(SUNGROW_01D.data.u8, SUNGROW_71D.data.u8, 8);
  memcpy(SUNGROW_01E.data.u8, SUNGROW_71E.data.u8, 8);

  //Copy 7## content to 5## messages
  memcpy(SUNGROW_501.data.u8, SUNGROW_701.data.u8, 8);
  memcpy(SUNGROW_502.data.u8, SUNGROW_702.data.u8, 8);
  memcpy(SUNGROW_503.data.u8, SUNGROW_703.data.u8, 8);
  memcpy(SUNGROW_504.data.u8, SUNGROW_704.data.u8, 8);
  memcpy(SUNGROW_505.data.u8, SUNGROW_705.data.u8, 8);
  memcpy(SUNGROW_506.data.u8, SUNGROW_706.data.u8, 8);

  // 0x504 cannot be a straight copy: current must be the opposite sign
  int32_t flipped = -(static_cast<int32_t>(current_dA));
  int16_t current_dA_flipped = clamp_i32_to_i16(flipped);

  SUNGROW_504.data.u8[2] = (current_dA_flipped & 0xFF);
  SUNGROW_504.data.u8[3] = ((current_dA_flipped >> 8) & 0xFF);

// TODO: This needs to do something useful
#ifdef DEBUG_VIA_USB
  if (inverter_sends_000) {
    Serial.println("Inverter sends 0x000");
  }
#endif
}

void SungrowInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x100:
      // SH10RS RUN @ ~1,250ms (group with one message every 250ms)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x101:
      // SH10RS init @ 13,000ms (SH15T as well?)
      // System time as epoch in b0->b3
      // b4->b7 all 0x00
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can_init = true;
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
      // SH10RS run @ ~1,250ms (group with one message every 250ms)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x108:
      // SH10RS run @ ~1,250ms (group with one message every 250ms)
      transmit_can_init = false;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x109:
      // SH10RS run @ ~1,250ms (group with one message every 250ms)
      transmit_can_init = false;
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
      // AU SH10RS inverter sends this but does not support a BYD battery.
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can_init = true;  // We only see 0x191 when the inverter is searching for a battery.
      break;
    case 0x1E0: {
      // Modbus RTU over CAN from inverter to battery.

      // Inverter Request
      // [8] 01 04 4D E2 00 02 C6 91
      // Slave Addr: 0x01
      // Function: 0x04
      // Start Register: 0x4DE2
      // Quantity: 0x0002 (2 registers)
      // CRC16: 0x91C6

      // Battery Response
      // [8] 01 04 04 01 F4 00 00 BB | [1] 8A
      // Slave Addr: 0x01
      // Function: 0x04
      // Byte Count = 0x04 (2 registers x 2 bytes)
      // Data: 0x01F4, 0x0000 => Decimal 500, 0
      // CRC16: 0x8ABB
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;

      // We only handle 8-byte Modbus request frames here.
      if (rx_frame.DLC != 8) {
        break;
      }

      const uint8_t* p = rx_frame.data.u8;

      // Validate Modbus RTU CRC16 (little-endian in bytes 6..7) over the first 6 bytes.
      const uint16_t req_crc = static_cast<uint16_t>(p[6]) | (static_cast<uint16_t>(p[7]) << 8);
      const uint16_t calc_crc = modbus_crc(p, 6);
      if (req_crc != calc_crc) {
        break;
      }

      const uint8_t addr = p[0];
      const uint8_t func = p[1];
      const uint16_t start_addr = (static_cast<uint16_t>(p[2]) << 8) | p[3];
      const uint16_t quantity = (static_cast<uint16_t>(p[4]) << 8) | p[5];

      if (addr != SungrowInverter::MODBUS_SLAVE_ADDR || start_addr != SungrowInverter::MODBUS_REGISTER_BASE_ADDR) {
        break;
      }

      // Helper to construct a Modbus RTU response (addr/func/byteCount/data),
      // append CRC, split into CAN frames of up to 8 bytes, and send them.
      auto send_modbus_response = [this, &rx_frame](const uint8_t* payload, uint8_t payload_len) {
        constexpr uint8_t MAX_PAYLOAD = 3 + MODBUS_MAX_DATA_BYTES;  // addr + func + byteCount + data bytes
        constexpr uint8_t MAX_TOTAL = MAX_PAYLOAD + 2;              // + CRC16

        if (payload_len > MAX_PAYLOAD) {
          return;  // Defensive: ignore impossible lengths.
        }

        uint8_t frame_bytes[MAX_TOTAL];

        // Copy payload (addr, func, byteCount, data...)
        for (uint8_t i = 0; i < payload_len; ++i) {
          frame_bytes[i] = payload[i];
        }

        // Append CRC16 (little-endian)
        const uint16_t crc = modbus_crc(payload, payload_len);
        const uint8_t crc_lo = static_cast<uint8_t>(crc & 0xFF);
        const uint8_t crc_hi = static_cast<uint8_t>((crc >> 8) & 0xFF);

        frame_bytes[payload_len] = crc_lo;
        frame_bytes[payload_len + 1] = crc_hi;

        uint8_t total_len = payload_len + 2;  // payload + CRC
        uint8_t offset = 0;

        while (total_len > 0) {
          CAN_frame f = rx_frame;
          const uint8_t chunk = (total_len > 8) ? 8 : total_len;
          f.DLC = chunk;
          for (uint8_t i = 0; i < chunk; ++i) {
            f.data.u8[i] = frame_bytes[offset + i];
          }
          transmit_can_frame(&f);
          offset += chunk;
          total_len -= chunk;
        }
      };

      auto send_modbus_registers = [send_modbus_response, addr, func](uint16_t quantity) {
        if (quantity == 0 || quantity > SungrowInverter::MODBUS_REGISTER_QTY) {
          return;
        }

        const uint8_t byte_count = static_cast<uint8_t>(quantity * 2);
        uint8_t payload[3 + MODBUS_MAX_DATA_BYTES];  // addr, func, byteCount, data bytes

        payload[0] = addr;
        payload[1] = func;
        payload[2] = byte_count;

        uint8_t idx = 3;
        for (uint16_t reg = 0; reg < quantity; ++reg) {
          const uint16_t value = MODBUS_REGISTER_VALUES[reg];
          payload[idx++] = static_cast<uint8_t>((value >> 8) & 0xFF);
          payload[idx++] = static_cast<uint8_t>(value & 0xFF);
        }

        send_modbus_response(payload, idx);
      };

      // For now we only support 0x03/0x04 reads against this register window.
      if (func == MODBUS_FUNC_READ_INPUT_REGS || func == MODBUS_FUNC_READ_HOLDING_REGS) {
        send_modbus_registers(quantity);
      }
    } break;
    default:
      break;
  }
}

void SungrowInverter::transmit_can(unsigned long currentMillis) {

  if (transmit_can_init) {

    // ---- INIT ----
    if ((int32_t)(currentMillis - previousMillis1s) >= INTERVAL_1_S) {
      previousMillis1s = currentMillis;

      // Head messages
      transmit_can_frame(&SUNGROW_500);
      transmit_can_frame(&SUNGROW_400);
      transmit_can_frame(&SUNGROW_401);

      // Init specific messages
      transmit_can_frame(&SUNGROW_007);
      transmit_can_frame(&SUNGROW_008_00);
      transmit_can_frame(&SUNGROW_008_01);
      transmit_can_frame(&SUNGROW_009);
      transmit_can_frame(&SUNGROW_00A_00);
      transmit_can_frame(&SUNGROW_00A_01);
      transmit_can_frame(&SUNGROW_00B);
      transmit_can_frame(&SUNGROW_00D);
      transmit_can_frame(&SUNGROW_00E);

      // Tail messages
      transmit_can_frame(&SUNGROW_512);
      transmit_can_frame(&SUNGROW_501);
      transmit_can_frame(&SUNGROW_502);
      transmit_can_frame(&SUNGROW_503);
      transmit_can_frame(&SUNGROW_504);
      transmit_can_frame(&SUNGROW_505);
      transmit_can_frame(&SUNGROW_506);
    }
    return;
  }

  // ---- RUN ----
  // ---- 1s group ----
  if ((int32_t)(currentMillis - previousMillis1s) >= INTERVAL_1_S) {
    if ((currentMillis - previousMillisBatch) < delay_between_batches_ms) {
      return;
    }

    previousMillisBatch = currentMillis;

    switch (batch_send_index) {
      case 0:
        // Head messages
        transmit_can_frame(&SUNGROW_500);
        transmit_can_frame(&SUNGROW_400);
        transmit_can_frame(&SUNGROW_401);
        break;

      case 1:
        // Run batch A
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
        break;

      case 2:
        // Run batch B
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
        break;

      case 3:
        // Run batch C
        transmit_can_frame(&SUNGROW_70F_00);
        transmit_can_frame(&SUNGROW_70F_01);
        transmit_can_frame(&SUNGROW_70F_02);
        transmit_can_frame(&SUNGROW_70F_03);
        transmit_can_frame(&SUNGROW_70F_04);
        transmit_can_frame(&SUNGROW_70F_05);  // Modules 1-3 SOC (zeros if unpopulated)
        transmit_can_frame(&SUNGROW_70F_06);  // Modules 4-6 SOC (zeros if unpopulated)
        transmit_can_frame(&SUNGROW_70F_07);  // Modules 7-8 SOC (zeros if unpopulated)
        transmit_can_frame(&SUNGROW_71A);
        transmit_can_frame(&SUNGROW_71B);  // Modules 1+2 production date (zeros if unpopulated)
        transmit_can_frame(&SUNGROW_71C);  // Modules 3+4 production date (zeros if unpopulated)
        transmit_can_frame(&SUNGROW_71D);  // Modules 5+6 production date (zeros if unpopulated)
        transmit_can_frame(&SUNGROW_71E);  // Modules 7+8 production date (zeros if unpopulated)
        break;

      case 4:
        previousMillis1s = currentMillis;

        // Tail messages
        transmit_can_frame(&SUNGROW_512);
        transmit_can_frame(&SUNGROW_501);
        transmit_can_frame(&SUNGROW_502);
        transmit_can_frame(&SUNGROW_503);
        transmit_can_frame(&SUNGROW_504);
        transmit_can_frame(&SUNGROW_505);
        transmit_can_frame(&SUNGROW_506);
        break;

      default:
        break;
    }

    batch_send_index++;
    if (batch_send_index > 4) {
      batch_send_index = 0;
    }
  }

  // ---- 10s group ----
  if ((int32_t)(currentMillis - previousMillis10s) >= INTERVAL_10_S) {
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

  // ---- 60s group ----
  if ((int32_t)(currentMillis - previousMillis60s) >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;
    transmit_can_frame(&SUNGROW_71F_01_01);
    transmit_can_frame(&SUNGROW_71F_01_02);
    transmit_can_frame(&SUNGROW_71F_01_03);
    transmit_can_frame(&SUNGROW_71F_02_01);
    transmit_can_frame(&SUNGROW_71F_02_02);
    transmit_can_frame(&SUNGROW_71F_02_03);
    if (battery_config.module_count >= 3) {
      transmit_can_frame(&SUNGROW_71F_03_01);
      transmit_can_frame(&SUNGROW_71F_03_02);
      transmit_can_frame(&SUNGROW_71F_03_03);
    }
    if (battery_config.module_count >= 4) {
      transmit_can_frame(&SUNGROW_71F_04_01);
      transmit_can_frame(&SUNGROW_71F_04_02);
      transmit_can_frame(&SUNGROW_71F_04_03);
    }
    if (battery_config.module_count >= 5) {
      transmit_can_frame(&SUNGROW_71F_05_01);
      transmit_can_frame(&SUNGROW_71F_05_02);
      transmit_can_frame(&SUNGROW_71F_05_03);
    }
    if (battery_config.module_count >= 6) {
      transmit_can_frame(&SUNGROW_71F_06_01);
      transmit_can_frame(&SUNGROW_71F_06_02);
      transmit_can_frame(&SUNGROW_71F_06_03);
    }
    if (battery_config.module_count >= 7) {
      transmit_can_frame(&SUNGROW_71F_07_01);
      transmit_can_frame(&SUNGROW_71F_07_02);
      transmit_can_frame(&SUNGROW_71F_07_03);
    }
    if (battery_config.module_count >= 8) {
      transmit_can_frame(&SUNGROW_71F_08_01);
      transmit_can_frame(&SUNGROW_71F_08_02);
      transmit_can_frame(&SUNGROW_71F_08_03);
    }
  }
}
