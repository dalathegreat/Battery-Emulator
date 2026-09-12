#include "parallel_safety.h"
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

ParallelJoinState join_state;

#ifdef UNIT_TEST
void reset_parallel_safety_state() {
  join_state = ParallelJoinState{};
}
#endif

void check_parallel_battery_safety(uint8_t batteryNumber) {
  /* Before the checks are started, we need to know the battery is alive via CAN, and that the voltages have ben read*/
  if ((batteryNumber == 2) && battery2_detected) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery2.status.voltage_dV == 0) {
      return;  // Both voltage values need to be available to start check
    }
    /* 3700 dV is the datalayer's init default, i.e. "no voltage decoded yet".
       Treating it as a continuous skip condition has two failure directions:
       a pack genuinely sitting at 370.0 V blocks battery 2 from ever
       joining, and a joined pair that drifts while one reads exactly 3700
       never reaches the 10 s disengage - both because this returns before the
       code that manages battery2_allowed_contactor_closing. Latch instead:
       once both packs have been seen off-sentinel, the check runs forever. */
    if (!join_state.voltages_seen_battery2) {
      if (datalayer.battery.status.voltage_dV == 3700 || datalayer.battery2.status.voltage_dV == 3700) {
        return;  // Startup grace: either pack may still hold the init default
      }
      join_state.voltages_seen_battery2 = true;
    }
    uint16_t voltage_diff_battery2_towards_main =
        abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV);

    if (voltage_diff_battery2_towards_main <= 15) {  // If we are within 1.5V between the batteries
      clear_event(EVENT_VOLTAGE_DIFFERENCE_BAT2);
      join_state.seconds_out_of_sync_battery2 = 0;
      if (datalayer.system.status.system_status == FAULT) {
        // If main battery is in fault state, disengage the second battery
        datalayer.system.status.battery2_allowed_contactor_closing = false;
      } else {  // If main battery is OK, allow second battery to join
        datalayer.system.status.battery2_allowed_contactor_closing = true;
      }
    } else {  //Voltage between the two packs is too large
      //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
      //We alert user if we have been out of sync for more than 3 seconds, but we allow 10 seconds before we disengage the second battery
      if (join_state.seconds_out_of_sync_battery2 < 10) {
        join_state.seconds_out_of_sync_battery2++;
        if (join_state.seconds_out_of_sync_battery2 > 3) {
          set_event(EVENT_VOLTAGE_DIFFERENCE_BAT2, (uint8_t)(voltage_diff_battery2_towards_main / 10));
        }
      } else {  //10 seconds out of sync, disengage the second battery
        datalayer.system.status.battery2_allowed_contactor_closing = false;
      }
    }
  }

  if ((batteryNumber == 3) && battery3_detected) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery3.status.voltage_dV == 0) {
      return;  // Both voltage values need to be available to start check
    }
    /* 3700 dV is the datalayer's init default, i.e. "no voltage decoded yet".
       Treating it as a continuous skip condition has two failure directions:
       a pack genuinely sitting at 370.0 V blocks battery 3 from ever
       joining, and a joined pair that drifts while one reads exactly 3700
       never reaches the 10 s disengage - both because this returns before the
       code that manages battery3_allowed_contactor_closing. Latch instead:
       once both packs have been seen off-sentinel, the check runs forever. */
    if (!join_state.voltages_seen_battery3) {
      if (datalayer.battery.status.voltage_dV == 3700 || datalayer.battery3.status.voltage_dV == 3700) {
        return;  // Startup grace: either pack may still hold the init default
      }
      join_state.voltages_seen_battery3 = true;
    }
    uint16_t voltage_diff_battery3_towards_main =
        abs(datalayer.battery.status.voltage_dV - datalayer.battery3.status.voltage_dV);

    if (voltage_diff_battery3_towards_main <= 15) {  // If we are within 1.5V between the batteries
      clear_event(EVENT_VOLTAGE_DIFFERENCE_BAT3);
      join_state.seconds_out_of_sync_battery3 = 0;
      if (datalayer.system.status.system_status == FAULT) {
        // If main battery is in fault state, disengage the second battery
        datalayer.system.status.battery3_allowed_contactor_closing = false;
      } else {  // If main battery is OK, allow second battery to join
        datalayer.system.status.battery3_allowed_contactor_closing = true;
      }
    } else {  //Voltage between the two packs is too large
      //If we start to drift out of sync between the two packs for more than 10 seconds, open contactors
      //We alert user if we have been out of sync for more than 3 seconds, but we allow 10 seconds before we disengage the second battery
      if (join_state.seconds_out_of_sync_battery3 < 10) {
        join_state.seconds_out_of_sync_battery3++;
        if (join_state.seconds_out_of_sync_battery3 > 3) {
          set_event(EVENT_VOLTAGE_DIFFERENCE_BAT3, (uint8_t)(voltage_diff_battery3_towards_main / 10));
        }
      } else {  //10 seconds out of sync, disengage the second battery
        datalayer.system.status.battery3_allowed_contactor_closing = false;
      }
    }
  }
}
