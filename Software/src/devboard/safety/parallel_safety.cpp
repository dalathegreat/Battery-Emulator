#include "parallel_safety.h"
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

// True when the pack currently holds the DC link (contactors closed). Prefers
// the state the pack's own BMS reports; Unknown falls back to the BE-commanded
// state.
static bool pack_holds_link(Battery* pack, bool commanded_engaged) {
  if (pack) {
    ContactorState reported = pack->reported_contactor_state();
    if (reported == ContactorState::Closed) {
      return true;
    }
    if (reported == ContactorState::Open) {
      return false;
    }
  }
  return commanded_engaged;
}

// Symmetric half of the 1.5 V parallel-join rule: while another pack holds the
// link and the voltages differ too much, the MAIN battery must not (re-)close
// onto it either. Close-gating only - opening the main under load is its own
// hazard and stays out of scope here.
static bool main_blocked_by_battery2 = false;
static bool main_blocked_by_battery3 = false;

static void update_main_join_permission() {
  datalayer.system.status.battery1_allowed_contactor_closing = !(main_blocked_by_battery2 || main_blocked_by_battery3);
}

// The 1.5 V parallel-join rule for one joining pack (battery 2 or 3) against
// the main battery, in both directions: gates the joiner's own permission
// (alert after 3 s out of sync, disengage after 10 s), and computes whether
// the joiner blocks the MAIN battery from (re-)closing onto the link.
static void check_parallel_join(const DATALAYER_BATTERY_TYPE& joiner_datalayer, Battery* joiner_pack,
                                bool commanded_engaged, EVENTS_ENUM_TYPE voltage_diff_event,
                                uint8_t& seconds_out_of_sync, bool& joiner_allowed_contactor_closing,
                                bool& main_blocked_by_joiner) {
  if (datalayer.battery.status.voltage_dV == 0 || joiner_datalayer.status.voltage_dV == 0) {
    return;  // Both voltage values need to be available to start check
  }
  if (datalayer.battery.status.voltage_dV == 3700 || joiner_datalayer.status.voltage_dV == 3700) {
    return;  // Also abort if both voltages happened to be initialized to the 3700 default value that most integrations use
  }
  uint16_t voltage_diff_towards_main = abs(datalayer.battery.status.voltage_dV - joiner_datalayer.status.voltage_dV);

  main_blocked_by_joiner = pack_holds_link(joiner_pack, commanded_engaged) && (voltage_diff_towards_main > 15);
  update_main_join_permission();

  if (voltage_diff_towards_main <= 15) {  // If we are within 1.5V between the batteries
    clear_event(voltage_diff_event);
    seconds_out_of_sync = 0;
    if (datalayer.system.status.system_status == FAULT) {
      // If main battery is in fault state, disengage the joining battery
      joiner_allowed_contactor_closing = false;
    } else {  // If main battery is OK, allow the joining battery to close
      joiner_allowed_contactor_closing = true;
    }
  } else {  //Voltage between the two packs is too large
    //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
    //We alert user if we have been out of sync for more than 3 seconds, but we allow 10 seconds before we disengage the joining battery
    if (seconds_out_of_sync < 10) {
      seconds_out_of_sync++;
      if (seconds_out_of_sync > 3) {
        set_event(voltage_diff_event, (uint8_t)(voltage_diff_towards_main / 10));
      }
    } else {  //10 seconds out of sync, disengage the joining battery
      joiner_allowed_contactor_closing = false;
    }
  }
}

void check_parallel_battery_safety(uint8_t batteryNumber) {
  /* Before the checks are started, we need to know the battery is alive via CAN, and that the voltages have ben read*/
  if (batteryNumber == 2) {
    if (battery2_detected) {
      static uint8_t secondsOutOfVoltageSyncBattery2 = 0;
      check_parallel_join(datalayer.battery2, battery2, datalayer.system.status.contactors_battery2_engaged,
                          EVENT_VOLTAGE_DIFFERENCE_BAT2, secondsOutOfVoltageSyncBattery2,
                          datalayer.system.status.battery2_allowed_contactor_closing, main_blocked_by_battery2);
    } else {
      main_blocked_by_battery2 = false;
      update_main_join_permission();
    }
  }

  if (batteryNumber == 3) {
    if (battery3_detected) {
      static uint8_t secondsOutOfVoltageSyncBattery3 = 0;
      check_parallel_join(datalayer.battery3, battery3, datalayer.system.status.contactors_battery3_engaged,
                          EVENT_VOLTAGE_DIFFERENCE_BAT3, secondsOutOfVoltageSyncBattery3,
                          datalayer.system.status.battery3_allowed_contactor_closing, main_blocked_by_battery3);
    } else {
      main_blocked_by_battery3 = false;
      update_main_join_permission();
    }
  }
}
