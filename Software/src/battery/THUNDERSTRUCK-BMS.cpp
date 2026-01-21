#include "THUNDERSTRUCK-BMS.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void ThunderstruckBMS::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer.battery.status.soh_pptt = 9900;  //Not in CAN data, hardcoded to 99%

  datalayer.battery.status.voltage_dV = packvoltage_dV;

  datalayer.battery.status.current_dA = pack_curent_dA;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = DCLMax * (packvoltage_dV / 10);

  datalayer.battery.status.max_charge_power_W = CCLMax * (packvoltage_dV / 10);

  datalayer.battery.status.cell_max_voltage_mV = highest_cell_voltage / 10;

  datalayer.battery.status.cell_min_voltage_mV = lowest_cell_voltage / 10;

  datalayer.battery.status.temperature_min_dC = lowest_cell_temperature * 10;

  datalayer.battery.status.temperature_max_dC = highest_cell_temperature * 10;
}

void ThunderstruckBMS::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x14ffcfd0:  // Temperatures
    case 0x14ffced0:
    case 0x14ffcdd0:
    case 0x14ffccd0:
    case 0x14ffcbd0:
    case 0x14ffcad0:
    case 0x14ffc9d0:
    case 0x14ffc8d0:
    case 0x14ffc7d0:
    case 0x14ffc6d0:
    case 0x14ffc5d0:
    case 0x14ffc4d0:
    case 0x14ffc3d0:
    case 0x14ffc2d0:
    case 0x14ffc1d0:
    case 0x14ffc0d0:
      //Mux = frame0
      //ID = 0-F
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ffafd0:  // Cellvoltages
    case 0x14ffaed0:
    case 0x14ffadd0:
    case 0x14ffacd0:
    case 0x14ffabd0:
    case 0x14ffaad0:
    case 0x14ffa9d0:
    case 0x14ffa8d0:
    case 0x14ffa7d0:
    case 0x14ffa6d0:
    case 0x14ffa5d0:
    case 0x14ffa4d0:
    case 0x14ffa3d0:
    case 0x14ffa2d0:
    case 0x14ffa1d0:
    case 0x14ffa0d0:
      //Mux = frame0
      //ID = 0-F
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff27d0:  //Cell internal resistance
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff26d0:  //OCV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff25d0:  //DCL / CCL
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      DCLMin = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      CCLMin = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      DCLMax = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      CCLMax = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      break;
    case 0x14ff24d0:  //SOC
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC = rx_frame.data.u8[1];
      break;
    case 0x14ff23d0:  //Minmax temperatures
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      lowest_cell_temperature = rx_frame.data.u8[2];
      highest_cell_temperature = rx_frame.data.u8[3];
      break;
    case 0x14ff22d0:  //Minmax cellvoltages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      lowest_cell_voltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      highest_cell_voltage = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      break;
    case 0x14ff21d0:  //Packvoltage
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      packvoltage_dV = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      pack_curent_dA = (int16_t)((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      break;
    case 0x14ff20d0:  //Active faults
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff1ad0:  //GM12-15
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff19d0:  //GM11-9
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff18d0:  //GM0-5
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff12d0:  //HVCC, LVCC
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff11d0:  //HVC LVC
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff10d0:  //Firmware version
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff06d0:  //Float status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff05d0:  //Finishing status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff04d0:  //Charger status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff03d0:  //Line EVSE wait
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff02d0:  //Max status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff01d0:  //Charger Model
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x14ff00d0:  //Balancing status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}
void ThunderstruckBMS::transmit_can(unsigned long currentMillis) {

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    transmit_can_frame(&THUND_14efd0d8);
    //TODO, should 14ebd0d8 be sent also?
  }
}

void ThunderstruckBMS::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
