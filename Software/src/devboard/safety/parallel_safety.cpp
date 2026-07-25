#include "parallel_safety.h"
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

void check_parallel_battery_safety(uint8_t batteryNumber) {

  if (millis() < INTERVAL_10_S) {
    // Skip running the state machine before system has started up.
    // Gives the system some time to detect sane voltages from the batteries before we start checking for voltage differences and potentially just report a fault and open contactors because the batteries haven't had time to report values yet.
    return;
  }

  if (batteryNumber == 2) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery2.status.voltage_dV == 0) {
      return;  // Both voltage values need to be available to start check
    }
    if (datalayer.battery.status.voltage_dV == 3700 || datalayer.battery2.status.voltage_dV == 3700) {
      return;  // Also abort if both voltages happened to be initialized to the 3700 default value that most integrations use
    }
    uint16_t voltage_diff_battery2_towards_main =
        abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV);
    static uint8_t secondsOutOfVoltageSyncBattery2 = 0;

    if (voltage_diff_battery2_towards_main <= 15) {  // If we are within 1.5V between the batteries
      clear_event(EVENT_VOLTAGE_DIFFERENCE_BAT2);
      secondsOutOfVoltageSyncBattery2 = 0;
      if (datalayer.system.status.system_status == FAULT) {
        // If main battery is in fault state, disengage the second battery
        datalayer.system.status.battery2_allowed_contactor_closing = false;
      } else {  // If main battery is OK, allow second battery to join
        datalayer.system.status.battery2_allowed_contactor_closing = true;
      }
    } else {  //Voltage between the two packs is too large
      //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
      //We alert user if we have been out of sync for more than 3 seconds, but we allow 10 seconds before we disengage the second battery
      if (secondsOutOfVoltageSyncBattery2 < 10) {
        secondsOutOfVoltageSyncBattery2++;
        if (secondsOutOfVoltageSyncBattery2 > 3) {
          set_event(EVENT_VOLTAGE_DIFFERENCE_BAT2, (uint8_t)(voltage_diff_battery2_towards_main / 10));
        }
      } else {  //10 seconds out of sync, disengage the second battery
        datalayer.system.status.battery2_allowed_contactor_closing = false;
      }
    }
  }

  if (batteryNumber == 3) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery3.status.voltage_dV == 0) {
      return;  // Both voltage values need to be available to start check
    }
    if (datalayer.battery.status.voltage_dV == 3700 || datalayer.battery3.status.voltage_dV == 3700) {
      return;  // Also abort if both voltages happened to be initialized to the 3700 default value that most integrations use
    }
    uint16_t voltage_diff_battery3_towards_main =
        abs(datalayer.battery.status.voltage_dV - datalayer.battery3.status.voltage_dV);
    static uint8_t secondsOutOfVoltageSyncBattery3 = 0;

    if (voltage_diff_battery3_towards_main <= 15) {  // If we are within 1.5V between the batteries
      clear_event(EVENT_VOLTAGE_DIFFERENCE_BAT3);
      secondsOutOfVoltageSyncBattery3 = 0;
      if (datalayer.system.status.system_status == FAULT) {
        // If main battery is in fault state, disengage the second battery
        datalayer.system.status.battery3_allowed_contactor_closing = false;
      } else {  // If main battery is OK, allow second battery to join
        datalayer.system.status.battery3_allowed_contactor_closing = true;
      }
    } else {  //Voltage between the two packs is too large
      //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
      //We alert user if we have been out of sync for more than 3 seconds, but we allow 10 seconds before we disengage the second battery
      if (secondsOutOfVoltageSyncBattery3 < 10) {
        secondsOutOfVoltageSyncBattery3++;
        if (secondsOutOfVoltageSyncBattery3 > 3) {
          set_event(EVENT_VOLTAGE_DIFFERENCE_BAT3, (uint8_t)(voltage_diff_battery3_towards_main / 10));
        }
      } else {  //10 seconds out of sync, disengage the second battery
        datalayer.system.status.battery3_allowed_contactor_closing = false;
      }
    }
  }
}
