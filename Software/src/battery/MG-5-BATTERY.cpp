#include "MG-5-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../include.h"

/* TODO: 
- Get contactor closing working
- Figure out which CAN messages need to be sent towards the battery to keep it alive
- Map all values from battery CAN messages
- Most important ones 
*/

void Mg5Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;
}

void Mg5Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x171:  //Following messages were detected on a MG5 battery BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN
      break;
    case 0x172:
      break;
    case 0x173:
      break;
    case 0x293:
      break;
    case 0x295:
      break;
    case 0x297:
      break;
    case 0x29B:
      break;
    case 0x29C:
      break;
    case 0x2A0:
      break;
    case 0x2A2:
      break;
    case 0x322:
      break;
    case 0x334:
      break;
    case 0x33F:
      break;
    case 0x391:
      break;
    case 0x393:
      break;
    case 0x3AB:
      break;
    case 0x3AC:
      break;
    case 0x3B8:
      break;
    case 0x3BA:
      break;
    case 0x3BC:
      break;
    case 0x3BE:
      break;
    case 0x3C0:
      break;
    case 0x3C2:
      break;
    case 0x400:
      break;
    case 0x402:
      break;
    case 0x418:
      break;
    case 0x44C:
      break;
    case 0x620:
      break;
    default:
      break;
  }
}
void Mg5Battery::transmit_can(unsigned long currentMillis) {
  //Send 10ms message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    transmit_can_frame(&MG_5_100, can_config.battery);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //transmit_can_frame(&MG_5_100, can_config.battery);
  }
}

void Mg5Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "MG 5 battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
}
