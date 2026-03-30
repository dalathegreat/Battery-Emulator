#include "interconnect.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void check_interconnect_available(uint8_t batteryNumber) {

  if (batteryNumber == 2) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery2.status.voltage_dV == 0) {
      return;  // Both voltage values need to be available to start check
    }
    uint16_t voltage_diff_battery2_towards_main =
        abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV);
    static uint8_t secondsOutOfVoltageSyncBattery2 = 0;

    if (voltage_diff_battery2_towards_main <= 15) {  // If we are within 1.5V between the batteries
      clear_event(EVENT_VOLTAGE_DIFFERENCE);
      secondsOutOfVoltageSyncBattery2 = 0;
      if (datalayer.battery.status.bms_status == FAULT) {
        // If main battery is in fault state, disengage the second battery
        datalayer.system.status.battery2_allowed_contactor_closing = false;
      } else {  // If main battery is OK, allow second battery to join
        datalayer.system.status.battery2_allowed_contactor_closing = true;
      }
    } else {  //Voltage between the two packs is too large
      set_event(EVENT_VOLTAGE_DIFFERENCE, (uint8_t)(voltage_diff_battery2_towards_main / 10));

      //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
      if (secondsOutOfVoltageSyncBattery2 < 10) {
        secondsOutOfVoltageSyncBattery2++;
      } else {
        datalayer.system.status.battery2_allowed_contactor_closing = false;
      }
    }
  }

  if (batteryNumber == 3) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery3.status.voltage_dV == 0) {
      return;  // Both voltage values need to be available to start check
    }
    uint16_t voltage_diff_battery3_towards_main =
        abs(datalayer.battery.status.voltage_dV - datalayer.battery3.status.voltage_dV);
    static uint8_t secondsOutOfVoltageSyncBattery3 = 0;

    if (voltage_diff_battery3_towards_main <= 15) {  // If we are within 1.5V between the batteries
      clear_event(EVENT_VOLTAGE_DIFFERENCE);
      secondsOutOfVoltageSyncBattery3 = 0;
      if (datalayer.battery.status.bms_status == FAULT) {
        // If main battery is in fault state, disengage the second battery
        datalayer.system.status.battery3_allowed_contactor_closing = false;
      } else {  // If main battery is OK, allow second battery to join
        datalayer.system.status.battery3_allowed_contactor_closing = true;
      }
    } else {  //Voltage between the two packs is too large
      set_event(EVENT_VOLTAGE_DIFFERENCE, (uint8_t)(voltage_diff_battery3_towards_main / 10));

      //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
      if (secondsOutOfVoltageSyncBattery3 < 10) {
        secondsOutOfVoltageSyncBattery3++;
      } else {
        datalayer.system.status.battery3_allowed_contactor_closing = false;
      }
    }
  }
}
