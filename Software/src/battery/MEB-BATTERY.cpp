#include "../include.h"
#ifdef MEB_BATTERY
#include <algorithm>  // For std::min and std::max
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "MEB-BATTERY.h"

/*
TODO list
- Get contactors closing
- What CAN messages needs to be sent towards the battery to keep it alive
- Check value mappings on the PID polls
- Check value mappings on the constantly broadcasted messages
- Check all TODO:s in the code
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send

#define MAX_CELL_VOLTAGE 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950  //Battery is put into emergency stop if one cell goes below this value

#define PID_SOC 0x028C
#define PID_VOLTAGE 0x1E3B
#define PID_CURRENT 0x1E3D
#define PID_MAX_TEMP 0x1E0E
#define PID_MIN_TEMP 0x1E0F
#define PID_MAX_CHARGE_VOLTAGE 0x5171
#define PID_MIN_DISCHARGE_VOLTAGE 0x5170
#define PID_ALLOWED_CHARGE_POWER 0x1E1B
#define PID_ALLOWED_DISCHARGE_POWER 0x1E1C
#define PID_CELLVOLTAGE_CELL_1 0x1E40
#define PID_CELLVOLTAGE_CELL_2 0x1E41
#define PID_CELLVOLTAGE_CELL_3 0x1E42
#define PID_CELLVOLTAGE_CELL_4 0x1E43
#define PID_CELLVOLTAGE_CELL_5 0x1E44
#define PID_CELLVOLTAGE_CELL_6 0x1E45
#define PID_CELLVOLTAGE_CELL_7 0x1E46
#define PID_CELLVOLTAGE_CELL_8 0x1E47
#define PID_CELLVOLTAGE_CELL_9 0x1E48
#define PID_CELLVOLTAGE_CELL_10 0x1E49
#define PID_CELLVOLTAGE_CELL_11 0x1E4A
#define PID_CELLVOLTAGE_CELL_12 0x1E4B
#define PID_CELLVOLTAGE_CELL_13 0x1E4C
#define PID_CELLVOLTAGE_CELL_14 0x1E4D
#define PID_CELLVOLTAGE_CELL_15 0x1E4E
#define PID_CELLVOLTAGE_CELL_16 0x1E4F
#define PID_CELLVOLTAGE_CELL_17 0x1E50
#define PID_CELLVOLTAGE_CELL_18 0x1E51
#define PID_CELLVOLTAGE_CELL_19 0x1E52
#define PID_CELLVOLTAGE_CELL_20 0x1E53
#define PID_CELLVOLTAGE_CELL_21 0x1E54
#define PID_CELLVOLTAGE_CELL_22 0x1E55
#define PID_CELLVOLTAGE_CELL_23 0x1E56
#define PID_CELLVOLTAGE_CELL_24 0x1E57
#define PID_CELLVOLTAGE_CELL_25 0x1E58
#define PID_CELLVOLTAGE_CELL_26 0x1E59
#define PID_CELLVOLTAGE_CELL_27 0x1E5A
#define PID_CELLVOLTAGE_CELL_28 0x1E5B
#define PID_CELLVOLTAGE_CELL_29 0x1E5C
#define PID_CELLVOLTAGE_CELL_30 0x1E5D
#define PID_CELLVOLTAGE_CELL_31 0x1E5E
#define PID_CELLVOLTAGE_CELL_32 0x1E5F
#define PID_CELLVOLTAGE_CELL_33 0x1E60
#define PID_CELLVOLTAGE_CELL_34 0x1E61
#define PID_CELLVOLTAGE_CELL_35 0x1E62
#define PID_CELLVOLTAGE_CELL_36 0x1E63
#define PID_CELLVOLTAGE_CELL_37 0x1E64
#define PID_CELLVOLTAGE_CELL_38 0x1E65
#define PID_CELLVOLTAGE_CELL_39 0x1E66
#define PID_CELLVOLTAGE_CELL_40 0x1E67
#define PID_CELLVOLTAGE_CELL_41 0x1E68
#define PID_CELLVOLTAGE_CELL_42 0x1E69
#define PID_CELLVOLTAGE_CELL_43 0x1E6A
#define PID_CELLVOLTAGE_CELL_44 0x1E6B
#define PID_CELLVOLTAGE_CELL_45 0x1E6C
#define PID_CELLVOLTAGE_CELL_46 0x1E6D
#define PID_CELLVOLTAGE_CELL_47 0x1E6E
#define PID_CELLVOLTAGE_CELL_48 0x1E6F
#define PID_CELLVOLTAGE_CELL_49 0x1E70
#define PID_CELLVOLTAGE_CELL_50 0x1E71
#define PID_CELLVOLTAGE_CELL_51 0x1E72
#define PID_CELLVOLTAGE_CELL_52 0x1E73
#define PID_CELLVOLTAGE_CELL_53 0x1E74
#define PID_CELLVOLTAGE_CELL_54 0x1E75
#define PID_CELLVOLTAGE_CELL_55 0x1E76
#define PID_CELLVOLTAGE_CELL_56 0x1E77
#define PID_CELLVOLTAGE_CELL_57 0x1E78
#define PID_CELLVOLTAGE_CELL_58 0x1E79
#define PID_CELLVOLTAGE_CELL_59 0x1E7A
#define PID_CELLVOLTAGE_CELL_60 0x1E7B
#define PID_CELLVOLTAGE_CELL_61 0x1E7C
#define PID_CELLVOLTAGE_CELL_62 0x1E7D
#define PID_CELLVOLTAGE_CELL_63 0x1E7E
#define PID_CELLVOLTAGE_CELL_64 0x1E7F
#define PID_CELLVOLTAGE_CELL_65 0x1E80
#define PID_CELLVOLTAGE_CELL_66 0x1E81
#define PID_CELLVOLTAGE_CELL_67 0x1E82
#define PID_CELLVOLTAGE_CELL_68 0x1E83
#define PID_CELLVOLTAGE_CELL_69 0x1E84
#define PID_CELLVOLTAGE_CELL_70 0x1E85
#define PID_CELLVOLTAGE_CELL_71 0x1E86
#define PID_CELLVOLTAGE_CELL_72 0x1E87
#define PID_CELLVOLTAGE_CELL_73 0x1E88
#define PID_CELLVOLTAGE_CELL_74 0x1E89
#define PID_CELLVOLTAGE_CELL_75 0x1E8A
#define PID_CELLVOLTAGE_CELL_76 0x1E8B
#define PID_CELLVOLTAGE_CELL_77 0x1E8C
#define PID_CELLVOLTAGE_CELL_78 0x1E8D
#define PID_CELLVOLTAGE_CELL_79 0x1E8E
#define PID_CELLVOLTAGE_CELL_80 0x1E8F
#define PID_CELLVOLTAGE_CELL_81 0x1E90
#define PID_CELLVOLTAGE_CELL_82 0x1E91
#define PID_CELLVOLTAGE_CELL_83 0x1E92
#define PID_CELLVOLTAGE_CELL_84 0x1E93
#define PID_CELLVOLTAGE_CELL_85 0x1E94
#define PID_CELLVOLTAGE_CELL_86 0x1E95
#define PID_CELLVOLTAGE_CELL_87 0x1E96
#define PID_CELLVOLTAGE_CELL_88 0x1E97
#define PID_CELLVOLTAGE_CELL_89 0x1E98
#define PID_CELLVOLTAGE_CELL_90 0x1E99
#define PID_CELLVOLTAGE_CELL_91 0x1E9A
#define PID_CELLVOLTAGE_CELL_92 0x1E9B
#define PID_CELLVOLTAGE_CELL_93 0x1E9C
#define PID_CELLVOLTAGE_CELL_94 0x1E9D
#define PID_CELLVOLTAGE_CELL_95 0x1E9E
#define PID_CELLVOLTAGE_CELL_96 0x1E9F
#define PID_CELLVOLTAGE_CELL_97 0x1EA0
#define PID_CELLVOLTAGE_CELL_98 0x1EA1
#define PID_CELLVOLTAGE_CELL_99 0x1EA2
#define PID_CELLVOLTAGE_CELL_100 0x1EA3
#define PID_CELLVOLTAGE_CELL_101 0x1EA4
#define PID_CELLVOLTAGE_CELL_102 0x1EA5
#define PID_CELLVOLTAGE_CELL_103 0x1EA6
#define PID_CELLVOLTAGE_CELL_104 0x1EA7
#define PID_CELLVOLTAGE_CELL_105 0x1EA8
#define PID_CELLVOLTAGE_CELL_106 0x1EA9
#define PID_CELLVOLTAGE_CELL_107 0x1EAA
#define PID_CELLVOLTAGE_CELL_108 0x1EAB

static uint32_t poll_pid = 0;
static uint32_t pid_reply = 0;

static uint16_t battery_soc = 0;
static uint16_t battery_voltage = 0;
static int16_t battery_current = 0;
static int16_t battery_max_temp = 600;
static int16_t battery_min_temp = 600;
static uint16_t battery_max_charge_voltage = 0;
static uint16_t battery_min_discharge_voltage = 0;
static uint16_t battery_allowed_charge_power = 0;
static uint16_t battery_allowed_discharge_power = 0;
static uint16_t cellvoltages[108];
static uint16_t tempval = 0;

static uint8_t BMS_20_CRC = 0;
static uint8_t BMS_20_BZ = 0;
static bool BMS_fault_status_contactor = false;
static bool BMS_exp_limits_active = 0;
static uint8_t BMS_is_mode = 0;
static bool BMS_HVIL_status = 0;
static bool BMS_fault_HVbatt_shutdown = 0;
static bool BMS_fault_HVbatt_shutdown_req = 0;
static bool BMS_fault_performance = 0;
static uint16_t BMS_current = 0;
static bool BMS_fault_emergency_shutdown_crash = 0;
static uint32_t BMS_voltage_intermediate = 0;
static uint32_t BMS_voltage = 0;

CAN_frame MEB_POLLING_FRAME = {.FD = true,
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x1C40007B,  // SOC 02 8C
                               .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_ACK_FRAME = {.FD = true,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1C40007B,  // Ack
                           .data = {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = battery_soc * 10;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = battery_voltage * 2.5;

  datalayer.battery.status.current_dA = battery_current / 10;  //TODO: scaling?

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W = 5000;
  //battery_allowed_charge_power * 10;  //TODO: Value is 0. Satisfy battery somehow?

  datalayer.battery.status.max_discharge_power_W = 5000;
  //battery_allowed_discharge_power * 10;  //TODO: Value is 0. Satisfy battery somehow?

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.temperature_min_dC = ((battery_min_temp / 2) - 350);

  datalayer.battery.status.temperature_max_dC = ((battery_max_temp / 2) - 350);

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, 108 * sizeof(uint16_t));

  // Initialize min and max, lets find which cells are min and max!
  uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
  uint16_t max_cell_mv_value = 0;
  // Loop to find the min and max while ignoring zero values
  for (uint8_t i = 0; i < 108; ++i) {
    uint16_t voltage_mV = datalayer.battery.status.cell_voltages_mV[i];
    if (voltage_mV != 0) {  // Skip unread values (0)
      min_cell_mv_value = std::min(min_cell_mv_value, voltage_mV);
      max_cell_mv_value = std::max(max_cell_mv_value, voltage_mV);
    }
  }
  // If all array values are 0, reset min/max to 3700
  if (min_cell_mv_value == std::numeric_limits<uint16_t>::max()) {
    min_cell_mv_value = 3700;
    max_cell_mv_value = 3700;
  }

  datalayer.battery.status.cell_min_voltage_mV = min_cell_mv_value;
  datalayer.battery.status.cell_max_voltage_mV = max_cell_mv_value;

#ifdef DEBUG_VIA_USB
  Serial.println();  //sepatator
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
  Serial.print(BMS_voltage);
  Serial.print(" HVIL: ");
  Serial.print(BMS_HVIL_status);
  Serial.print(" Current: ");
  Serial.print(BMS_current);
#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0xCF:  //BMS_20 , TODO: confirm location for all these
      BMS_20_CRC = rx_frame.data.u8[0];
      BMS_20_BZ = (rx_frame.data.u8[1] & 0x0F);
      BMS_fault_status_contactor = (rx_frame.data.u8[1] & 0x20) >> 5;
      //BMS_exp_limits_active
      BMS_is_mode = (rx_frame.data.u8[2] & 0x07);
      BMS_HVIL_status = (rx_frame.data.u8[2] & 0x08) >> 3;
      //BMS_fault_HVbatt_shutdown
      //BMS_fault_HVbatt_shutdown_req
      //BMS_fault_performance
      BMS_current = (rx_frame.data.u8[4] << 8) + rx_frame.data.u8[3];
      //BMS_fault_emergency_shutdown_crash
      BMS_voltage_intermediate = (((rx_frame.data.u8[6] & 0x0F) << 8) + (rx_frame.data.u8[5])) * 25;
      BMS_voltage = ((rx_frame.data.u8[7] << 4) + ((rx_frame.data.u8[6] & 0xF0) >> 4)) * 25;
      break;
    case 0x1C42007B:                      // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can(&MEB_ACK_FRAME, can_config.battery);
      }
      if (rx_frame.DLC == 8) {
        pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];
      } else {  //12 or 24bit message has reply in other location
        pid_reply = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
      }

      switch (pid_reply) {
        case PID_SOC:
          battery_soc = rx_frame.data.u8[4] * 4;  // 135*4 = 54.0%
        case PID_VOLTAGE:
          battery_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_CURRENT:  // IDLE 0A: 00 08 62 1E 3D (00 02) 49 F0 39 AA AA
          battery_current = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);  //TODO: right bits?
          break;
        case PID_MAX_TEMP:
          battery_max_temp = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_TEMP:
          battery_min_temp = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MAX_CHARGE_VOLTAGE:
          battery_max_charge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_DISCHARGE_VOLTAGE:
          battery_min_discharge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_CHARGE_POWER:
          battery_allowed_charge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_DISCHARGE_POWER:
          battery_allowed_discharge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_CELLVOLTAGE_CELL_1:
          cellvoltages[0] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_2:
          cellvoltages[1] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_3:
          cellvoltages[2] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_4:
          cellvoltages[3] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_5:
          cellvoltages[4] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_6:
          cellvoltages[5] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_7:
          cellvoltages[6] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_8:
          cellvoltages[7] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_9:
          cellvoltages[8] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_10:
          cellvoltages[9] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_11:
          cellvoltages[10] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_12:
          cellvoltages[11] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_13:
          cellvoltages[12] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_14:
          cellvoltages[13] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_15:
          cellvoltages[14] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_16:
          cellvoltages[15] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_17:
          cellvoltages[16] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_18:
          cellvoltages[17] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_19:
          cellvoltages[18] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_20:
          cellvoltages[19] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_21:
          cellvoltages[20] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_22:
          cellvoltages[21] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_23:
          cellvoltages[22] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_24:
          cellvoltages[23] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_25:
          cellvoltages[24] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_26:
          cellvoltages[25] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_27:
          cellvoltages[26] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_28:
          cellvoltages[27] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_29:
          cellvoltages[28] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_30:
          cellvoltages[29] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_31:
          cellvoltages[30] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_32:
          cellvoltages[31] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_33:
          cellvoltages[32] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_34:
          cellvoltages[33] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_35:
          cellvoltages[34] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_36:
          cellvoltages[35] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_37:
          cellvoltages[36] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_38:
          cellvoltages[37] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_39:
          cellvoltages[38] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_40:
          cellvoltages[39] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_41:
          cellvoltages[40] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_42:
          cellvoltages[41] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_43:
          cellvoltages[42] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_44:
          cellvoltages[43] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_45:
          cellvoltages[44] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_46:
          cellvoltages[45] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_47:
          cellvoltages[46] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_48:
          cellvoltages[47] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_49:
          cellvoltages[48] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_50:
          cellvoltages[49] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_51:
          cellvoltages[50] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_52:
          cellvoltages[51] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_53:
          cellvoltages[52] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_54:
          cellvoltages[53] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_55:
          cellvoltages[54] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_56:
          cellvoltages[55] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_57:
          cellvoltages[56] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_58:
          cellvoltages[57] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_59:
          cellvoltages[58] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_60:
          cellvoltages[59] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_61:
          cellvoltages[60] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_62:
          cellvoltages[61] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_63:
          cellvoltages[62] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_64:
          cellvoltages[63] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_65:
          cellvoltages[64] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_66:
          cellvoltages[65] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_67:
          cellvoltages[66] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_68:
          cellvoltages[67] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_69:
          cellvoltages[68] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_70:
          cellvoltages[69] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_71:
          cellvoltages[70] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_72:
          cellvoltages[71] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_73:
          cellvoltages[72] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_74:
          cellvoltages[73] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_75:
          cellvoltages[74] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_76:
          cellvoltages[75] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_77:
          cellvoltages[76] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_78:
          cellvoltages[77] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_79:
          cellvoltages[78] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_80:
          cellvoltages[79] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_81:
          cellvoltages[80] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_82:
          cellvoltages[81] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_83:
          cellvoltages[82] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_84:
          cellvoltages[83] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_85:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[84] = (tempval + 1000);
          } else {  // Cell 85 unavailable. We have a 84S battery (48kWh)
            datalayer.battery.info.number_of_cells = 84;
            datalayer.battery.info.max_design_voltage_dV = 3528;
            datalayer.battery.info.min_design_voltage_dV = 2520;
          }
          break;
        case PID_CELLVOLTAGE_CELL_86:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[85] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_87:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[86] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_88:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[87] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_89:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[88] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_90:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[89] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_91:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[90] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_92:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[91] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_93:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[92] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_94:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[93] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_95:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[94] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_96:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[95] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_97:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[96] = (tempval + 1000);
          } else {  // Cell 97 unavailable. We have a 96S battery (55kWh) (Unless already specified as 84S)
            if (datalayer.battery.info.number_of_cells == 84) {
              // Do nothing, we already identified it as 84S
            } else {
              datalayer.battery.info.number_of_cells = 96;
              datalayer.battery.info.max_design_voltage_dV = 4032;
              datalayer.battery.info.min_design_voltage_dV = 2880;
            }
          }
          break;
        case PID_CELLVOLTAGE_CELL_98:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[97] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_99:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[98] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_100:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[99] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_101:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[100] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_102:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[101] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_103:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[102] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_104:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[103] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_105:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[104] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_106:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[105] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_107:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[106] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_108:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[107] = (tempval + 1000);
            datalayer.battery.info.number_of_cells = 108;
            datalayer.battery.info.max_design_voltage_dV = 4536;
            datalayer.battery.info.min_design_voltage_dV = 3240;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  //Send 200ms message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (poll_pid) {
      case PID_SOC:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_SOC >> 8);  // High byte
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_SOC;         // Low byte
        poll_pid = PID_VOLTAGE;
        break;
      case PID_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_VOLTAGE;
        poll_pid = PID_CURRENT;
        break;
      case PID_CURRENT:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CURRENT >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CURRENT;
        poll_pid = PID_MAX_TEMP;
        break;
      case PID_MAX_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_TEMP;
        poll_pid = PID_MIN_TEMP;
        break;
      case PID_MIN_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_TEMP;
        poll_pid = PID_MAX_CHARGE_VOLTAGE;
        break;
      case PID_MAX_CHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_CHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_CHARGE_VOLTAGE;
        poll_pid = PID_MIN_DISCHARGE_VOLTAGE;
        break;
      case PID_MIN_DISCHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_DISCHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_DISCHARGE_VOLTAGE;
        poll_pid = PID_ALLOWED_CHARGE_POWER;
        break;
      case PID_ALLOWED_CHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_CHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_CHARGE_POWER;
        poll_pid = PID_ALLOWED_DISCHARGE_POWER;
        break;
      case PID_ALLOWED_DISCHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_DISCHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_DISCHARGE_POWER;
        poll_pid = PID_CELLVOLTAGE_CELL_1;  // Start polling cell voltages
        break;
      // Cell Voltage Cases
      case PID_CELLVOLTAGE_CELL_1:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CELLVOLTAGE_CELL_1 >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_1;
        poll_pid = PID_CELLVOLTAGE_CELL_2;
        break;
      case PID_CELLVOLTAGE_CELL_2:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_2;
        poll_pid = PID_CELLVOLTAGE_CELL_3;
        break;
      case PID_CELLVOLTAGE_CELL_3:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_3;
        poll_pid = PID_CELLVOLTAGE_CELL_4;
        break;
      case PID_CELLVOLTAGE_CELL_4:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_4;
        poll_pid = PID_CELLVOLTAGE_CELL_5;
        break;
      case PID_CELLVOLTAGE_CELL_5:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_5;
        poll_pid = PID_CELLVOLTAGE_CELL_6;
        break;
      case PID_CELLVOLTAGE_CELL_6:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_6;
        poll_pid = PID_CELLVOLTAGE_CELL_7;
        break;
      case PID_CELLVOLTAGE_CELL_7:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_7;
        poll_pid = PID_CELLVOLTAGE_CELL_8;
        break;
      case PID_CELLVOLTAGE_CELL_8:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_8;
        poll_pid = PID_CELLVOLTAGE_CELL_9;
        break;
      case PID_CELLVOLTAGE_CELL_9:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_9;
        poll_pid = PID_CELLVOLTAGE_CELL_10;
        break;
      case PID_CELLVOLTAGE_CELL_10:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_10;
        poll_pid = PID_CELLVOLTAGE_CELL_11;
        break;
      case PID_CELLVOLTAGE_CELL_11:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_11;
        poll_pid = PID_CELLVOLTAGE_CELL_12;
        break;
      case PID_CELLVOLTAGE_CELL_12:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_12;
        poll_pid = PID_CELLVOLTAGE_CELL_13;
        break;
      case PID_CELLVOLTAGE_CELL_13:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_13;
        poll_pid = PID_CELLVOLTAGE_CELL_14;
        break;
      case PID_CELLVOLTAGE_CELL_14:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_14;
        poll_pid = PID_CELLVOLTAGE_CELL_15;
        break;
      case PID_CELLVOLTAGE_CELL_15:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_15;
        poll_pid = PID_CELLVOLTAGE_CELL_16;
        break;
      case PID_CELLVOLTAGE_CELL_16:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_16;
        poll_pid = PID_CELLVOLTAGE_CELL_17;
        break;
      case PID_CELLVOLTAGE_CELL_17:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_17;
        poll_pid = PID_CELLVOLTAGE_CELL_18;
        break;
      case PID_CELLVOLTAGE_CELL_18:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_18;
        poll_pid = PID_CELLVOLTAGE_CELL_19;
        break;
      case PID_CELLVOLTAGE_CELL_19:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_19;
        poll_pid = PID_CELLVOLTAGE_CELL_20;
        break;
      case PID_CELLVOLTAGE_CELL_20:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_20;
        poll_pid = PID_CELLVOLTAGE_CELL_21;
        break;
      case PID_CELLVOLTAGE_CELL_21:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_21;
        poll_pid = PID_CELLVOLTAGE_CELL_22;
        break;
      case PID_CELLVOLTAGE_CELL_22:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_22;
        poll_pid = PID_CELLVOLTAGE_CELL_23;
        break;
      case PID_CELLVOLTAGE_CELL_23:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_23;
        poll_pid = PID_CELLVOLTAGE_CELL_24;
        break;
      case PID_CELLVOLTAGE_CELL_24:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_24;
        poll_pid = PID_CELLVOLTAGE_CELL_25;
        break;
      case PID_CELLVOLTAGE_CELL_25:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_25;
        poll_pid = PID_CELLVOLTAGE_CELL_26;
        break;
      case PID_CELLVOLTAGE_CELL_26:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_26;
        poll_pid = PID_CELLVOLTAGE_CELL_27;
        break;
      case PID_CELLVOLTAGE_CELL_27:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_27;
        poll_pid = PID_CELLVOLTAGE_CELL_28;
        break;
      case PID_CELLVOLTAGE_CELL_28:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_28;
        poll_pid = PID_CELLVOLTAGE_CELL_29;
        break;
      case PID_CELLVOLTAGE_CELL_29:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_29;
        poll_pid = PID_CELLVOLTAGE_CELL_30;
        break;
      case PID_CELLVOLTAGE_CELL_30:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_30;
        poll_pid = PID_CELLVOLTAGE_CELL_31;
        break;
      case PID_CELLVOLTAGE_CELL_31:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_31;
        poll_pid = PID_CELLVOLTAGE_CELL_32;
        break;
      case PID_CELLVOLTAGE_CELL_32:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_32;
        poll_pid = PID_CELLVOLTAGE_CELL_33;
        break;
      case PID_CELLVOLTAGE_CELL_33:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_33;
        poll_pid = PID_CELLVOLTAGE_CELL_34;
        break;
      case PID_CELLVOLTAGE_CELL_34:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_34;
        poll_pid = PID_CELLVOLTAGE_CELL_35;
        break;
      case PID_CELLVOLTAGE_CELL_35:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_35;
        poll_pid = PID_CELLVOLTAGE_CELL_36;
        break;
      case PID_CELLVOLTAGE_CELL_36:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_36;
        poll_pid = PID_CELLVOLTAGE_CELL_37;
        break;
      case PID_CELLVOLTAGE_CELL_37:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_37;
        poll_pid = PID_CELLVOLTAGE_CELL_38;
        break;
      case PID_CELLVOLTAGE_CELL_38:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_38;
        poll_pid = PID_CELLVOLTAGE_CELL_39;
        break;
      case PID_CELLVOLTAGE_CELL_39:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_39;
        poll_pid = PID_CELLVOLTAGE_CELL_40;
        break;
      case PID_CELLVOLTAGE_CELL_40:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_40;
        poll_pid = PID_CELLVOLTAGE_CELL_41;
        break;
      case PID_CELLVOLTAGE_CELL_41:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_41;
        poll_pid = PID_CELLVOLTAGE_CELL_42;
        break;
      case PID_CELLVOLTAGE_CELL_42:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_42;
        poll_pid = PID_CELLVOLTAGE_CELL_43;
        break;
      case PID_CELLVOLTAGE_CELL_43:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_43;
        poll_pid = PID_CELLVOLTAGE_CELL_44;
        break;
      case PID_CELLVOLTAGE_CELL_44:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_44;
        poll_pid = PID_CELLVOLTAGE_CELL_45;
        break;
      case PID_CELLVOLTAGE_CELL_45:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_45;
        poll_pid = PID_CELLVOLTAGE_CELL_46;
        break;
      case PID_CELLVOLTAGE_CELL_46:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_46;
        poll_pid = PID_CELLVOLTAGE_CELL_47;
        break;
      case PID_CELLVOLTAGE_CELL_47:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_47;
        poll_pid = PID_CELLVOLTAGE_CELL_48;
        break;
      case PID_CELLVOLTAGE_CELL_48:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_48;
        poll_pid = PID_CELLVOLTAGE_CELL_49;
        break;
      case PID_CELLVOLTAGE_CELL_49:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_49;
        poll_pid = PID_CELLVOLTAGE_CELL_50;
        break;
      case PID_CELLVOLTAGE_CELL_50:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_50;
        poll_pid = PID_CELLVOLTAGE_CELL_51;
        break;
      case PID_CELLVOLTAGE_CELL_51:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_51;
        poll_pid = PID_CELLVOLTAGE_CELL_52;
        break;
      case PID_CELLVOLTAGE_CELL_52:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_52;
        poll_pid = PID_CELLVOLTAGE_CELL_53;
        break;
      case PID_CELLVOLTAGE_CELL_53:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_53;
        poll_pid = PID_CELLVOLTAGE_CELL_54;
        break;
      case PID_CELLVOLTAGE_CELL_54:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_54;
        poll_pid = PID_CELLVOLTAGE_CELL_55;
        break;
      case PID_CELLVOLTAGE_CELL_55:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_55;
        poll_pid = PID_CELLVOLTAGE_CELL_56;
        break;
      case PID_CELLVOLTAGE_CELL_56:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_56;
        poll_pid = PID_CELLVOLTAGE_CELL_57;
        break;
      case PID_CELLVOLTAGE_CELL_57:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_57;
        poll_pid = PID_CELLVOLTAGE_CELL_58;
        break;
      case PID_CELLVOLTAGE_CELL_58:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_58;
        poll_pid = PID_CELLVOLTAGE_CELL_59;
        break;
      case PID_CELLVOLTAGE_CELL_59:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_59;
        poll_pid = PID_CELLVOLTAGE_CELL_60;
        break;
      case PID_CELLVOLTAGE_CELL_60:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_60;
        poll_pid = PID_CELLVOLTAGE_CELL_61;
        break;
      case PID_CELLVOLTAGE_CELL_61:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_61;
        poll_pid = PID_CELLVOLTAGE_CELL_62;
        break;
      case PID_CELLVOLTAGE_CELL_62:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_62;
        poll_pid = PID_CELLVOLTAGE_CELL_63;
        break;
      case PID_CELLVOLTAGE_CELL_63:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_63;
        poll_pid = PID_CELLVOLTAGE_CELL_64;
        break;
      case PID_CELLVOLTAGE_CELL_64:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_64;
        poll_pid = PID_CELLVOLTAGE_CELL_65;
        break;
      case PID_CELLVOLTAGE_CELL_65:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_65;
        poll_pid = PID_CELLVOLTAGE_CELL_66;
        break;
      case PID_CELLVOLTAGE_CELL_66:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_66;
        poll_pid = PID_CELLVOLTAGE_CELL_67;
        break;
      case PID_CELLVOLTAGE_CELL_67:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_67;
        poll_pid = PID_CELLVOLTAGE_CELL_68;
        break;
      case PID_CELLVOLTAGE_CELL_68:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_68;
        poll_pid = PID_CELLVOLTAGE_CELL_69;
        break;
      case PID_CELLVOLTAGE_CELL_69:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_69;
        poll_pid = PID_CELLVOLTAGE_CELL_70;
        break;
      case PID_CELLVOLTAGE_CELL_70:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_70;
        poll_pid = PID_CELLVOLTAGE_CELL_71;
        break;
      case PID_CELLVOLTAGE_CELL_71:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_71;
        poll_pid = PID_CELLVOLTAGE_CELL_72;
        break;
      case PID_CELLVOLTAGE_CELL_72:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_72;
        poll_pid = PID_CELLVOLTAGE_CELL_73;
        break;
      case PID_CELLVOLTAGE_CELL_73:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_73;
        poll_pid = PID_CELLVOLTAGE_CELL_74;
        break;
      case PID_CELLVOLTAGE_CELL_74:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_74;
        poll_pid = PID_CELLVOLTAGE_CELL_75;
        break;
      case PID_CELLVOLTAGE_CELL_75:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_75;
        poll_pid = PID_CELLVOLTAGE_CELL_76;
        break;
      case PID_CELLVOLTAGE_CELL_76:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_76;
        poll_pid = PID_CELLVOLTAGE_CELL_77;
        break;
      case PID_CELLVOLTAGE_CELL_77:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_77;
        poll_pid = PID_CELLVOLTAGE_CELL_78;
        break;
      case PID_CELLVOLTAGE_CELL_78:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_78;
        poll_pid = PID_CELLVOLTAGE_CELL_79;
        break;
      case PID_CELLVOLTAGE_CELL_79:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_79;
        poll_pid = PID_CELLVOLTAGE_CELL_80;
        break;
      case PID_CELLVOLTAGE_CELL_80:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_80;
        poll_pid = PID_CELLVOLTAGE_CELL_81;
        break;
      case PID_CELLVOLTAGE_CELL_81:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_81;
        poll_pid = PID_CELLVOLTAGE_CELL_82;
        break;
      case PID_CELLVOLTAGE_CELL_82:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_82;
        poll_pid = PID_CELLVOLTAGE_CELL_83;
        break;
      case PID_CELLVOLTAGE_CELL_83:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_83;
        poll_pid = PID_CELLVOLTAGE_CELL_84;
        break;
      case PID_CELLVOLTAGE_CELL_84:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_84;
        poll_pid = PID_CELLVOLTAGE_CELL_85;
        break;
      case PID_CELLVOLTAGE_CELL_85:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_85;
        poll_pid = PID_CELLVOLTAGE_CELL_86;
        break;
      case PID_CELLVOLTAGE_CELL_86:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_86;
        poll_pid = PID_CELLVOLTAGE_CELL_87;
        break;
      case PID_CELLVOLTAGE_CELL_87:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_87;
        poll_pid = PID_CELLVOLTAGE_CELL_88;
        break;
      case PID_CELLVOLTAGE_CELL_88:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_88;
        poll_pid = PID_CELLVOLTAGE_CELL_89;
        break;
      case PID_CELLVOLTAGE_CELL_89:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_89;
        poll_pid = PID_CELLVOLTAGE_CELL_90;
        break;
      case PID_CELLVOLTAGE_CELL_90:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_90;
        poll_pid = PID_CELLVOLTAGE_CELL_91;
        break;
      case PID_CELLVOLTAGE_CELL_91:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_91;
        poll_pid = PID_CELLVOLTAGE_CELL_92;
        break;
      case PID_CELLVOLTAGE_CELL_92:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_92;
        poll_pid = PID_CELLVOLTAGE_CELL_93;
        break;
      case PID_CELLVOLTAGE_CELL_93:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_93;
        poll_pid = PID_CELLVOLTAGE_CELL_94;
        break;
      case PID_CELLVOLTAGE_CELL_94:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_94;
        poll_pid = PID_CELLVOLTAGE_CELL_95;
        break;
      case PID_CELLVOLTAGE_CELL_95:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_95;
        poll_pid = PID_CELLVOLTAGE_CELL_96;
        break;
      case PID_CELLVOLTAGE_CELL_96:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_96;
        poll_pid = PID_CELLVOLTAGE_CELL_97;
        break;
      case PID_CELLVOLTAGE_CELL_97:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_97;
        poll_pid = PID_CELLVOLTAGE_CELL_98;
        break;
      case PID_CELLVOLTAGE_CELL_98:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_98;
        poll_pid = PID_CELLVOLTAGE_CELL_99;
        break;
      case PID_CELLVOLTAGE_CELL_99:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_99;
        poll_pid = PID_CELLVOLTAGE_CELL_100;
        break;
      case PID_CELLVOLTAGE_CELL_100:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_100;
        poll_pid = PID_CELLVOLTAGE_CELL_101;
        break;
      case PID_CELLVOLTAGE_CELL_101:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_101;
        poll_pid = PID_CELLVOLTAGE_CELL_102;
        break;
      case PID_CELLVOLTAGE_CELL_102:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_102;
        poll_pid = PID_CELLVOLTAGE_CELL_103;
        break;
      case PID_CELLVOLTAGE_CELL_103:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_103;
        poll_pid = PID_CELLVOLTAGE_CELL_104;
        break;
      case PID_CELLVOLTAGE_CELL_104:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_104;
        poll_pid = PID_CELLVOLTAGE_CELL_105;
        break;
      case PID_CELLVOLTAGE_CELL_105:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_105;
        poll_pid = PID_CELLVOLTAGE_CELL_106;
        break;
      case PID_CELLVOLTAGE_CELL_106:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_106;
        poll_pid = PID_CELLVOLTAGE_CELL_107;
        break;
      case PID_CELLVOLTAGE_CELL_107:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_107;
        poll_pid = PID_CELLVOLTAGE_CELL_108;
        break;
      case PID_CELLVOLTAGE_CELL_108:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_108;
        poll_pid = PID_SOC;
        break;
      default:
        poll_pid = PID_SOC;
        break;
    }

    transmit_can(&MEB_POLLING_FRAME, can_config.battery);
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis10 = currentMillis;
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Volkswagen Group MEB platform battery selected");
#endif
  datalayer.battery.info.number_of_cells = 108;  //Startup in 108S mode. We figure out the actual count later.
  datalayer.battery.info.max_design_voltage_dV = 4536;
  datalayer.battery.info.min_design_voltage_dV = 2436;
}

#endif
