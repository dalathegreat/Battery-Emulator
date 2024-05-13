#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

static uint16_t cell_deviation_mV = 0;

void update_machineryprotection() {
  // Start checking that the battery is within reason. Incase we see any funny business, raise an event!

  // Battery is overheated!
  if (datalayer.battery.status.temperature_max_dC > 500) {
    set_event(EVENT_BATTERY_OVERHEAT, datalayer.battery.status.temperature_max_dC);
  } else {
    clear_event(EVENT_BATTERY_OVERHEAT);
  }

  // Battery is frozen!
  if (datalayer.battery.status.temperature_min_dC < -250) {
    set_event(EVENT_BATTERY_FROZEN, datalayer.battery.status.temperature_min_dC);
  } else {
    clear_event(EVENT_BATTERY_FROZEN);
  }

  // Battery is extremely degraded, not fit for secondlifestorage
  if (datalayer.battery.status.soh_pptt < 2500) {
    set_event(EVENT_LOW_SOH, datalayer.battery.status.soh_pptt);
  } else {
    clear_event(EVENT_LOW_SOH);
  }

  // Check diff between highest and lowest cell
  cell_deviation_mV = (datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV);
  if (cell_deviation_mV > MAX_CELL_DEVIATION_MV) {
    set_event(EVENT_CELL_DEVIATION_HIGH, (cell_deviation_mV / 20));
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

  // Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error
  if (!datalayer.battery.status.CAN_battery_still_alive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    datalayer.battery.status.CAN_battery_still_alive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

  // Also check if we have recieved too many malformed CAN messages
  if (datalayer.battery.status.CAN_error_counter > MAX_CAN_FAILURES) {
    set_event(EVENT_CAN_RX_WARNING, 0);
  } else {
    clear_event(EVENT_CAN_RX_WARNING);
  }
}
