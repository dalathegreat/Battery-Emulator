#include "calculated_soc.h"

#include "../battery/BATTERIES.h"
#include "../devboard/utils/value_mapping.h"
#include "datalayer.h"

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

namespace {

// The user's SOC window, shared by every pack: the inverter sees one battery,
// so one window applies to all of them.
int32_t window_delta_pct() {
  return datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage;
}

// Maps a real SOC onto the user's window. Used for every pack and for the
// extreme-pack override below, so a scaled figure is never mixed with a raw one.
int32_t scale_soc(int32_t real_soc) {
  const int32_t delta_pct = window_delta_pct();
  if (delta_pct == 0) {  // Safeguard against division by 0
    return 0;
  }
  const int32_t clamped_soc =
      CONSTRAIN(real_soc, datalayer.battery.settings.min_percentage, datalayer.battery.settings.max_percentage);
  return 10000 * (clamped_soc - datalayer.battery.settings.min_percentage) / delta_pct;
}

// Writes one pack's own scaled SOC and capacity into its own reported fields.
// Every value comes from that pack: using battery 1's capacity for the extra
// packs is what made their reported figures wrong whenever the packs differed.
void scale_pack(DATALAYER_BATTERY_TYPE& pack) {
  const int32_t scaled_soc = scale_soc(pack.status.real_soc);
  pack.status.reported_soc = scaled_soc;

  if (pack.info.total_capacity_Wh > 0 && pack.status.real_soc > 0) {
    const int32_t scaled_total_capacity = (pack.info.total_capacity_Wh * window_delta_pct()) / 10000;
    pack.info.reported_total_capacity_Wh = scaled_total_capacity;
    pack.status.reported_remaining_capacity_Wh = (scaled_total_capacity * scaled_soc) / 10000;
  } else {
    // Fallback if scaling cannot be performed
    pack.info.reported_total_capacity_Wh = pack.info.total_capacity_Wh;
    pack.status.reported_remaining_capacity_Wh = pack.status.remaining_capacity_Wh;
  }
}

// Copies a pack's real values into its reported fields, for when no window is
// wanted.
void report_pack_unscaled(DATALAYER_BATTERY_TYPE& pack) {
  pack.status.reported_soc = pack.status.real_soc;
  pack.info.reported_total_capacity_Wh = pack.info.total_capacity_Wh;
  pack.status.reported_remaining_capacity_Wh = pack.status.remaining_capacity_Wh;
}

// True when an extra pack has reached either end of its range and the whole
// system should report that, so the inverter stops before the weakest pack is
// driven past its limit.
bool at_extreme(const DATALAYER_BATTERY_TYPE& pack) {
  return (pack.status.real_soc < 100) || (pack.status.real_soc > 9900);
}

}  // namespace

void update_reported_soc_and_capacity() {
  const bool scaling = datalayer.battery.settings.soc_scaling_active;

  // Each configured pack first gets its own reported values, in its own right.
  if (scaling) {
    scale_pack(datalayer.battery);
  } else {
    report_pack_unscaled(datalayer.battery);
  }
  if (battery2) {
    scaling ? scale_pack(datalayer.battery2) : report_pack_unscaled(datalayer.battery2);
  }
  if (battery3) {
    scaling ? scale_pack(datalayer.battery3) : report_pack_unscaled(datalayer.battery3);
  }

  // The inverter sees the whole system as one battery, so battery 1's reported
  // capacity doubles as the system total. Summed over every configured pack -
  // battery 3 used to be left out of this whenever scaling was active, so a
  // triple-pack system under-reported its capacity by a third.
  uint32_t total_capacity_Wh = datalayer.battery.info.reported_total_capacity_Wh;
  uint32_t remaining_capacity_Wh = datalayer.battery.status.reported_remaining_capacity_Wh;
  if (battery2) {
    total_capacity_Wh += datalayer.battery2.info.reported_total_capacity_Wh;
    remaining_capacity_Wh += datalayer.battery2.status.reported_remaining_capacity_Wh;
  }
  if (battery3) {
    total_capacity_Wh += datalayer.battery3.info.reported_total_capacity_Wh;
    remaining_capacity_Wh += datalayer.battery3.status.reported_remaining_capacity_Wh;
  }
  datalayer.battery.info.reported_total_capacity_Wh = total_capacity_Wh;
  datalayer.battery.status.reported_remaining_capacity_Wh = remaining_capacity_Wh;

  // If an extra pack is at either extreme, report its SOC for the whole system.
  // Scaled through the same window when scaling is active: writing the raw
  // value here put an unscaled number into the field the inverter reads, so the
  // reported SOC jumped domains the moment an extra pack crossed 1% or 99%.
  if (battery2 && datalayer.system.status.battery2_allowed_contactor_closing && at_extreme(datalayer.battery2)) {
    datalayer.battery.status.reported_soc = datalayer.battery2.status.reported_soc;
  }
  if (battery3 && datalayer.system.status.battery3_allowed_contactor_closing && at_extreme(datalayer.battery3)) {
    datalayer.battery.status.reported_soc = datalayer.battery3.status.reported_soc;
  }
}
