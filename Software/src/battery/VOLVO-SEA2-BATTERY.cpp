#include "VOLVO-SEA2-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void VolvoSea2Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct paramete,rs used for the inverter

  /*
  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;
  */

  if (datalayer.battery.info.chemistry == LFP) {  //If user configured LFP in use (or we autodetected it), switch to it
    datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_LFP_WH;
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_LFP_120S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_LFP_120S_DV;
    datalayer.battery.info.number_of_cells = 120;
  } else if (datalayer.battery.info.chemistry ==
             NMC) {  //If user configured NCM in use (or we autodetected it), switch to it
    datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_NCM_WH;
    datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_NCM_107S_DV;
    datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_NCM_107S_DV;
    datalayer.battery.info.number_of_cells = 107;
  }
}

void VolvoSea2Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x53:  //100ms
      break;
    case 0x140:  //20ms
      break;
    case 0x141:  //20ms
      break;
    case 0x142:  //20ms
      break;
    case 0x143:  //20ms
      break;
    case 0x145:  //100ms
      break;
    case 0x175:  //100ms
      break;
    case 0x17B:  //100ms
      break;
    case 0x178:  //70ms
      break;
    case 0x183:  //100ms
      break;
    case 0x18A:  //100ms
      break;
    case 0x286:  //120ms
      break;
    case 0x288:  //100ms
      break;
    case 0x290:  //100ms
      break;
    case 0x293:  //?ms
      break;
    case 0x302:  //?ms
      break;
    case 0x309:  //?ms
      break;
    case 0x313:  //?ms
      break;
    case 0x315:  //?ms
      break;
    default:
      break;
  }
}

void VolvoSea2Battery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&VOLVO_536);  //Send 0x536 Network managing frame to keep BMS alive
    transmit_can_frame(&VOLVO_372);  //Send 0x372 ECMAmbientTempCalculated
  }
}

void VolvoSea2Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.total_capacity_Wh = MAX_CAPACITY_NCM_WH;               //Startout in NCM mode
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_NCM_107S_DV;  //Startout with max allowed range
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_NCM_107S_DV;  //Startout with min allowed range
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
