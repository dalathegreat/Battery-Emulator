#include "pause.h"
#include "../../datalayer/datalayer.h"
#include "events.h"

bool emulator_pause_request_ON = false;
bool emulator_pause_CAN_send_ON = false;
bool can_send_CAN = true;

battery_pause_status emulator_pause_status = NORMAL;

void setBatteryPause(bool pause_battery, bool pause_CAN) {

  emulator_pause_CAN_send_ON = pause_CAN;

  if (pause_battery) {

    set_event(EVENT_PAUSE_BEGIN, 1);
    emulator_pause_request_ON = true;
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.max_charge_power_W = 0;
#ifdef DOUBLE_BATTERY
    datalayer.battery2.status.max_discharge_power_W = 0;
    datalayer.battery2.status.max_charge_power_W = 0;
#endif

    emulator_pause_status = PAUSING;
  } else {
    clear_event(EVENT_PAUSE_BEGIN);
    set_event(EVENT_PAUSE_END, 0);
    emulator_pause_request_ON = false;
    emulator_pause_CAN_send_ON = false;
    emulator_pause_status = RESUMING;
  }
}

/// @brief handle emulator pause status
/// @return true if CAN messages should be sent to battery, false if not
void emulator_pause_state_send_CAN_battery() {

  if (emulator_pause_status == NORMAL)
    can_send_CAN = true;

  // in some inverters this values are not accurate, so we need to check if we are consider 1.8 amps as the limit
  if (emulator_pause_request_ON && emulator_pause_status == PAUSING && datalayer.battery.status.current_dA < 18 &&
      datalayer.battery.status.current_dA > -18) {
    emulator_pause_status = PAUSED;
  }

  if (!emulator_pause_request_ON && emulator_pause_status == RESUMING) {
    emulator_pause_status = NORMAL;
    can_send_CAN = true;
  }

  can_send_CAN = (!emulator_pause_CAN_send_ON || emulator_pause_status == NORMAL);
}

std::string get_emulator_pause_status() {
  switch (emulator_pause_status) {
    case NORMAL:
      return "RUNNING";
    case PAUSING:
      return "PAUSING";
    case PAUSED:
      return "PAUSED";
    case RESUMING:
      return "RESUMING";
    default:
      return "UNKNOWN";
  }
}
