#include "STELLANTIS-PRO-ONE-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

static uint8_t CalculateCRC8SAEJ1850(CAN_frame rx_frame, uint8_t length) {
  uint8_t crc = 0xFF;  // initial value 0xFF
  for (uint8_t j = 0; j < length;
       j++) {  //Length is the number of bytes to calculate CRC for, not including the CRC byte itself!
    crc = crc8_table_SAE_J1850_ZER0[crc ^ rx_frame.data.u8[j]];
  }
  return crc ^ 0xFF;  // final XOR 0xFF
}

void StellantisProOneBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //datalayer.battery.status.real_soc; //TODO: locate
  //datalayer.battery.status.soh_pptt; //TODO: locate
  //datalayer.battery.status.voltage_dV; //TODO: locate
  //datalayer.battery.status.current_dA; //TODO: locate
  //datalayer.battery.status.remaining_capacity_Wh; //TODO: locate
  //datalayer.battery.status.max_discharge_power_W; //TODO: locate
  //datalayer.battery.status.max_charge_power_W; //TODO: locate
  //datalayer.battery.status.cell_max_voltage_mV; //TODO: locate
  //datalayer.battery.status.cell_min_voltage_mV; //TODO: locate
  //datalayer.battery.status.temperature_min_dC; //TODO: locate
  //datalayer.battery.status.temperature_max_dC; //TODO: locate
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String StellantisProOneBattery::get_uds_info_html() {
  String content;
  content.reserve(600);

  // clang-format off
  content << "<h4>Pack voltage: " << pid_pack_voltage << " dV</h4>"
              "<h4>12V voltage: " << lead_acid_voltage << "mV</h4>"
              "<h4>Lowest temperature: " << pid_lowest_temperature << "°C</h4>"
              "<h4>Highest temperature: " << pid_highest_temperature << "°C</h4>";
  // clang-format on

  return content;
}

void StellantisProOneBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x7E7 PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  switch (rx_frame.ID) {
    case 0x95:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0xE0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x107:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      expectedCRC = CalculateCRC8SAEJ1850(rx_frame, 7);
      if (expectedCRC == rx_frame.data.u8[7]) {
        //Message is valid, process it
      } else {
        //CRC error, ignore message
        datalayer.battery.status.CAN_error_counter++;
      }
      break;
    case 0x1D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x220:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x281:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x285:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x306:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x307:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x312:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x354:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x358:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EF:  //UDS reply
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

uint16_t StellantisProOneBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_WELD_CHECK:
      break;
    case PID_CONT_REASON_OPEN:
      break;
    case PID_CONTACTOR_STATUS:
      break;
    case PID_NEG_CONT_CONTROL:
      break;
    case PID_NEG_CONT_STATUS:
      break;
    case PID_POS_CONT_CONTROL:
      break;
    case PID_POS_CONT_STATUS:
      break;
    case PID_CONTACTOR_NEGATIVE:
      break;
    case PID_CONTACTOR_POSITIVE:
      break;
    case PID_PRECHARGE_RELAY_CONTROL:
      break;
    case PID_PRECHARGE_RELAY_STATUS:
      break;
    case PID_RECHARGE_STATUS:
      break;
    case PID_DELTA_TEMPERATURE:
      break;
    case PID_COLDEST_MODULE:
      break;
    case PID_LOWEST_TEMPERATURE:
      pid_lowest_temperature = (int16_t)(value - 40);
      break;
    case PID_AVERAGE_TEMPERATURE:
      break;
    case PID_HIGHEST_TEMPERATURE:
      pid_highest_temperature = (int16_t)(value - 40);
      break;
    case PID_HOTTEST_MODULE:
      break;
    case PID_AVG_CELL_VOLTAGE:
      break;
    case PID_CURRENT:
      break;
    case PID_INSULATION_NEG:
      break;
    case PID_INSULATION_POS:
      break;
    case PID_MAX_CURRENT_10S:
      break;
    case PID_MAX_DISCHARGE_10S:
      break;
    case PID_MAX_DISCHARGE_30S:
      break;
    case PID_MAX_CHARGE_10S:
      break;
    case PID_MAX_CHARGE_30S:
      break;
    case PID_ENERGY_CAPACITY:
      break;
    case PID_HIGH_CELL_NUM:
      break;
    case PID_LOW_CELL_NUM:
      break;
    case PID_SUM_OF_CELLS:
      break;
    case PID_CELL_MIN_CAPACITY:
      break;
    case PID_CELL_VOLTAGE_MEAS_STATUS:
      break;
    case PID_INSULATION_RES:
      break;
    case PID_PACK_VOLTAGE:
      pid_pack_voltage = (uint16_t)value;
      datalayer.battery.status.voltage_dV = (uint16_t)value;
      break;
    case PID_HIGH_CELL_VOLTAGE:
      datalayer.battery.status.cell_max_voltage_mV = (uint16_t)value;
      break;
    case PID_ALL_CELL_VOLTAGES:
      break;
    case PID_LOW_CELL_VOLTAGE:
      datalayer.battery.status.cell_min_voltage_mV = (uint16_t)value;
      break;
    case PID_BATTERY_ENERGY:
      break;
    case PID_CELLBALANCE_STATUS:
      break;
    case PID_CELLBALANCE_HWERR_MASK:
      break;
    case PID_CRASH_COUNTER:
      break;
    case PID_WIRE_CRASH:
      break;
    case PID_CAN_CRASH:
      break;
    case PID_HISTORY_DATA:
      break;
    case PID_LOWSOC_COUNTER:
      break;
    case PID_LAST_CAN_FAILURE_DETAIL:
      break;
    case PID_HW_VERSION_NUM:
      break;
    case PID_SW_VERSION_NUM:
      break;
    case PID_FACTORY_MODE_CONTROL:
      break;
    case PID_BATTERY_SERIAL:
      break;
    case PID_ALL_CELL_SOH:
      break;
    case PID_AUX_FUSE_STATE:
      break;
    case PID_BATTERY_STATE:
      break;
    case PID_PRECHARGE_SHORT_CIRCUIT:
      break;
    case PID_ESERVICE_PLUG_STATE:
      break;
    case PID_MAINFUSE_STATE:
      break;
    case PID_MOST_CRITICAL_FAULT:
      break;
    case PID_CURRENT_TIME:
      break;
    case PID_TIME_SENT_BY_CAR:
      break;
    case PID_12V:
      lead_acid_voltage = (uint16_t)value;
      break;
    case PID_12V_ABNORMAL:
      break;
    case PID_HVIL_IN_VOLTAGE:
      break;
    case PID_HVIL_OUT_VOLTAGE:
      break;
    case PID_HVIL_STATE:
      break;
    case PID_BMS_STATE:
      break;
    case PID_VEHICLE_SPEED:
      break;
    case PID_TIME_SPENT_OVER_55C:
      break;
    case PID_CONTACTOR_CLOSING_COUNTER:
      break;
    case PID_DATE_OF_MANUFACTURE:
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void StellantisProOneBattery::transmit_can(unsigned long currentMillis) {

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    //Counter goes from 0-0xF and starts over
    counter_10ms = (counter_10ms + 1) & 0x0F;
    //Sentmessages counter
    if (sent_10ms_messages < 254) {
      sent_10ms_messages++;
    }

    if (sent_10ms_messages == 37) {
      ONE_108.data.u8[0] = 0x33;
      ONE_108.data.u8[1] = 0x1D;
      ONE_108.data.u8[2] = 0xF0;
    } else if (sent_10ms_messages == 42) {
      ONE_108.data.u8[1] = 0x25;
      ONE_108.data.u8[2] = 0xC0;
    } else {
    }

    ONE_15A.data.u8[2] = counter_10ms;
    ONE_15A.data.u8[3] = CalculateCRC8SAEJ1850(ONE_15A, 3);
    ONE_1D7.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_1D7.data.u8[7] = CalculateCRC8SAEJ1850(ONE_1D7, 7);
    ONE_175.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_175.data.u8[7] = CalculateCRC8SAEJ1850(ONE_175, 7);
    ONE_108.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_108.data.u8[7] = CalculateCRC8SAEJ1850(ONE_108, 7);

    transmit_can_frame(&ONE_15A);
    transmit_can_frame(&ONE_1D7);
    transmit_can_frame(&ONE_175);
    transmit_can_frame(&ONE_108);
    //transmit_can_frame(&ONE_0F2);
    //transmit_can_frame(&ONE_0F0);
    //transmit_can_frame(&ONE_0B4);

    ONE_175.data.u8[3] = 0x2E;
  }

  // Send 20ms CAN Message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    //Counter goes from 0-0xF and starts over
    counter_20ms = (counter_20ms + 1) & 0x0F;
    //Sentmessages counter
    if (sent_20ms_messages < 254) {
      sent_20ms_messages++;
    }

    //Safety checks for Contactor closing
    if ((datalayer.system.status.inverter_allows_contactor_closing == true) &&
        (datalayer.system.status.system_status != FAULT) && (!datalayer.system.info.equipment_stop_active)) {
      ONE_1D8.data.u8[4] = 0x21;
      ONE_1D8.data.u8[5] = 0x4F;
    } else {  //Closing not allowed
      ONE_1D8.data.u8[4] = 0x00;
      ONE_1D8.data.u8[5] = 0x00;
    }

    ONE_1D8.data.u8[6] = (counter_20ms & 0x0F) << 4;
    ONE_1D8.data.u8[7] = CalculateCRC8SAEJ1850(ONE_1D8, 7);

    transmit_can_frame(&ONE_1D8);  // Required for contactor closing
    //transmit_can_frame(&ONE_212);
    //transmit_can_frame(&ONE_160);
  }

  // Send 50ms CAN Message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;
    //transmit_can_frame(&ONE_242);
    //transmit_can_frame(&ONE_240);
    //transmit_can_frame(&ONE_234);
    //transmit_can_frame(&ONE_235);
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //transmit_can_frame(&ONE_3E0);
    //transmit_can_frame(&ONE_3E7);
    //transmit_can_frame(&ONE_3EB);
    //transmit_can_frame(&ONE_3EE);
    //transmit_can_frame(&ONE_320);
    //transmit_can_frame(&ONE_321);
    //transmit_can_frame(&ONE_322);
    //transmit_can_frame(&ONE_323);
  }

  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);
}

void StellantisProOneBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  // UDS: send requests to 0x7E7, accept replies from the BMS on 0x7EF.
  setup_uds(0x7E7, 0x7EF);
  static const uint16_t pid_scan_list[] = {
      PID_WELD_CHECK,
      PID_CONT_REASON_OPEN,
      PID_CONTACTOR_STATUS,
      PID_NEG_CONT_CONTROL,
      PID_NEG_CONT_STATUS,
      PID_POS_CONT_CONTROL,
      PID_POS_CONT_STATUS,
      PID_CONTACTOR_NEGATIVE,
      PID_CONTACTOR_POSITIVE,
      PID_PRECHARGE_RELAY_CONTROL,
      PID_PRECHARGE_RELAY_STATUS,
      PID_RECHARGE_STATUS,
      PID_DELTA_TEMPERATURE,
      PID_COLDEST_MODULE,
      PID_LOWEST_TEMPERATURE,
      PID_AVERAGE_TEMPERATURE,
      PID_HIGHEST_TEMPERATURE,
      PID_HOTTEST_MODULE,
      PID_AVG_CELL_VOLTAGE,
      PID_CURRENT,
      PID_INSULATION_NEG,
      PID_INSULATION_POS,
      PID_MAX_CURRENT_10S,
      PID_MAX_DISCHARGE_10S,
      PID_MAX_DISCHARGE_30S,
      PID_MAX_CHARGE_10S,
      PID_MAX_CHARGE_30S,
      PID_ENERGY_CAPACITY,
      PID_HIGH_CELL_NUM,
      PID_LOW_CELL_NUM,
      PID_SUM_OF_CELLS,
      PID_CELL_MIN_CAPACITY,
      PID_CELL_VOLTAGE_MEAS_STATUS,
      PID_INSULATION_RES,
      PID_PACK_VOLTAGE,
      PID_HIGH_CELL_VOLTAGE,
      PID_ALL_CELL_VOLTAGES,
      PID_LOW_CELL_VOLTAGE,
      PID_BATTERY_ENERGY,
      PID_CELLBALANCE_STATUS,
      PID_CELLBALANCE_HWERR_MASK,
      PID_CRASH_COUNTER,
      PID_WIRE_CRASH,
      PID_CAN_CRASH,
      PID_HISTORY_DATA,
      PID_LOWSOC_COUNTER,
      PID_LAST_CAN_FAILURE_DETAIL,
      PID_HW_VERSION_NUM,
      PID_SW_VERSION_NUM,
      PID_FACTORY_MODE_CONTROL,
      PID_BATTERY_SERIAL,
      PID_ALL_CELL_SOH,
      PID_AUX_FUSE_STATE,
      PID_BATTERY_STATE,
      PID_PRECHARGE_SHORT_CIRCUIT,
      PID_ESERVICE_PLUG_STATE,
      PID_MAINFUSE_STATE,
      PID_MOST_CRITICAL_FAULT,
      PID_CURRENT_TIME,
      PID_TIME_SENT_BY_CAR,
      PID_12V,
      PID_12V_ABNORMAL,
      PID_HVIL_IN_VOLTAGE,
      PID_HVIL_OUT_VOLTAGE,
      PID_HVIL_STATE,
      PID_BMS_STATE,
      PID_VEHICLE_SPEED,
      PID_TIME_SPENT_OVER_55C,
      PID_CONTACTOR_CLOSING_COUNTER,
      PID_DATE_OF_MANUFACTURE,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
