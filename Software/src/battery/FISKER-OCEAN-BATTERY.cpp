#include "FISKER-OCEAN-BATTERY.h"
#include <cstring>  //For unit test
#include "../battery/BATTERIES.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
void FiskerOceanBattery::update_values() {
  /*
  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV;
  
  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.info.max_design_voltage_dV;

  datalayer.battery.info.min_design_voltage_dV;
  */

  datalayer.battery.status.voltage_dV = pack_voltage / 10;

  datalayer.battery.status.temperature_min_dC = cell_temperature_min_C * 10;

  datalayer.battery.status.temperature_max_dC = cell_temperature_max_C * 10;

  datalayer.battery.info.number_of_cells = NUM_CELLS;
}

void FiskerOceanBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x100:  //BBus 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x103:  //BBus 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x110:  //BBus 10ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x180:  //BBus 50ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x181:  //BBus 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x182:  //BBus 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x310:  //BBus 30ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame6 has counter low nibble, 0-F incrementing every frame
      break;
    case 0x0E9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      pack_voltage = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x0EB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0EE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x0F2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x215:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x24A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x278:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x279:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2EE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2EF:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2F9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x330:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x360:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x370:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x372:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cell_temperature_max_C =
          rx_frame.data.u8[4] -
          40;  //Matches with data from 0x6D0 and 0x6D1 frames, so we can use this as the max temperature
      cell_temperature_min_C =
          rx_frame.data.u8[5] -
          40;  //Matches with data from 0x6D0 and 0x6D1 frames, so we can use this as the min temperature
      break;
    case 0x3A0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3A7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x595:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5A7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x63A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x652:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6A0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6A5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6B0 ... 0x6CD: {
      uint8_t frame_offset = rx_frame.ID - CELLVOLTAGE_FRAME_START;

      for (uint8_t i = 0; i < 4; i++) {
        uint16_t raw = (rx_frame.data.u8[i * 2] << 8) | rx_frame.data.u8[i * 2 + 1];

        if (raw == 0xFFFF) {
          // Padding, no cell present in this slot (covers 6C9's 2nd half
          // and all of 6CA..6CD)
          continue;
        }

        uint8_t cell_index = (frame_offset * 4) + i;

        if (cell_index < NUM_CELLS) {
          datalayer.battery.status.cell_voltages_mV[cell_index] = raw / 10;
        }
      }
      break;
    }
    case 0x6D0:  //Temperatures (49 49 48 48 48 48 49 48 )
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //All individual temperature measurements
      break;
    case 0x6D1:  //Temperatures (48 49 48 49 49 48 48 FF )
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D2:  //Temperatures (All FF)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D3:  //Temperatures (All FF)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6D9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DD:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6DF:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x6F4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7E9:  //BMS reply
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if (rx_frame.data.u8[0] < 0x10) {  //One line response
        incoming_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      }

      if (rx_frame.data.u8[0] == 0x10) {      //Multiframe response, send ACK
        transmit_can_frame(&FISKER_PID_ACK);  //Not seen yet
        incoming_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      }

      switch (incoming_poll) {
        case PID_BATTERY_SUM_VOLTAGE:
          datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 10;
          break;
        case PID_BATTERY_CURRENT:
          datalayer.battery.status.current_dA = ((int16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5])) / 10;
          break;
        case PID_BATTERY_CURRENT_VALID:
          break;
        case PID_DISCHARGE_CURR_LIMIT:
          break;
        case PID_CHARGE_CURR_LIMIT:
          break;
        case PID_CHARGE_OVER_CURR_LIMIT:
          break;
        case PID_HALL_SAMPLE_CURRENT:
          break;
        case PID_CSU_SAMPLE_CURRENT:
          break;
        case PID_CSU_CURRENT_STATE:
          break;
        case PID_INLET_WATER_TEMP:
          break;
        case PID_OUTLET_WATER_TEMP:
          break;
        case PID_MAX_BALANCE_CIRCUIT_TEMP:
          break;
        case PID_SA_LVMD_BAL_TEMP_VALID:
          break;
        case PID_MAX_CHIP_TEMP:
          break;
        case PID_SA_LVMD_CHIP_INSIDE_TEMP_VALID:
          break;
        case PID_AVG_MODULE_TEMP:
          break;
        case PID_MAX_MODULE_TEMP:
          break;
        case PID_MIN_MODULE_TEMP:
          break;
        case PID_MAX_MODULE_TEMP_CMC_AND_POINT_PSTN:
          break;
        case PID_MIN_MODULE_TEMP_CMC_AND_POINT_PSTN:
          break;
        case PID_MODULE_TEMP_VALID:
          break;
        case PID_MAX_VOLT_CELL_SOC_PERCENT:
          break;
        case PID_MIN_VOLT_CELL_SOC_PERCENT:
          break;
        case PID_AVG_VOLT_CELL_SOC_PERCENT:
          break;
        case PID_PACK_DISPLAY_SOC_PERCENT:
          break;
        case PID_UNEXPECTED_POWER_DOWN_FAULT:
          break;
        case PID_MODULE_TEMP_DAISYCHAIN_UPDATED:
          break;
        case PID_CELL_VOLT_DAISYCHAIN_UPDATED:
          break;
        case PID_CMC_RESET_ERR_FLAG:
          break;
        case PID_VCU_CRASH_MESSAGE_STATUS:
          break;
        case PID_HARDWARE_SIG_PWM_PERIOD:
          break;
        case PID_HARDWARE_PWM_DUTY_CYCLE:
          break;
        case PID_FORCE_FORBIDDEN_ISO_DETECT_CMD:
          break;
        case PID_ISOLATION_MEAS_STATUS:
          break;
        case PID_ISOLATION_MEAS_STATE:
          break;
        case PID_POS_ISO_MEAS_VOLT_RAW:
          break;
        case PID_NEG_ISO_MEAS_VOLT_RAW:
          break;
        case PID_ISO_MEAS_POS_RES_KOHM:
          break;
        case PID_ISO_MEAS_NEG_RES_KOHM:
          break;
        case PID_BAL_CIRCUIT_OPEN_ERR_CMC_PSTN:
          break;
        case PID_BAL_CIRCUIT_OPEN_ERR_CELL_PSTN:
          break;
        case PID_BAL_CIRCUIT_SHORT_ERR_CMC_PSTN:
          break;
        case PID_BAL_CIRCUIT_SHORT_ERR_CELL_PSTN:
          break;
        case PID_VOLT_OR_CURR_CH0_HIGH_VOLT_MV:
          break;
        case PID_VOLT_OR_CURR_CH1_HIGH_VOLT_MV:
          break;
        case PID_BATTERY_TO_G0_VOLT:
          break;
        case PID_PV_POS_TO_G0_VOLT:
          break;
        case PID_MAIN_POS_TO_G0_VOLT:
          break;
        case PID_MAIN_POS_TO_G1_VOLT:
          break;
        case PID_KL30C_VOLTAGE:
          break;
        case PID_MAX_CELL_VOLT_CMC_AND_POINT_PSTN:
          break;
        case PID_MIN_CELL_VOLT_CMC_AND_POINT_PSTN:
          break;
        case PID_AVG_CELL_VOLTAGE:
          break;
        case PID_MAX_CELL_VOLTAGE:
          break;
        case PID_MIN_CELL_VOLTAGE:
          break;
        case PID_CELL_VOLT_VALID:
          break;
        case PID_PV_POS_CONTACTOR_AGING:
          break;
        case PID_PR_NEG_CONTACTOR_AGING:
          break;
        case PID_TIME_STAMP:
          break;
        case PID_VEHICLE_SPEED:
          break;
        case PID_ST_MIN:
          break;
        case PID_APPLICATION_SOFTWARE_FINGERPRINT:
          break;
        case PID_VEHICLE_IDENTIFICATION_NUMBER:
          break;
        default:
          break;
      }
      break;
    default:
      //logging.printf("Received unexpected CAN frame with ID: 0x%03X\n", rx_frame.ID);
      break;
  }
}

void FiskerOceanBattery::transmit_can(unsigned long currentMillis) {
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    // Update current poll from the array
    currentpoll = poll_commands[poll_index];
    poll_index = (poll_index + 1) % 36;

    FISKER_PID_REQUEST.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
    FISKER_PID_REQUEST.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

    transmit_can_frame(&FISKER_PID_REQUEST);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    if (UserRequestDTCreset) {
      transmit_can_frame(&FISKER_DTC_RESET);
      UserRequestDTCreset = false;
    }
  }
}

void FiskerOceanBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_113S_DV;  //Startout wide
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_106S_DV;  //Narrow later if we can detect pack size
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
