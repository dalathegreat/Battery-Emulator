#include "parallel_safety.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "../../battery/BATTERIES.h"
#include "../../datalayer/battery_aggregate.h"
#include "../../datalayer/datalayer.h"
#include "../utils/events.h"

void check_parallel_battery_safety(uint8_t batteryNumber) {
  /* Before the checks are started, we need to know the battery is alive via CAN, and that the voltages have ben read*/
  if ((batteryNumber == 2) && battery2_detected) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery2.status.voltage_dV == 0) {
      return;  // 0 = not decoded yet, every pack starts there. Both are needed to start the check
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

  if ((batteryNumber == 3) && battery3_detected) {
    if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery3.status.voltage_dV == 0) {
      return;  // 0 = not decoded yet, every pack starts there. Both are needed to start the check
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

/* A battery on the DC link that stops carrying current while the others take over its share.

   Packs in parallel share the current by their voltages and internal resistances. When one of them
   leaves the link without BE opening its contactor - a tripped breaker, a blown fuse, a loose HV
   connection - its BMS keeps talking and has nothing to report: its current just falls to zero,
   and the packs left take its share on top of their own. Nothing else in BE notices.

   Once per second, among the packs on the link (pack 1, and pack 2/3 while allowed to close), a
   pack is suspected when all of these hold:
   - it reads about zero: within 1 A, or within 5% of what the others carry if that is more,
   - the others carry at least 2 A per pack, all one way,
   - within the last 15 s it carried at least 2 A that way, and since then it lost at least 40% of
     that beyond an even share of how the total changed. A cloud or a ramp moves every pack
     together and is taken out. A pack at another SOC drifts away from its share, it does not drop
     it in a step,
   - both readings were taken with the total held still for 4 s, so every pack had reported a
     fresh current: some integrations poll theirs every 2-3 s, others average it over 1.5 s.
   Three passes in a row raise the warning: 2-4 s after the trip with packs that broadcast their
   current, up to ~10 s with polled ones. A pack BE drops itself, for the voltage check or a fault,
   is left out from that moment - unless it was suspected already, which is what a trip looks like
   when the pack's voltage then drifts away. Not caught: a pack that never carried current since
   startup, and a trip in the same seconds as the total falling below 40% of what it was.

   A warning only. BE opens contactors and stops power on what a battery reports about itself; this
   is an inference from the currents. It clears once the pack carries current again. */
static constexpr uint8_t SHARE_WINDOW_PASSES = 15;  // How far back a pack's share is looked for
static constexpr uint8_t SHARE_SETTLED_PASSES = 4;  // How long the total has to hold still
static constexpr uint8_t SHARE_CONFIRM_PASSES = 3;  // In a row, to raise the warning and to clear it
static constexpr int32_t SHARE_MIN_DA = 20;         // 2 A per pack: less is too little to tell
static constexpr int32_t SHARE_ZERO_DA = 10;        // 1 A reads as no current...
static constexpr int32_t SHARE_ZERO_PCT = 5;        // ...or 5% of what the others carry, if more
static constexpr int32_t SHARE_SETTLED_PCT = 10;    // Held still: within 10% of the total, or 1 A
static constexpr int32_t SHARE_LOST_PCT = 40;       // Of what it carried, beyond an even split

struct CurrentSample {
  int16_t current_dA[3];
  bool settled;
};

struct CurrentSharing {
  CurrentSample history[SHARE_WINDOW_PASSES];  // Oldest first
  uint8_t samples;
  uint8_t suspect_passes[3];
  uint8_t carrying_passes[3];
  bool no_current[3];  // Found carrying nothing, warning up
  int16_t carried_A[3];
};

static CurrentSharing sharing;

static int32_t sum_of(const int16_t current_dA[3], const bool counted[3]) {
  int32_t sum = 0;
  for (uint8_t k = 0; k < 3; k++) {
    if (counted[k]) {
      sum += current_dA[k];
    }
  }
  return sum;
}

void check_parallel_current_sharing(void) {
  if (datalayer.system.info.configured_batteries < 2) {
    return;
  }

  const DATALAYER_BATTERY_STATUS_TYPE* status[3] = {&datalayer.battery.status, &datalayer.battery2.status,
                                                    &datalayer.battery3.status};
  const bool on_link[3] = {battery != nullptr,
                           battery2 != nullptr && datalayer.system.status.battery2_allowed_contactor_closing,
                           battery3 != nullptr && datalayer.system.status.battery3_allowed_contactor_closing};
  int16_t now_dA[3];
  bool counted[3];
  int32_t packs = 0;
  for (uint8_t k = 0; k < 3; k++) {
    now_dA[k] = pack_current_dA(*status[k]);
    counted[k] = (on_link[k] || sharing.suspect_passes[k] > 0) && !sharing.no_current[k];
    packs += counted[k] ? 1 : 0;
  }
  const int32_t total_dA = sum_of(now_dA, counted);

  // Held still: the same packs added up to about the same over the last passes
  const int32_t band_dA = std::max(SHARE_ZERO_DA, std::abs(total_dA) * SHARE_SETTLED_PCT / 100);
  bool settled = sharing.samples >= SHARE_SETTLED_PASSES;
  for (uint8_t age = 1; settled && age <= SHARE_SETTLED_PASSES; age++) {
    const CurrentSample& then = sharing.history[sharing.samples - age];
    settled = std::abs(sum_of(then.current_dA, counted) - total_dA) <= band_dA;
  }

  const int32_t way = (total_dA >= 0) ? 1 : -1;  // Charging or discharging
  for (uint8_t k = 0; k < 3; k++) {
    if (!counted[k]) {
      continue;
    }
    const int32_t others_dA = total_dA - now_dA[k];
    const int32_t zero_dA = std::max(SHARE_ZERO_DA, std::abs(others_dA) * SHARE_ZERO_PCT / 100);
    int32_t carried_dA = 0;
    if (packs >= 2 && settled && std::abs((int32_t)now_dA[k]) <= zero_dA &&
        way * others_dA >= SHARE_MIN_DA * (packs - 1)) {
      for (uint8_t i = 0; i < sharing.samples; i++) {
        const CurrentSample& then = sharing.history[i];
        const int32_t then_dA = way * then.current_dA[k];
        if (!then.settled || then_dA < SHARE_MIN_DA) {
          continue;
        }
        // What it lost beyond an even share of how the total changed, times the number of packs
        const int32_t lost_dA_x_packs =
            -way * (packs * (now_dA[k] - then.current_dA[k]) - (total_dA - sum_of(then.current_dA, counted)));
        if (lost_dA_x_packs * 100 >= then_dA * SHARE_LOST_PCT * packs) {
          carried_dA = then_dA;
          break;
        }
      }
    }
    if (carried_dA == 0) {
      sharing.suspect_passes[k] = 0;
    } else if (++sharing.suspect_passes[k] >= SHARE_CONFIRM_PASSES) {
      sharing.no_current[k] = true;
      sharing.carried_A[k] = (int16_t)((carried_dA + 5) / 10);
      sharing.suspect_passes[k] = 0;
      sharing.carrying_passes[k] = 0;
    }
  }

  // A pack found carrying nothing keeps its warning until it carries current again, either way
  for (uint8_t k = 0; k < 3; k++) {
    if (!sharing.no_current[k]) {
      continue;
    }
    if (std::abs((int32_t)now_dA[k]) >= SHARE_MIN_DA) {
      if (++sharing.carrying_passes[k] >= SHARE_CONFIRM_PASSES) {
        sharing.no_current[k] = false;
        sharing.carrying_passes[k] = 0;
        clear_event(EVENT_BATTERY_NO_CURRENT, (uint8_t)(k + 1));
        continue;
      }
    } else {
      sharing.carrying_passes[k] = 0;
    }
    set_event(EVENT_BATTERY_NO_CURRENT, sharing.carried_A[k], (uint8_t)(k + 1));
  }

  if (sharing.samples == SHARE_WINDOW_PASSES) {
    memmove(&sharing.history[0], &sharing.history[1], sizeof(CurrentSample) * (SHARE_WINDOW_PASSES - 1));
    sharing.samples--;
  }
  sharing.history[sharing.samples++] = {{now_dA[0], now_dA[1], now_dA[2]}, settled};
}

#ifdef UNIT_TEST
void reset_parallel_current_sharing(void) {
  sharing = CurrentSharing();
}
#endif
