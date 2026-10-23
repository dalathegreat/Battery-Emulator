#include "battery_aggregate.h"

#include "../battery/BATTERIES.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/value_mapping.h"
#include "datalayer.h"

/* SOC at which the aggregate starts blending from the emptiest pack towards the fullest one,
   in integer-percent x 100. Below this the emptiest pack has it alone. */
static constexpr uint16_t SOC_BLEND_START_PPTT = 9000;

/* What the inverter is told until pack 1 has decoded a voltage. The packs themselves read 0
   until then. Some inverters (Solax in particular) fault with "voltage out of range" when this
   lies outside what the pack can really reach, and an LFP pack can top out below 370.0 V, so
   LFP gets 330.0 V - the value the CMP Smart Car integration used to start its LFP pack on. */
static constexpr uint16_t PLACEHOLDER_LINK_VOLTAGE_DV = 3700;
static constexpr uint16_t PLACEHOLDER_LINK_VOLTAGE_LFP_DV = 3300;

/* The parallel charge limit is only raised while the installation draws at least this much of it,
   in percent. Below that the inverter is not using what it has, so the split between the packs
   says nothing about the limit - see parallel_charge_limit_W(). */
static constexpr uint32_t PARALLEL_RAISE_MIN_DRAW_PCT = 95;

/* The safety layer, the SOC taper and the low pass filter all rewrite a pack's power limits in
   place, so by the time the web page renders, what the BMS actually asked for is gone. Keep a
   copy while it is still the driver's own number. */
void snapshot_bms_limits(DATALAYER_BATTERY_TYPE& pack) {
  pack.status.bms_max_charge_power_W = pack.status.max_charge_power_W;
  pack.status.bms_max_discharge_power_W = pack.status.max_discharge_power_W;
}

#ifndef SMALL_FLASH_DEVICE
/* These two live here rather than inline in the header so that no caller can inline them. GCC
   (13 and 14, at -Os) miscompiles them once inlined into a function that hands in the status of a
   pack it took by reference, as the main page's fill_card_view(pack) does: it carries the address
   check below over to the pack itself, forgetting where in the pack the status sits, concludes
   that pack 1 can never match, and drops that path as unreachable. Pack 1 then ran into code that
   was not there, and the main page ended right after the combined card. */
int16_t pack_current_dA(const DATALAYER_BATTERY_STATUS_TYPE& status) {
  if (&status == &datalayer.battery.status && shunt_measures_battery1()) {
    return shunt_current_dA();
  }
  return status.current_dA;
}

int32_t pack_power_W(const DATALAYER_BATTERY_STATUS_TYPE& status) {
  if (&status == &datalayer.battery.status && shunt_measures_battery1()) {
    return current_dA_to_power_W(shunt_current_dA(), status.voltage_dV);
  }
  return status.active_power_W;
}
#endif  // SMALL_FLASH_DEVICE

/* The SOC window belongs to the installation, not to any one pack: it decides what the inverter
   is told, and a scaled number on a single pack of several describes nothing that exists. So
   once there is more than one battery the packs report exactly what they would with scaling
   switched off - which is what the per-pack cards, MQTT topics and ESP-NOW frames then carry -
   and the window is applied once, to the aggregate.

   With a single battery datalayer.battery IS the installation, so it keeps its scaled reported_
   fields and nothing downstream of it sees any change. */
void scale_pack_values(DATALAYER_BATTERY_TYPE& pack) {
  const bool window_applies =
      datalayer.battery_settings.soc_scaling_active && (datalayer.system.info.configured_batteries < 2);

  if (!window_applies) {
    pack.status.reported_soc = pack.status.real_soc;
    pack.info.reported_total_capacity_Wh = pack.info.total_capacity_Wh;
    pack.status.reported_remaining_capacity_Wh = pack.status.remaining_capacity_Wh;
    return;
  }

  int32_t delta_pct = datalayer.battery_settings.max_percentage - datalayer.battery_settings.min_percentage;
  int32_t clamped_soc = CONSTRAIN(pack.status.real_soc, datalayer.battery_settings.min_percentage,
                                  datalayer.battery_settings.max_percentage);
  int32_t scaled_soc = 0;
  if (delta_pct != 0) {  //Safeguard against division by 0
    scaled_soc = 10000 * (clamped_soc - datalayer.battery_settings.min_percentage) / delta_pct;
  }
  pack.status.reported_soc = scaled_soc;

  if (pack.info.total_capacity_Wh > 0 && pack.status.real_soc > 0) {
    int32_t scaled_total_capacity = (pack.info.total_capacity_Wh * delta_pct) / 10000;
    pack.info.reported_total_capacity_Wh = scaled_total_capacity;
    pack.status.reported_remaining_capacity_Wh = (scaled_total_capacity * scaled_soc) / 10000;
  } else {
    // Fallback if scaling cannot be performed
    pack.info.reported_total_capacity_Wh = pack.info.total_capacity_Wh;
    pack.status.reported_remaining_capacity_Wh = pack.status.remaining_capacity_Wh;
  }
}

/* Apply the SOC window to the installation. Same arithmetic scale_pack_values() uses, run once
   on the summed figures, so the reported SOC and the reported energy agree with each other. */
static void apply_soc_window(DATALAYER_AGGREGATE_TYPE& agg) {
  if (!datalayer.battery_settings.soc_scaling_active) {
    agg.reported_soc = agg.real_soc;
    agg.reported_total_capacity_Wh = agg.total_capacity_Wh;
    agg.reported_remaining_capacity_Wh = agg.remaining_capacity_Wh;
    return;
  }

  int32_t delta_pct = datalayer.battery_settings.max_percentage - datalayer.battery_settings.min_percentage;
  int32_t clamped_soc =
      CONSTRAIN(agg.real_soc, datalayer.battery_settings.min_percentage, datalayer.battery_settings.max_percentage);
  int32_t scaled_soc = 0;
  if (delta_pct != 0) {  //Safeguard against division by 0
    scaled_soc = 10000 * (clamped_soc - datalayer.battery_settings.min_percentage) / delta_pct;
  }
  agg.reported_soc = scaled_soc;

  if (agg.total_capacity_Wh > 0) {
    agg.reported_total_capacity_Wh = ((uint64_t)agg.total_capacity_Wh * delta_pct) / 10000;
    agg.reported_remaining_capacity_Wh = ((uint64_t)agg.reported_total_capacity_Wh * scaled_soc) / 10000;
  } else {
    agg.reported_total_capacity_Wh = agg.total_capacity_Wh;
    agg.reported_remaining_capacity_Wh = agg.remaining_capacity_Wh;
  }
}

/* Roll every configured pack up into datalayer.aggregate.
 *
 * With one battery this is a plain copy of datalayer.battery, so a single-pack system ends up
 * with exactly the numbers it had before this struct existed.
 *
 * Three different gates decide whether a pack counts, and they are not interchangeable:
 *
 *   configured - the pack object exists. Energy counts on this one, whether or not the pack has
 *                joined the DC link yet, so the inverter's picture of the installation does not
 *                jump when the contactors finally close.
 *   detected   - the pack has spoken on the bus. Anything measured needs this: a configured but
 *                silent pack still holds its power-on defaults (3700 mV cells, 0 dC, 0% SOC,
 *                99.00% SOH), which are not measurements and would drag the aggregate somewhere
 *                the installation never went.
 *   joined     - the pack is actually on the DC link. SOC needs this on top of detected. A pack
 *                held out by the voltage check, or dropped after a fault, is still talking and
 *                still reporting a real SOC - but it is not the SOC of anything the inverter can
 *                charge or discharge. Letting it through means an empty detached pack reads the
 *                installation empty and a full one reads it full, either of which stops the
 *                system on behalf of a battery that is not connected to it.
 *
 * Cell voltages and temperatures stop at detected on purpose. Those can only ever make the
 * inverter more cautious - a hot or high-cell pack lowers what it asks for - so seeing a pack
 * that is about to join a moment early costs nothing, while SOC can halt the system outright in
 * either direction.
 *
 * The limit fields are deliberately not touched here - see combine_pack_limits() and
 * update_aggregate_limits().
 */
void update_aggregate_values() {
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;

  agg.current_dA = datalayer.battery.status.reported_current_dA;  // Already the sum of every pack
#ifndef SMALL_FLASH_DEVICE
  /* A current sensor fitted for this measures what actually flows between the batteries and the
     inverter, so while it has a reading it replaces what the packs report - their sum, with
     several. The packs keep their own figures. Power below follows from it. */
  if (shunt_replaces_battery_current()) {
    agg.current_dA = shunt_current_dA();
  }
#endif  // SMALL_FLASH_DEVICE
  agg.cell_max_voltage_mV = datalayer.battery.status.cell_max_voltage_mV;
  agg.cell_min_voltage_mV = datalayer.battery.status.cell_min_voltage_mV;
  agg.temperature_max_dC = datalayer.battery.status.temperature_max_dC;
  agg.temperature_min_dC = datalayer.battery.status.temperature_min_dC;
  /* Health only from packs that have actually decoded one. Pack 1's soh_pptt is the fallback
     when none has: it is a safe default and it still has to feed the inverter, but it is not a
     reading, so soh_available says so. */
  bool soh_found = datalayer.battery.status.soh_available && datalayer.battery.status.soh_pptt > 0;
  uint16_t lowest_soh = soh_found ? datalayer.battery.status.soh_pptt : 0;
  agg.max_design_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  agg.min_design_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  agg.total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
  agg.remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
  agg.total_charged_battery_Wh = datalayer.battery.status.total_charged_battery_Wh;
  agg.total_discharged_battery_Wh = datalayer.battery.status.total_discharged_battery_Wh;

  uint16_t lowest_soc = datalayer.battery.status.real_soc;
  uint16_t highest_soc = datalayer.battery.status.real_soc;

  if (datalayer.system.info.configured_batteries > 1) {
    const DATALAYER_BATTERY_TYPE* extra_pack[2] = {battery2 ? &datalayer.battery2 : nullptr,
                                                   battery3 ? &datalayer.battery3 : nullptr};
    const bool pack_detected[2] = {battery2_detected, battery3_detected};
    const bool pack_joined[2] = {datalayer.system.status.battery2_allowed_contactor_closing,
                                 datalayer.system.status.battery3_allowed_contactor_closing};

    for (uint8_t i = 0; i < 2; i++) {
      const DATALAYER_BATTERY_TYPE* pack = extra_pack[i];
      if (!pack) {
        continue;
      }

      agg.total_capacity_Wh += pack->info.total_capacity_Wh;
      agg.remaining_capacity_Wh += pack->status.remaining_capacity_Wh;
      agg.total_charged_battery_Wh += pack->status.total_charged_battery_Wh;
      agg.total_discharged_battery_Wh += pack->status.total_discharged_battery_Wh;

      if (!pack_detected[i]) {
        continue;  // Never seen on the bus, its defaults are not measurements
      }

      agg.cell_max_voltage_mV = MAX(agg.cell_max_voltage_mV, pack->status.cell_max_voltage_mV);
      agg.cell_min_voltage_mV = MIN(agg.cell_min_voltage_mV, pack->status.cell_min_voltage_mV);
      agg.temperature_max_dC = MAX(agg.temperature_max_dC, pack->status.temperature_max_dC);
      agg.temperature_min_dC = MIN(agg.temperature_min_dC, pack->status.temperature_min_dC);
      /* Only a pack that is actually on the link gets to move the installation's SOC */
      if (pack_joined[i]) {
        lowest_soc = MIN(lowest_soc, pack->status.real_soc);
        highest_soc = MAX(highest_soc, pack->status.real_soc);
      }
      /* Health follows the weakest pack, like every other limit here. A pack that has not
         decoded one yet is skipped, rather than dragging the installation to its default or to
         zero. */
      if (pack->status.soh_available && pack->status.soh_pptt > 0) {
        lowest_soh = soh_found ? MIN(lowest_soh, pack->status.soh_pptt) : pack->status.soh_pptt;
        soh_found = true;
      }

      /* The installation may only be charged as high as the lowest ceiling any pack reports,
         and only discharged as low as the highest floor, or a mismatched pack gets pushed past
         what it will tolerate. Zeroes are skipped: an integration that has not decoded these
         yet must not set the limit for everyone. */
      if (pack->info.max_design_voltage_dV > 0) {
        agg.max_design_voltage_dV = MIN(agg.max_design_voltage_dV, pack->info.max_design_voltage_dV);
      }
      if (pack->info.min_design_voltage_dV > 0) {
        agg.min_design_voltage_dV = MAX(agg.min_design_voltage_dV, pack->info.min_design_voltage_dV);
      }
    }
  }

  /* Every pack reads 0 until its integration has decoded a voltage, which is what
     check_parallel_battery_safety() waits for. The inverter never sees that 0: until pack 1
     has a reading it gets 370.0 V, or 330.0 V for an LFP pack, or the middle of the design
     window when that value does not fit the installation (LV). The chemistry decides rather
     than the window, because some integrations start on a deliberately wide window until they
     have counted their cells. Done after the loop, so the window is the installation's. */
  agg.voltage_dV = datalayer.battery.status.voltage_dV;
  if (agg.voltage_dV == 0) {
    const uint16_t placeholder_dV = (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP)
                                        ? PLACEHOLDER_LINK_VOLTAGE_LFP_DV
                                        : PLACEHOLDER_LINK_VOLTAGE_DV;
    const bool fits = agg.min_design_voltage_dV <= placeholder_dV && placeholder_dV <= agg.max_design_voltage_dV;
    agg.voltage_dV = fits ? placeholder_dV : (uint16_t)((agg.min_design_voltage_dV + agg.max_design_voltage_dV) / 2);
  }

  /* Power from the summed current against the shared bus voltage, dividing once at the end so
     the fractional Volt survives: 386.0 V spent as 380 V costs about 1.5%. Written out rather
     than calling current_dA_to_power_W() from datalayer.h, so this branch does not touch a line
     #2978 also touches - switch to the helper once that has merged. */
  agg.active_power_W = ((int32_t)agg.voltage_dV * (int32_t)agg.current_dA) / 100;

  /* SOC follows the emptiest pack, which is what protects the weakest one on discharge. Once
     the fullest pack climbs into the top tenth, blend towards it so the installation arrives at
     100% smoothly instead of stepping there the moment one pack tops out - and so that charging
     does stop, which reporting the emptiest pack alone would never do. At 100% on the fullest
     pack the blend has handed over completely. A single pack has lowest == highest, so this is
     its own SOC either way. */
  if (highest_soc >= SOC_BLEND_START_PPTT) {
    uint32_t blend = (uint32_t)(highest_soc - SOC_BLEND_START_PPTT);
    uint32_t spread = (highest_soc > lowest_soc) ? (uint32_t)(highest_soc - lowest_soc) : 0u;
    agg.real_soc = (uint16_t)(lowest_soc + (spread * blend) / (10000u - SOC_BLEND_START_PPTT));
  } else {
    agg.real_soc = lowest_soc;
  }

  agg.soh_available = soh_found;
  agg.soh_pptt = soh_found ? lowest_soh : datalayer.battery.status.soh_pptt;

  apply_soc_window(agg);
}

/* The charge power the joined packs take together.
 *
 * The packs sit in parallel, so whatever the inverter pushes splits between them - by their
 * internal resistance, not necessarily evenly, but never all into one. Holding the installation
 * to the lowest pack limit therefore never lets that pack reach it: two alike packs each see half
 * of it.
 *
 * So the lowest limit is only where it starts. While the installation draws what it has been
 * allowed, the limit is raised by the headroom the tightest pack has left - its own limit minus
 * the power it takes - and again later, so it climbs in steps until that pack's power meets its
 * limit. A raise of that size cannot push any pack past its own limit, since each takes only its
 * share of the extra. There is no raise while the installation draws less than it is allowed -
 * not enough sun, the inverter's own limit, or the SOC taper, the low pass filter or a current
 * cap holding it below - because the split then says nothing about the limit. Nor while a joined
 * pack shows no charge power, which is also what a pack that does not report its current looks
 * like.
 *
 * The headroom is taken from the lowest limit the pack has reported lately rather than from the
 * one it reports right now. A BMS that flips between two limits every few seconds - a LEAF can
 * for half an hour before it settles - would otherwise have the limit raised on every high and
 * be pushed over on every low. A new low counts at once, a higher limit only once nothing lower
 * has come for PARALLEL_LIMIT_HOLD_PASSES.
 *
 * A pack whose limit drops below the one the raise was measured against, or that goes over its
 * limit anyway because the split moved, cuts the limit at once to the total at which it would sit
 * on its limit at the split measured now - whether or not the inverter is drawing that much yet,
 * so a lower limit that arrives under a cloud is not left for the sun to find. A pack that shows
 * no charge power has no split to go by, so a drop on it takes the limit back to the lowest pack
 * limit. Being worked out from what is flowing, the cut comes out the same again while the
 * inverter has not followed yet, instead of cutting deeper every pass.
 *
 * After every raise or cut, and at the start of every charge session, the inverter is given
 * PARALLEL_SETTLE_PASSES to follow before the next raise. Fronius over Modbus only reads the
 * limits every 5 s, and every inverter needs time to ramp: measured any sooner, a raise would
 * stack on top of the last one before it had been seen. Cuts are never held back.
 *
 * Never below the lowest pack limit, which is what was always sent, and never above the sum of
 * the joined packs' limits. The lowest pack limit is applied to what is sent but not kept, so
 * when it comes back down what is sent follows it, as it did before the raise existed. Back to
 * the lowest whenever the installation stops charging or a joined pack allows no charge at all.
 */
static uint32_t parallel_charge_limit_W(const DATALAYER_BATTERY_TYPE* const joined[], const uint8_t pack_index[],
                                        uint8_t count, uint32_t lowest_W) {
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;
  uint32_t& raised_W = agg.parallel_charge_limit_W;  // What has been measured to be safe, pass to pass
  uint8_t& settle = agg.parallel_settle_passes;

  int32_t power_W[3];
  bool needs_cut[3] = {false, false, false};  // This pack has to be put back on its limit
  uint64_t sum_W = 0;                         // Every joined pack at its own limit
  int64_t drawn_W = 0;                        // What they take between them, positive while charging
  int64_t headroom_W = INT64_MAX;             // The tightest pack's held limit minus the power it takes
  bool over_limit = false;                    // Some pack takes more than it allows right now
  bool any_cut = false;
  bool all_charging = true;
  for (uint8_t i = 0; i < count; i++) {
    const uint32_t pack_limit_W = joined[i]->status.max_charge_power_W;
    uint32_t& held_W = agg.parallel_held_limit_W[pack_index[i]];
    uint8_t& hold = agg.parallel_hold_passes[pack_index[i]];
    const bool dropped = pack_limit_W < held_W;  // Below the limit the raise was measured against
    if (pack_limit_W <= held_W) {
      held_W = pack_limit_W;  // A new low counts at once, and coming back to it keeps it...
      hold = PARALLEL_LIMIT_HOLD_PASSES;
    } else if (hold > 0) {
      hold--;  // ...a higher limit once it has stayed up
    } else {
      held_W = pack_limit_W;
    }

    power_W[i] = pack_power_W(joined[i]->status);
    sum_W += pack_limit_W;
    drawn_W += power_W[i];
    headroom_W = MIN(headroom_W, (int64_t)held_W - power_W[i]);
    const bool over = power_W[i] > (int64_t)pack_limit_W;
    over_limit = over_limit || over;
    needs_cut[i] = dropped || over;
    any_cut = any_cut || needs_cut[i];
    all_charging = all_charging && (power_W[i] > 0);
  }

  if (lowest_W == 0 || drawn_W <= 0) {
    raised_W = 0;  // Start over
    settle = PARALLEL_SETTLE_PASSES;
    return lowest_W;
  }
  if (settle > 0) {
    settle--;
  }

  const uint64_t ceiling_W = MIN(sum_W, (uint64_t)UINT32_MAX);
  uint64_t sent_W = CONSTRAIN((uint64_t)raised_W, (uint64_t)lowest_W, ceiling_W);
  uint64_t kept_W = MIN((uint64_t)raised_W, ceiling_W);  // The floor is sent, not kept
  if (any_cut) {  // A pack's limit dropped, or a pack is over it: against the limit it reports now
    uint64_t cut_W = sent_W;
    for (uint8_t i = 0; i < count; i++) {
      if (!needs_cut[i]) {
        continue;
      }
      const uint32_t pack_limit_W = joined[i]->status.max_charge_power_W;
      cut_W = (power_W[i] > 0) ? MIN(cut_W, (uint64_t)drawn_W * pack_limit_W / (uint64_t)power_W[i])
                               : MIN(cut_W, (uint64_t)lowest_W);  // No split to go by
    }
    if (over_limit || cut_W < sent_W) {
      sent_W = cut_W;
      kept_W = MIN(kept_W, cut_W);
      settle = PARALLEL_SETTLE_PASSES;
    }
  } else if (settle == 0 && all_charging && headroom_W > 0 &&
             (uint64_t)drawn_W * 100 >= sent_W * PARALLEL_RAISE_MIN_DRAW_PCT) {
    sent_W += (uint64_t)headroom_W;
    kept_W = MIN(sent_W, ceiling_W);
    settle = PARALLEL_SETTLE_PASSES;
  }
  raised_W = (uint32_t)kept_W;
  return (uint32_t)CONSTRAIN(sent_W, (uint64_t)lowest_W, ceiling_W);
}

/* The installation's power limits, before the SOC taper and the low pass filter shape them.
 *
 * With a single battery pack 1 is the installation, and the filters shape its own limits as they
 * always have, so there is nothing to do here: update_aggregate_limits() takes them over.
 *
 * With several, only the joined packs count. Same "joined" gate update_aggregate_values() uses
 * for SOC, and for the same reason: a pack held out by the voltage check, or dropped after a
 * fault, is still talking and still reporting real limits, but they are not limits on anything
 * the inverter can charge or discharge. Letting an unjoined pack's 0 W through here would stop
 * the installation on behalf of a battery that is not connected to it.
 *
 * Runs after the safety layer, so a zero it wrote on any joined pack stops the installation, and
 * before the SOC taper and the low pass filter, so both have the last word over the combined
 * limits - including the parallel raise on the charge side. Pack 2 and 3 used to be folded in
 * only after both filters had run, past either of them.
 */
void combine_pack_limits() {
  if (datalayer.system.info.configured_batteries < 2) {
    return;
  }
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;

  const DATALAYER_BATTERY_TYPE* joined[3] = {&datalayer.battery, nullptr, nullptr};  // Pack 1 is always on the link
  uint8_t pack_index[3] = {0, 0, 0};
  uint8_t count = 1;
  if (battery2 && datalayer.system.status.battery2_allowed_contactor_closing) {
    pack_index[count] = 1;
    joined[count++] = &datalayer.battery2;
  }
  if (battery3 && datalayer.system.status.battery3_allowed_contactor_closing) {
    pack_index[count] = 2;
    joined[count++] = &datalayer.battery3;
  }

  uint32_t lowest_charge_W = UINT32_MAX;
  uint32_t lowest_discharge_W = UINT32_MAX;
  for (uint8_t i = 0; i < count; i++) {
    lowest_charge_W = MIN(lowest_charge_W, joined[i]->status.max_charge_power_W);
    lowest_discharge_W = MIN(lowest_discharge_W, joined[i]->status.max_discharge_power_W);
  }

  agg.max_charge_power_W = parallel_charge_limit_W(joined, pack_index, count, lowest_charge_W);
  agg.max_discharge_power_W = lowest_discharge_W;
  /* Pack 1's, with the manual or remote cap update_calculated_values() put on it. Only what
     reaches the inverter when there is no voltage to derive the currents from the power at. */
  agg.max_charge_current_dA = datalayer.battery.status.max_charge_current_dA;
  agg.max_discharge_current_dA = datalayer.battery.status.max_discharge_current_dA;
}

/* The limits the inverter is finally told about.
 *
 * Runs last, after update_machineryprotection(), combine_pack_limits(), filter_charge_taper_soc()
 * and filter_inverter_limits(). With a single battery the filters shaped pack 1's own limits,
 * which are taken over here. With several, combine_pack_limits() put the joined packs' limits
 * into the aggregate before either filter ran, and the filters shaped them there. Either way only
 * the current caps go on top, and nothing zeroed upstream is walked back - with one deliberate
 * exception, forced recovery charge, which exists to charge an installation its BMS will not.
 */
void update_aggregate_limits() {
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;

  if (datalayer.system.info.configured_batteries < 2) {
    agg.max_charge_power_W = datalayer.battery.status.max_charge_power_W;
    agg.max_discharge_power_W = datalayer.battery.status.max_discharge_power_W;
    agg.max_charge_current_dA = datalayer.battery.status.max_charge_current_dA;
    agg.max_discharge_current_dA = datalayer.battery.status.max_discharge_current_dA;
  }

  /* Same conversion voltage and the same fallback as update_calculated_values() */
  uint16_t conversion_voltage_dV = agg.voltage_dV;
  if (conversion_voltage_dV <= 10) {
    conversion_voltage_dV = agg.max_design_voltage_dV;
  }

  /* The remote restriction while one is active, otherwise the user setting - on the power as well
     as on the current. BYD-Modbus, VCU-CAN and FoxESS-EP hand their inverter a power limit, so a
     cap on the current alone never reached them, while the main page showed it as applied.
     Deriving the current from the capped power keeps the two in step for every protocol. */
  const uint16_t charge_cap_dA = effective_charge_cap_dA();
  const uint16_t discharge_cap_dA = effective_discharge_cap_dA();
  if (conversion_voltage_dV > 10) {
    agg.max_charge_power_W = MIN(agg.max_charge_power_W, current_cap_to_power_W(charge_cap_dA, conversion_voltage_dV));
    agg.max_discharge_power_W =
        MIN(agg.max_discharge_power_W, current_cap_to_power_W(discharge_cap_dA, conversion_voltage_dV));
    agg.max_charge_current_dA = power_W_to_current_dA(agg.max_charge_power_W, conversion_voltage_dV);
    agg.max_discharge_current_dA = power_W_to_current_dA(agg.max_discharge_power_W, conversion_voltage_dV);
  }
  /* The power was rounded down, so the round trip cannot carry the current past the cap. With no
     voltage to convert at, the power is left as it is and only the current can be capped. */
  if (agg.max_charge_current_dA > charge_cap_dA) {
    agg.max_charge_current_dA = charge_cap_dA;
  }
  if (agg.max_discharge_current_dA > discharge_cap_dA) {
    agg.max_discharge_current_dA = discharge_cap_dA;
  }

  /* Forced recovery charge: a deeply discharged installation the BMS will not charge gets a small
     current anyway, for as long as update_machineryprotection() keeps the mode on - 30 minutes at
     most, ended at once by a cell below 2.0 V. It replaces what the packs allow, in either
     direction, as it always has: 5.0 A, less if the manual limit or an active remote limit is
     lower. Applied here, after every filter, as a power as well as a current, because anything
     written earlier was overwritten above. A pause or an equipment stop still stops it. */
  if (datalayer.battery_settings.user_requests_forced_charging_recovery_mode && !emulator_pause_request_ON) {
    uint16_t recovery_dA = MIN(MAX_CHARGEPOWER_RECOVERY_CHARGE_DA, datalayer.battery_settings.max_user_set_charge_dA);
    recovery_dA = MIN(recovery_dA, charge_cap_dA);
    agg.max_charge_current_dA = recovery_dA;
    if (conversion_voltage_dV > 10) {
      agg.max_charge_power_W = current_cap_to_power_W(recovery_dA, conversion_voltage_dV);
    }
  }
}
