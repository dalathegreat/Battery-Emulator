#include "BYD-ATTO-3-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/* Notes
SOC% by default is now ESTIMATED.
If you have a crash-locked pack, See the Wiki for more info on how to attempt an unlock
After battery has been unlocked, you can remove the "USE_ESTIMATED_SOC" from the BYD-ATTO-3-BATTERY.h file
*/

#define POLL_FOR_BATTERY_VOLTAGE 0x0008
#define POLL_FOR_BATTERY_CURRENT 0x0009
#define POLL_FOR_LOWEST_TEMP_CELL 0x002f
#define POLL_FOR_HIGHEST_TEMP_CELL 0x0031
#define POLL_FOR_BATTERY_PACK_AVG_TEMP 0x0032
#define POLL_FOR_BATTERY_CELL_MV_MAX 0x002D
#define POLL_FOR_BATTERY_CELL_MV_MIN 0x002B
#define UNKNOWN_POLL_0 0x1FFE  //0x64 19 C4 3B
#define UNKNOWN_POLL_1 0x1FFC  //0x72 1F C4 3B
#define POLL_MAX_CHARGE_POWER 0x000A
#define UNKNOWN_POLL_3 0x000B  //0x00B1 (177 interesting!)
#define UNKNOWN_POLL_4 0x000E  //0x0B27 (2855 interesting!)
#define POLL_TOTAL_CHARGED_AH 0x000F
#define POLL_TOTAL_DISCHARGED_AH 0x0010
#define POLL_TOTAL_CHARGED_KWH 0x0011
#define POLL_TOTAL_DISCHARGED_KWH 0x0012
#define UNKNOWN_POLL_9 0x0004   //0x0034 (52 interesting!)
#define UNKNOWN_POLL_10 0x002A  //0x5B
#define UNKNOWN_POLL_11 0x002E  //0x08 (probably module number, or cell number?)
#define UNKNOWN_POLL_12 0x002C  //0x43
#define UNKNOWN_POLL_13 0x0030  //0x01 (probably module number, or cell number?)
#define POLL_MODULE_1_LOWEST_MV_NUMBER 0x016C
#define POLL_MODULE_1_LOWEST_CELL_MV 0x016D
#define POLL_MODULE_1_HIGHEST_MV_NUMBER 0x016E
#define POLL_MODULE_1_HIGH_CELL_MV 0x016F
#define POLL_MODULE_1_HIGH_TEMP 0x0171
#define POLL_MODULE_1_LOW_TEMP 0x0173
#define POLL_MODULE_2_LOWEST_MV_NUMBER 0x0174
#define POLL_MODULE_2_LOWEST_CELL_MV 0x0175
#define POLL_MODULE_2_HIGHEST_MV_NUMBER 0x0176
#define POLL_MODULE_2_HIGH_CELL_MV 0x0177
#define POLL_MODULE_2_HIGH_TEMP 0x0179
#define POLL_MODULE_2_LOW_TEMP 0x017B
#define POLL_MODULE_3_LOWEST_MV_NUMBER 0x017C
#define POLL_MODULE_3_LOWEST_CELL_MV 0x017D
#define POLL_MODULE_3_HIGHEST_MV_NUMBER 0x017E
#define POLL_MODULE_3_HIGH_CELL_MV 0x017F
#define POLL_MODULE_3_HIGH_TEMP 0x0181
#define POLL_MODULE_3_LOW_TEMP 0x0183
#define POLL_MODULE_4_LOWEST_MV_NUMBER 0x0184
#define POLL_MODULE_4_LOWEST_CELL_MV 0x0185
#define POLL_MODULE_4_HIGHEST_MV_NUMBER 0x0186
#define POLL_MODULE_4_HIGH_CELL_MV 0x0187
#define POLL_MODULE_4_HIGH_TEMP 0x0189
#define POLL_MODULE_4_LOW_TEMP 0x018B
#define POLL_MODULE_5_LOWEST_MV_NUMBER 0x018C
#define POLL_MODULE_5_LOWEST_CELL_MV 0x018D
#define POLL_MODULE_5_HIGHEST_MV_NUMBER 0x018E
#define POLL_MODULE_5_HIGH_CELL_MV 0x018F
#define POLL_MODULE_5_HIGH_TEMP 0x0191
#define POLL_MODULE_5_LOW_TEMP 0x0193
#define POLL_MODULE_6_LOWEST_MV_NUMBER 0x0194
#define POLL_MODULE_6_LOWEST_CELL_MV 0x0195
#define POLL_MODULE_6_HIGHEST_MV_NUMBER 0x0196
#define POLL_MODULE_6_HIGH_CELL_MV 0x0197
#define POLL_MODULE_6_HIGH_TEMP 0x0199
#define POLL_MODULE_6_LOW_TEMP 0x019B
#define POLL_MODULE_7_LOWEST_MV_NUMBER 0x019C
#define POLL_MODULE_7_LOWEST_CELL_MV 0x019D
#define POLL_MODULE_7_HIGHEST_MV_NUMBER 0x019E
#define POLL_MODULE_7_HIGH_CELL_MV 0x019F
#define POLL_MODULE_7_HIGH_TEMP 0x01A1
#define POLL_MODULE_7_LOW_TEMP 0x01A3
#define POLL_MODULE_8_LOWEST_MV_NUMBER 0x01A4
#define POLL_MODULE_8_LOWEST_CELL_MV 0x01A5
#define POLL_MODULE_8_HIGHEST_MV_NUMBER 0x01A6
#define POLL_MODULE_8_HIGH_CELL_MV 0x01A7
#define POLL_MODULE_8_HIGH_TEMP 0x01A9
#define POLL_MODULE_8_LOW_TEMP 0x01AB
#define POLL_MODULE_9_LOWEST_MV_NUMBER 0x01AC
#define POLL_MODULE_9_LOWEST_CELL_MV 0x01AD
#define POLL_MODULE_9_HIGHEST_MV_NUMBER 0x01AE
#define POLL_MODULE_9_HIGH_CELL_MV 0x01AF
#define POLL_MODULE_9_HIGH_TEMP 0x01B1
#define POLL_MODULE_9_LOW_TEMP 0x01B3
#define POLL_MODULE_10_LOWEST_MV_NUMBER 0x01B4
#define POLL_MODULE_10_LOWEST_CELL_MV 0x01B5
#define POLL_MODULE_10_HIGHEST_MV_NUMBER 0x01B6
#define POLL_MODULE_10_HIGH_CELL_MV 0x01B7
#define POLL_MODULE_10_HIGH_TEMP 0x01B9
#define POLL_MODULE_10_LOW_TEMP 0x01BB

#define ESTIMATED 0
#define MEASURED 1

// Define the data points for %SOC depending on pack voltage
const uint8_t numPoints = 28;
const uint16_t SOC[numPoints] = {10000, 9985, 9970, 9730, 9490, 8980, 8470, 8110, 7750, 7270, 6790, 6145, 5500, 5200,
                                 4900,  4405, 3910, 3455, 3000, 2640, 2280, 1940, 1600, 1040, 480,  240,  120,  0};

const uint16_t voltage_extended[numPoints] = {4300, 4250, 4230, 4205, 4180, 4175, 4171, 4170, 4169, 4164,
                                              4160, 4145, 4130, 4125, 4121, 4120, 4119, 4109, 4100, 4085,
                                              4070, 4050, 4030, 3990, 3950, 3875, 3840, 3800};

const uint16_t voltage_standard[numPoints] = {3570, 3552, 3485, 3464, 3443, 3439, 3435, 3434, 3433, 3429,
                                              3425, 3412, 3400, 3396, 3392, 3391, 3390, 3382, 3375, 3362,
                                              3350, 3332, 3315, 3282, 3250, 3195, 3170, 3140};

uint16_t estimateSOCextended(uint16_t packVoltage) {  // Linear interpolation function
  if (packVoltage >= voltage_extended[0]) {
    return SOC[0];
  }
  if (packVoltage <= voltage_extended[numPoints - 1]) {
    return SOC[numPoints - 1];
  }

  for (int i = 1; i < numPoints; ++i) {
    if (packVoltage >= voltage_extended[i]) {
      double t = (packVoltage - voltage_extended[i]) / (voltage_extended[i - 1] - voltage_extended[i]);
      return SOC[i] + t * (SOC[i - 1] - SOC[i]);
    }
  }
  return 0;  // Default return for safety, should never reach here
}

uint16_t estimateSOCstandard(uint16_t packVoltage) {  // Linear interpolation function
  if (packVoltage >= voltage_standard[0]) {
    return SOC[0];
  }
  if (packVoltage <= voltage_standard[numPoints - 1]) {
    return SOC[numPoints - 1];
  }

  for (int i = 1; i < numPoints; ++i) {
    if (packVoltage >= voltage_standard[i]) {
      double t = (packVoltage - voltage_standard[i]) / (voltage_standard[i - 1] - voltage_standard[i]);
      return SOC[i] + t * (SOC[i - 1] - SOC[i]);
    }
  }
  return 0;  // Default return for safety, should never reach here
}

void BydAttoBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  if (BMS_voltage > 0) {
    datalayer_battery->status.voltage_dV = BMS_voltage * 10;
  }

  if (battery_type == EXTENDED_RANGE) {
    battery_estimated_SOC = estimateSOCextended(datalayer_battery->status.voltage_dV);
  }
  if (battery_type == STANDARD_RANGE) {
    battery_estimated_SOC = estimateSOCstandard(datalayer_battery->status.voltage_dV);
  }

  if (SOC_method == MEASURED) {
    // Pack is not crashed, we can use periodically transmitted SOC
    datalayer_battery->status.real_soc = battery_highprecision_SOC * 10;
  } else {
    // When the battery is crashed hard, it locks itself and SOC becomes unavailable.
    // We instead estimate the SOC% based on the battery voltage.
    // This is a bad solution, you wont be able to use 100% of the battery
    datalayer_battery->status.real_soc = battery_estimated_SOC;
  }

  datalayer_battery->status.current_dA = -BMS_current;

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = MAXPOWER_DISCHARGE_W;  //TODO: Map from CAN later on

  datalayer_battery->status.max_charge_power_W = BMS_allowed_charge_power * 10;  //TODO: Scaling unknown, *10 best guess

  datalayer_battery->status.cell_max_voltage_mV = BMS_highest_cell_voltage_mV;

  datalayer_battery->status.cell_min_voltage_mV = BMS_lowest_cell_voltage_mV;

  datalayer_battery->status.total_discharged_battery_Wh = BMS_total_discharged_kwh * 1000;
  datalayer_battery->status.total_charged_battery_Wh = BMS_total_charged_kwh * 1000;

  //Map all cell voltages to the global array
  memcpy(datalayer_battery->status.cell_voltages_mV, battery_cellvoltages, CELLCOUNT_EXTENDED * sizeof(uint16_t));

  // Check if we are on Standard range or Extended range battery.
  // We use a variety of checks to ensure we catch a potential Standard range battery
  if ((battery_cellvoltages[125] > 0) && (battery_type == NOT_DETERMINED_YET)) {
    battery_type = EXTENDED_RANGE;
  }
  if ((battery_cellvoltages[104] == 4095) && (battery_type == NOT_DETERMINED_YET)) {
    battery_type = STANDARD_RANGE;  //This cell reading is always 4095 on Standard range
  }
  if ((battery_daughterboard_temperatures[9] == 215) && (battery_type == NOT_DETERMINED_YET)) {
    battery_type = STANDARD_RANGE;  //Sensor 10 is missing on Standard range
  }
  if ((battery_daughterboard_temperatures[8] == 215) && (battery_type == NOT_DETERMINED_YET)) {
    battery_type = STANDARD_RANGE;  //Sensor 9 is missing on Standard range
  }

  switch (battery_type) {
    case STANDARD_RANGE:
      datalayer_battery->info.total_capacity_Wh = 50000;
      datalayer_battery->info.number_of_cells = CELLCOUNT_STANDARD;
      datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_STANDARD_DV;
      datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_STANDARD_DV;
      break;
    case EXTENDED_RANGE:
      datalayer_battery->info.total_capacity_Wh = 60000;
      datalayer_battery->info.number_of_cells = CELLCOUNT_EXTENDED;
      datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_EXTENDED_DV;
      datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_EXTENDED_DV;
      break;
    case NOT_DETERMINED_YET:
    default:
      //Do nothing
      break;
  }

#ifdef SKIP_TEMPERATURE_SENSOR_NUMBER
  // Initialize min and max variables for temperature calculation
  battery_calc_min_temperature = battery_daughterboard_temperatures[0];
  battery_calc_max_temperature = battery_daughterboard_temperatures[0];

  // Loop through the array of 10x daughterboard temps to find the smallest and largest values
  // Note, it is possible for user to skip using a faulty sensor in the .h file
  if (SKIP_TEMPERATURE_SENSOR_NUMBER == 1) {  //If sensor 1 is skipped, init minmax to sensor 2
    battery_calc_min_temperature = battery_daughterboard_temperatures[1];
    battery_calc_max_temperature = battery_daughterboard_temperatures[1];
  }
  for (int i = 1; i < 10; i++) {
    if (i == (SKIP_TEMPERATURE_SENSOR_NUMBER - 1)) {
      i++;
    }
    if (battery_daughterboard_temperatures[i] < battery_calc_min_temperature) {
      battery_calc_min_temperature = battery_daughterboard_temperatures[i];
    }
    if (battery_daughterboard_temperatures[i] > battery_calc_max_temperature) {
      battery_calc_max_temperature = battery_daughterboard_temperatures[i];
    }
  }
  //Write the result to datalayer
  if ((battery_calc_min_temperature != 0) && (battery_calc_max_temperature != 0)) {
    //Avoid triggering high delta if only one of the values is available
    datalayer_battery->status.temperature_min_dC = battery_calc_min_temperature * 10;
    datalayer_battery->status.temperature_max_dC = battery_calc_max_temperature * 10;
  }
#else   //User does not need filtering out a broken sensor, just use the min-max the BMS sends
  if ((BMS_lowest_cell_temperature != 0) && (BMS_highest_cell_temperature != 0)) {
    //Avoid triggering high delta if only one of the values is available
    datalayer_battery->status.temperature_min_dC = BMS_lowest_cell_temperature * 10;
    datalayer_battery->status.temperature_max_dC = BMS_highest_cell_temperature * 10;
  }
#endif  //!SKIP_TEMPERATURE_SENSOR_NUMBER

  // Update webserver datalayer
  if (datalayer_bydatto) {
    datalayer_bydatto->SOC_method = SOC_method;
    datalayer_bydatto->SOC_estimated = battery_estimated_SOC;
    datalayer_bydatto->SOC_highprec = battery_highprecision_SOC;
    datalayer_bydatto->SOC_polled = BMS_SOC;
    datalayer_bydatto->voltage_periodic = battery_voltage;
    datalayer_bydatto->voltage_polled = BMS_voltage;
    datalayer_bydatto->battery_temperatures[0] = battery_daughterboard_temperatures[0];
    datalayer_bydatto->battery_temperatures[1] = battery_daughterboard_temperatures[1];
    datalayer_bydatto->battery_temperatures[2] = battery_daughterboard_temperatures[2];
    datalayer_bydatto->battery_temperatures[3] = battery_daughterboard_temperatures[3];
    datalayer_bydatto->battery_temperatures[4] = battery_daughterboard_temperatures[4];
    datalayer_bydatto->battery_temperatures[5] = battery_daughterboard_temperatures[5];
    datalayer_bydatto->battery_temperatures[6] = battery_daughterboard_temperatures[6];
    datalayer_bydatto->battery_temperatures[7] = battery_daughterboard_temperatures[7];
    datalayer_bydatto->battery_temperatures[8] = battery_daughterboard_temperatures[8];
    datalayer_bydatto->battery_temperatures[9] = battery_daughterboard_temperatures[9];
    datalayer_bydatto->unknown0 = BMS_unknown0;
    datalayer_bydatto->unknown1 = BMS_unknown1;
    datalayer_bydatto->chargePower = BMS_allowed_charge_power;
    datalayer_bydatto->unknown3 = BMS_unknown3;
    datalayer_bydatto->unknown4 = BMS_unknown4;
    datalayer_bydatto->total_charged_ah = BMS_total_charged_ah;
    datalayer_bydatto->total_discharged_ah = BMS_total_discharged_ah;
    datalayer_bydatto->total_charged_kwh = BMS_total_charged_kwh;
    datalayer_bydatto->total_discharged_kwh = BMS_total_discharged_kwh;
    datalayer_bydatto->unknown9 = BMS_unknown9;
    datalayer_bydatto->unknown10 = BMS_unknown10;
    datalayer_bydatto->unknown11 = BMS_unknown11;
    datalayer_bydatto->unknown12 = BMS_unknown12;
    datalayer_bydatto->unknown13 = BMS_unknown13;

    // Update requests from webserver datalayer
    if (datalayer_bydatto->UserRequestCrashReset && stateMachineClearCrash == NOT_RUNNING) {
      stateMachineClearCrash = STARTED;
      datalayer_bydatto->UserRequestCrashReset = false;
    }
  }
}

void BydAttoBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x244:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x245:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] == 0x01) {
        battery_temperature_ambient = (rx_frame.data.u8[4] - 40);
      }
      break;
    case 0x286:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x334:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x338:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x344:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x345:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x347:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x34A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x35E:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x36C:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x438:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43A:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43B:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43C:
      if (rx_frame.data.u8[0] == 0x00) {
        battery_daughterboard_temperatures[0] = (rx_frame.data.u8[1] - 40);
        battery_daughterboard_temperatures[1] = (rx_frame.data.u8[2] - 40);
        battery_daughterboard_temperatures[2] = (rx_frame.data.u8[3] - 40);
        battery_daughterboard_temperatures[3] = (rx_frame.data.u8[4] - 40);
        battery_daughterboard_temperatures[4] = (rx_frame.data.u8[5] - 40);
        battery_daughterboard_temperatures[5] = (rx_frame.data.u8[6] - 40);
      }
      if (rx_frame.data.u8[0] == 0x01) {
        battery_daughterboard_temperatures[6] = (rx_frame.data.u8[1] - 40);
        battery_daughterboard_temperatures[7] = (rx_frame.data.u8[2] - 40);
        battery_daughterboard_temperatures[8] = (rx_frame.data.u8[3] - 40);
        battery_daughterboard_temperatures[9] = (rx_frame.data.u8[4] - 40);
      }
      break;
    case 0x43D:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_frame_index = rx_frame.data.u8[0];

      if (battery_frame_index < (CELLCOUNT_EXTENDED / 3)) {
        uint8_t base_index = battery_frame_index * 3;
        for (uint8_t i = 0; i < 3; i++) {
          battery_cellvoltages[base_index + i] =
              (((rx_frame.data.u8[2 * (i + 1)] & 0x0F) << 8) | rx_frame.data.u8[2 * i + 1]);
        }
      }
      break;
    case 0x444:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_voltage = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[0];
      //battery_temperature_something = rx_frame.data.u8[7] - 40; resides in frame 7
      break;
    case 0x445:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x446:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x447:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_highprecision_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4];  // 03 E0 = 992 = 99.2%
      battery_lowest_temperature = (rx_frame.data.u8[1] - 40);                                //Best guess for now
      battery_highest_temperature = (rx_frame.data.u8[3] - 40);                               //Best guess for now
      break;
    case 0x47B:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x524:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EF:  //OBD2 PID reply from battery
      if (rx_frame.data.u8[0] == 0x10) {
        transmit_can_frame(&ATTO_3_7E7_ACK, can_interface);  //Send next line request
      }
      pid_reply = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      switch (pid_reply) {
        case POLL_FOR_BATTERY_SOC:
          BMS_SOC = rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_VOLTAGE:
          BMS_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_CURRENT:
          BMS_current = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) - 5000;
          break;
        case POLL_FOR_LOWEST_TEMP_CELL:
          BMS_lowest_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_HIGHEST_TEMP_CELL:
          BMS_highest_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_BATTERY_PACK_AVG_TEMP:
          BMS_average_cell_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_FOR_BATTERY_CELL_MV_MAX:
          BMS_highest_cell_voltage_mV = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_FOR_BATTERY_CELL_MV_MIN:
          BMS_lowest_cell_voltage_mV = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_0:
          BMS_unknown0 = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                          rx_frame.data.u8[4]);
          break;
        case UNKNOWN_POLL_1:
          BMS_unknown1 = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                          rx_frame.data.u8[4]);
          break;
        case POLL_MAX_CHARGE_POWER:
          BMS_allowed_charge_power = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_3:
          BMS_unknown3 = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_4:
          BMS_unknown4 = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_CHARGED_AH:
          BMS_total_charged_ah = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_DISCHARGED_AH:
          BMS_total_discharged_ah = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_CHARGED_KWH:
          BMS_total_charged_kwh = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case POLL_TOTAL_DISCHARGED_KWH:
          BMS_total_discharged_kwh = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_9:
          BMS_unknown9 = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_10:
          BMS_unknown10 = rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_11:
          BMS_unknown11 = rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_12:
          BMS_unknown12 = rx_frame.data.u8[4];
          break;
        case UNKNOWN_POLL_13:
          BMS_unknown13 = rx_frame.data.u8[4];
          break;
        default:  //Unrecognized reply
          break;
      }
      break;
    default:
      break;
  }
}

void BydAttoBattery::transmit_can(unsigned long currentMillis) {
  //Send 50ms message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;

    // Set close contactors to allowed (Useful for crashed packs, started via contactor control thru GPIO)
    if (allows_contactor_closing) {
      if (datalayer_battery->status.bms_status == ACTIVE) {
        *allows_contactor_closing = true;
      } else {  // Fault state, open contactors!
        *allows_contactor_closing = false;
      }
    }

    counter_50ms++;
    if (counter_50ms > 23) {
      ATTO_3_12D.data.u8[2] = 0x00;  // Goes from 02->00
      ATTO_3_12D.data.u8[3] = 0x22;  // Goes from A0->22
      ATTO_3_12D.data.u8[5] = 0x31;  // Goes from 71->31
    }

    // Update the counters in frame 6 & 7 (they are not in sync)
    if (frame6_counter == 0x0) {
      frame6_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame6_counter--;  // Decrement the counter
    }
    if (frame7_counter == 0x0) {
      frame7_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame7_counter--;  // Decrement the counter
    }

    ATTO_3_12D.data.u8[6] = (0x0F | (frame6_counter << 4));
    ATTO_3_12D.data.u8[7] = (0x09 | (frame7_counter << 4));

    transmit_can_frame(&ATTO_3_12D, can_interface);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if (counter_100ms < 100) {
      counter_100ms++;
    }

    if (counter_100ms > 3) {
      ATTO_3_441.data.u8[4] = 0x9D;
      ATTO_3_441.data.u8[5] = 0x01;
      ATTO_3_441.data.u8[6] = 0xFF;
      ATTO_3_441.data.u8[7] = 0xF5;
    }

    transmit_can_frame(&ATTO_3_441, can_interface);
    switch (stateMachineClearCrash) {
      case STARTED:
        ATTO_3_7E7_CLEAR_CRASH.data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH, can_interface);
        stateMachineClearCrash = RUNNING_STEP_1;
        break;
      case RUNNING_STEP_1:
        ATTO_3_7E7_CLEAR_CRASH.data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH, can_interface);
        stateMachineClearCrash = RUNNING_STEP_2;
        break;
      case RUNNING_STEP_2:
        ATTO_3_7E7_CLEAR_CRASH.data = {0x03, 0x19, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&ATTO_3_7E7_CLEAR_CRASH, can_interface);
        stateMachineClearCrash = NOT_RUNNING;
        break;
      case NOT_RUNNING:
        break;
      default:
        break;
    }
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    switch (poll_state) {
      case POLL_FOR_BATTERY_SOC:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_SOC & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_SOC & 0x00FF);
        poll_state = POLL_FOR_BATTERY_VOLTAGE;
        break;
      case POLL_FOR_BATTERY_VOLTAGE:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_VOLTAGE & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_VOLTAGE & 0x00FF);
        poll_state = POLL_FOR_BATTERY_CURRENT;
        break;
      case POLL_FOR_BATTERY_CURRENT:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_CURRENT & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_CURRENT & 0x00FF);
        poll_state = POLL_FOR_LOWEST_TEMP_CELL;
        break;
      case POLL_FOR_LOWEST_TEMP_CELL:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_LOWEST_TEMP_CELL & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_LOWEST_TEMP_CELL & 0x00FF);
        poll_state = POLL_FOR_HIGHEST_TEMP_CELL;
        break;
      case POLL_FOR_HIGHEST_TEMP_CELL:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_HIGHEST_TEMP_CELL & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_HIGHEST_TEMP_CELL & 0x00FF);
        poll_state = POLL_FOR_BATTERY_PACK_AVG_TEMP;
        break;
      case POLL_FOR_BATTERY_PACK_AVG_TEMP:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_PACK_AVG_TEMP & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_PACK_AVG_TEMP & 0x00FF);
        poll_state = POLL_FOR_BATTERY_CELL_MV_MAX;
        break;
      case POLL_FOR_BATTERY_CELL_MV_MAX:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_CELL_MV_MAX & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_CELL_MV_MAX & 0x00FF);
        poll_state = POLL_FOR_BATTERY_CELL_MV_MIN;
        break;
      case POLL_FOR_BATTERY_CELL_MV_MIN:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_FOR_BATTERY_CELL_MV_MIN & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_FOR_BATTERY_CELL_MV_MIN & 0x00FF);
        poll_state = UNKNOWN_POLL_0;
        break;
      case UNKNOWN_POLL_0:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_0 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_0 & 0x00FF);
        poll_state = UNKNOWN_POLL_1;
        break;
      case UNKNOWN_POLL_1:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_1 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_1 & 0x00FF);
        poll_state = POLL_MAX_CHARGE_POWER;
        break;
      case POLL_MAX_CHARGE_POWER:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_MAX_CHARGE_POWER & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_MAX_CHARGE_POWER & 0x00FF);
        poll_state = UNKNOWN_POLL_3;
        break;
      case UNKNOWN_POLL_3:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_3 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_3 & 0x00FF);
        poll_state = UNKNOWN_POLL_4;
        break;
      case UNKNOWN_POLL_4:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_4 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_4 & 0x00FF);
        poll_state = POLL_TOTAL_CHARGED_AH;
        break;
      case POLL_TOTAL_CHARGED_AH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_CHARGED_AH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_CHARGED_AH & 0x00FF);
        poll_state = POLL_TOTAL_DISCHARGED_AH;
        break;
      case POLL_TOTAL_DISCHARGED_AH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_DISCHARGED_AH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_DISCHARGED_AH & 0x00FF);
        poll_state = POLL_TOTAL_CHARGED_KWH;
        break;
      case POLL_TOTAL_CHARGED_KWH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_CHARGED_KWH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_CHARGED_KWH & 0x00FF);
        poll_state = POLL_TOTAL_DISCHARGED_KWH;
        break;
      case POLL_TOTAL_DISCHARGED_KWH:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((POLL_TOTAL_DISCHARGED_KWH & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(POLL_TOTAL_DISCHARGED_KWH & 0x00FF);
        poll_state = UNKNOWN_POLL_9;
        break;
      case UNKNOWN_POLL_9:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_9 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_9 & 0x00FF);
        poll_state = UNKNOWN_POLL_10;
        break;
      case UNKNOWN_POLL_10:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_10 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_10 & 0x00FF);
        poll_state = UNKNOWN_POLL_11;
        break;
      case UNKNOWN_POLL_11:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_11 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_11 & 0x00FF);
        poll_state = UNKNOWN_POLL_12;
        break;
      case UNKNOWN_POLL_12:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_12 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_12 & 0x00FF);
        poll_state = UNKNOWN_POLL_13;
        break;
      case UNKNOWN_POLL_13:
        ATTO_3_7E7_POLL.data.u8[2] = (uint8_t)((UNKNOWN_POLL_13 & 0xFF00) >> 8);
        ATTO_3_7E7_POLL.data.u8[3] = (uint8_t)(UNKNOWN_POLL_13 & 0x00FF);
        poll_state = POLL_FOR_BATTERY_SOC;
        break;
      default:
        poll_state = POLL_FOR_BATTERY_SOC;
        break;
    }

    if (stateMachineClearCrash == NOT_RUNNING) {  //Don't poll battery for data if clear crash running
      transmit_can_frame(&ATTO_3_7E7_POLL, can_interface);
    }
  }
}

void BydAttoBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = CELLCOUNT_STANDARD;
  datalayer_battery->info.chemistry = battery_chemistry_enum::LFP;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_EXTENDED_DV;  //Startup in extremes
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_STANDARD_DV;  //We later determine range
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
#ifdef USE_ESTIMATED_SOC  // Initial setup for selected SOC method
  SOC_method = ESTIMATED;
#else
  SOC_method = MEASURED;
#endif
}
