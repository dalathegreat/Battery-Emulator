#include "CMP-SMART-CAR-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For More Battery Info page
#include "../devboard/utils/events.h"

/* TODO:
Everything
*/

/* Do not change code below unless you are sure what you are doing */
void CmpSmartCarBattery::update_values() {

  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  temp_min = temperature_sensors[0];
  temp_max = temperature_sensors[0];

  // Loop through the array to find min and max, ignoring 0 values
  for (int i = 0; i < 16; i++) {
    if (temperature_sensors[i] != 0) {  // Ignore zero values
      if (temperature_sensors[i] < temp_min) {
        temp_min = temperature_sensors[i];
      }
      if (temperature_sensors[i] > temp_max) {
        temp_max = temperature_sensors[i];
      }
    }
  }

  datalayer.battery.status.temperature_min_dC = temp_min * 10;

  datalayer.battery.status.temperature_max_dC = temp_max * 10;

  // Initialize min and max, lets find which cells are min and max!
  uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
  uint16_t max_cell_mv_value = 0;
  // Loop to find the min and max while ignoring zero values
  for (uint8_t i = 0; i < datalayer.battery.info.number_of_cells; ++i) {
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

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cell_voltages_mV, 100 * sizeof(uint16_t));
}

void CmpSmartCarBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x205:  //00 00 F0 03 35 80 94 45
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame7 is a counter, highbyte F-0, lowbyte 0-F
      break;
    case 0x235:  //0 in all logs
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x275:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame7 is a counter, highbyte F-0, lowbyte 0-F
      break;
    case 0x285:  //0 in all logs
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame7 is a counter, highbyte F-0, lowbyte 0-F
      break;
    case 0x2A5:  //0 in all logs
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x325:  //70 00 F8 42 80 EA 60 0B
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame7 is a counter, highbyte F-0, lowbyte 0-F
      break;
    case 0x334:  // Cellvoltages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] >> 3);  // Mux goes from 0-25

      if (mux < 25) {  // Only process valid cell data frames (0-24)
        uint8_t base_index = mux * 4;

        cell_voltages_mV[base_index + 0] = ((rx_frame.data.u8[1] << 4) | (rx_frame.data.u8[2] >> 4)) * 4;
        cell_voltages_mV[base_index + 1] = ((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[3];
        cell_voltages_mV[base_index + 2] = ((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4)) * 4;
        cell_voltages_mV[base_index + 3] = ((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[7];
      }
      break;
    case 0x335:  //00 03 9A D5 F0 00 00 85
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame7 is a counter, highbyte F-0, lowbyte 0-F
      break;
    case 0x385:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3F4:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];
      if (mux == 0) {
        temperature_sensors[0] = (rx_frame.data.u8[1] - 40);
        temperature_sensors[1] = (rx_frame.data.u8[2] - 40);
        temperature_sensors[2] = (rx_frame.data.u8[3] - 40);
        temperature_sensors[3] = (rx_frame.data.u8[4] - 40);
        temperature_sensors[4] = (rx_frame.data.u8[5] - 40);
        temperature_sensors[5] = (rx_frame.data.u8[6] - 40);
        temperature_sensors[6] = (rx_frame.data.u8[7] - 40);
      } else if (mux == 0x20) {
        temperature_sensors[7] = (rx_frame.data.u8[1] - 40);
        temperature_sensors[8] = (rx_frame.data.u8[2] - 40);
        temperature_sensors[9] = (rx_frame.data.u8[3] - 40);
        temperature_sensors[10] = (rx_frame.data.u8[4] - 40);
        temperature_sensors[11] = (rx_frame.data.u8[5] - 40);
        temperature_sensors[12] = (rx_frame.data.u8[6] - 40);
        temperature_sensors[13] = (rx_frame.data.u8[7] - 40);
      } else if (mux == 0x40) {
        temperature_sensors[14] = (rx_frame.data.u8[1] - 40);
        temperature_sensors[15] = (rx_frame.data.u8[2] - 40);
      }
      break;
    case 0x434:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x435:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x455:  //64 00 00 1C 20 64 E4 00 static
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x485:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x494:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x535:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x543:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x583:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x595:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5B5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x615:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x625:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x665:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x575:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x694:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x795:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7B5:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void CmpSmartCarBattery::transmit_can(unsigned long currentMillis) {

  // Send 1s periodic CAN Message simulating the car still being attached
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    //transmit_can_frame(&ECMP_55F);
  }
}

void CmpSmartCarBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 100;
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.total_capacity_Wh = 41400;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_100S_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_100S_DV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
