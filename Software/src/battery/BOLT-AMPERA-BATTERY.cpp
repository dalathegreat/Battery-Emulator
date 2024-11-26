#include "../include.h"
#ifdef BOLT_AMPERA_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BOLT-AMPERA-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20 = 0;  // will store last time a 20ms CAN Message was send

CAN_frame BOLT_778 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x778,
                      .data = {0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;
}

void receive_can_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x2C7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E3:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  //Send 20ms message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis20 >= INTERVAL_20_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis20));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis20 = currentMillis;
    transmit_can(&BOLT_778, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Chevrolet Bolt EV/Opel Ampera-e", 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

#endif
