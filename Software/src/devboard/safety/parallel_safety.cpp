#include "parallel_safety.h"
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

static void check_voltage_sync(const DATALAYER_BATTERY_TYPE& pack, bool detected, bool& matches,
                               uint8_t& seconds_out_of_sync, EVENTS_ENUM_TYPE mismatch_event) {
  matches = false;  // Until this reading says otherwise
  /* Before the checks are started, we need to know the battery is alive via CAN, and that the voltages have ben read*/
  if (!detected) {
    return;
  }
  if (datalayer.battery.status.voltage_dV == 0 || pack.status.voltage_dV == 0) {
    return;  // 0 = not decoded yet, every pack starts there. Both are needed to start the check
  }
  uint16_t voltage_diff_towards_main = abs(datalayer.battery.status.voltage_dV - pack.status.voltage_dV);
  matches = voltage_diff_towards_main <= 15;  // Within 1.5V between the batteries

  if (matches) {
    clear_event(mismatch_event);
    seconds_out_of_sync = 0;
  } else if (seconds_out_of_sync < 255) {
    // Alert the user once we have been out of sync for more than 3 seconds
    seconds_out_of_sync++;
    if (seconds_out_of_sync > 3) {
      set_event(mismatch_event, (uint8_t)(voltage_diff_towards_main / 10));
    }
  }
}

void check_parallel_battery_safety(uint8_t batteryNumber) {
  static uint8_t secondsOutOfVoltageSyncBattery2 = 0;
  static uint8_t secondsOutOfVoltageSyncBattery3 = 0;

  if (batteryNumber == 2) {
    check_voltage_sync(datalayer.battery2, battery2_detected, datalayer.system.status.battery2_voltage_matches,
                       secondsOutOfVoltageSyncBattery2, EVENT_VOLTAGE_DIFFERENCE_BAT2);
  } else if (batteryNumber == 3) {
    check_voltage_sync(datalayer.battery3, battery3_detected, datalayer.system.status.battery3_voltage_matches,
                       secondsOutOfVoltageSyncBattery3, EVENT_VOLTAGE_DIFFERENCE_BAT3);
  }
}
