#include "../include.h"
#ifdef CMFA_EV_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "CMFA-EV-BATTERY.h"

/*
TODO:

/*

/* Do not change code below unless you are sure what you are doing */

CAN_frame ZOE_423 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x423,
                     .data = {0x07, 0x1d, 0x00, 0x02, 0x5d, 0x80, 0x5d, 0xc8}};

static unsigned long previousMillis100 = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.real_soc;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.voltage_dV;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.status.cell_max_voltage_mV;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //These frames are transmitted by the battery
    case 0x127:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x431:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5A9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5AB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis100 >= INTERVAL_100_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis100));
    }
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "CMFA platform 26.8/27.4kWh", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif  //CMFA_EV_BATTERY
