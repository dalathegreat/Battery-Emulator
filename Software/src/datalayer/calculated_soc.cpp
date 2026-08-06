#include "calculated_soc.h"

#include "../battery/BATTERIES.h"
#include "../devboard/utils/value_mapping.h"
#include "datalayer.h"

// Moved verbatim out of update_calculated_values() in Software.cpp. Behaviour
// is unchanged by this commit; the defects it contains are addressed next.
void update_reported_soc_and_capacity() {
  if (datalayer.battery.settings.soc_scaling_active) {
    /** SOC Scaling
   * A static version of a stochastic oscillator. The scaled SoC is calculated as:
   *
   *     10000 * (real_soc - min_percentage)
   * ---------------------------------------
   *     (max_percentage - min_percentage)
   *
   * And scaled capacity is:
   *
   *     reported_total_capacity_Wh = total_capacity_Wh * (max - min) / 10000
   *     reported_remaining_capacity_Wh = reported_total_capacity_Wh * scaled_soc / 10000
   */
    // Compute delta_pct and clamped_soc
    int32_t delta_pct = datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage;
    int32_t clamped_soc = CONSTRAIN(datalayer.battery.status.real_soc, datalayer.battery.settings.min_percentage,
                                    datalayer.battery.settings.max_percentage);
    int32_t scaled_soc = 0;
    int32_t scaled_total_capacity = 0;
    if (delta_pct != 0) {  //Safeguard against division by 0
      scaled_soc = 10000 * (clamped_soc - datalayer.battery.settings.min_percentage) / delta_pct;
    }

    datalayer.battery.status.reported_soc = scaled_soc;

    // If battery info is valid
    if (datalayer.battery.info.total_capacity_Wh > 0 && datalayer.battery.status.real_soc > 0) {
      // Scale total usable capacity
      scaled_total_capacity = (datalayer.battery.info.total_capacity_Wh * delta_pct) / 10000;
      datalayer.battery.info.reported_total_capacity_Wh = scaled_total_capacity;

      // Scale remaining capacity based on scaled SOC
      datalayer.battery.status.reported_remaining_capacity_Wh = (scaled_total_capacity * scaled_soc) / 10000;

    } else {
      // Fallback if scaling cannot be performed
      datalayer.battery.info.reported_total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    }

    if (battery2) {
      // If battery info is valid
      if (datalayer.battery2.info.total_capacity_Wh > 0 && datalayer.battery.status.real_soc > 0) {

        datalayer.battery2.info.reported_total_capacity_Wh = scaled_total_capacity;
        // Scale remaining capacity based on scaled SOC
        datalayer.battery2.status.reported_remaining_capacity_Wh = (scaled_total_capacity * scaled_soc) / 10000;

      } else {
        // Fallback if scaling cannot be performed
        datalayer.battery2.info.reported_total_capacity_Wh = datalayer.battery2.info.total_capacity_Wh;
        datalayer.battery2.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
      }

      //Since we are running double battery, the scaled value of battery1 becomes the sum of battery1+battery2
      //This way the inverter connected to the system sees both batteries as one large battery
      datalayer.battery.info.reported_total_capacity_Wh += datalayer.battery2.info.reported_total_capacity_Wh;
      datalayer.battery.status.reported_remaining_capacity_Wh +=
          datalayer.battery2.status.reported_remaining_capacity_Wh;
    }

  } else {  // soc_scaling_active == false. No SOC window wanted. Set scaled SOC & capacity to same as real.
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;

    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    datalayer.battery.info.reported_total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;

    if (battery2) {
      datalayer.battery.status.reported_remaining_capacity_Wh =
          datalayer.battery.status.remaining_capacity_Wh + datalayer.battery2.status.remaining_capacity_Wh;
      datalayer.battery.info.reported_total_capacity_Wh =
          datalayer.battery.info.total_capacity_Wh + datalayer.battery2.info.total_capacity_Wh;
    }
    if (battery3) {
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh +
                                                                datalayer.battery2.status.remaining_capacity_Wh +
                                                                datalayer.battery3.status.remaining_capacity_Wh;
      datalayer.battery.info.reported_total_capacity_Wh = datalayer.battery.info.total_capacity_Wh +
                                                          datalayer.battery2.info.total_capacity_Wh +
                                                          datalayer.battery3.info.total_capacity_Wh;
    }
  }

  datalayer.battery2.status.reported_soc =
      datalayer.battery2.status.real_soc;  //For screen to display correct SOC of battery 2
  datalayer.battery3.status.reported_soc =
      datalayer.battery3.status.real_soc;  //For screen to display correct SOC of battery 3

  //Check each extra battery, and if they are at the extremes, report the SOC from these batteries instead
  if (battery2 && datalayer.system.status.battery2_allowed_contactor_closing) {  //Battery2 is in the mix
    if ((datalayer.battery2.status.real_soc < 100) || (datalayer.battery2.status.real_soc > 9900)) {
      datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
    }
  }
  if (battery3 && datalayer.system.status.battery3_allowed_contactor_closing) {  //Battery3 is in the mix
    if ((datalayer.battery3.status.real_soc < 100) || (datalayer.battery3.status.real_soc > 9900)) {
      datalayer.battery.status.reported_soc = datalayer.battery3.status.real_soc;
    }
  }
}
