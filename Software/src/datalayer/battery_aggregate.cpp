#include "battery_aggregate.h"

#include "../battery/BATTERIES.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/value_mapping.h"
#include "datalayer.h"

/* SOC at which the aggregate starts blending from the emptiest pack towards the fullest one,
   in integer-percent x 100. Below this the emptiest pack has it alone. */
static constexpr uint16_t SOC_BLEND_START_PPTT = 9000;

/* The safety layer, the SOC taper and the low pass filter all rewrite a pack's power limits in
   place, so by the time the web page renders, what the BMS actually asked for is gone. Keep a
   copy while it is still the driver's own number. */
void snapshot_bms_limits(DATALAYER_BATTERY_TYPE& pack) {
  pack.status.bms_max_charge_power_W = pack.status.max_charge_power_W;
  pack.status.bms_max_discharge_power_W = pack.status.max_discharge_power_W;
}

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
 * The limit fields are deliberately not touched here - see update_aggregate_limits().
 */
void update_aggregate_values() {
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;

  agg.voltage_dV = datalayer.battery.status.voltage_dV;
  agg.current_dA = datalayer.battery.status.reported_current_dA;  // Already the sum of every pack
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

/* The limits the inverter is finally told about.
 *
 * Runs last, after update_machineryprotection(), filter_charge_taper_soc() and
 * filter_inverter_limits() have all shaped datalayer.battery. Each of them keeps working on a
 * single pack's numbers, and nothing they zero can be walked back here - capping to the
 * weakest pack can only ever lower the result. Doing it here rather than before the safety
 * layer also means a pack 2 or pack 3 fault that zeroes that pack's limits now reaches the
 * inverter, which it did not when the cap ran first.
 */
void update_aggregate_limits() {
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;

  /* Cap max charge/discharge to the lowest battery's limits */
  agg.max_charge_power_W = datalayer.battery.status.max_charge_power_W;
  agg.max_discharge_power_W = datalayer.battery.status.max_discharge_power_W;
  if (battery2) {
    agg.max_charge_power_W = MIN(agg.max_charge_power_W, datalayer.battery2.status.max_charge_power_W);
    agg.max_discharge_power_W = MIN(agg.max_discharge_power_W, datalayer.battery2.status.max_discharge_power_W);
  }
  if (battery3) {
    agg.max_charge_power_W = MIN(agg.max_charge_power_W, datalayer.battery3.status.max_charge_power_W);
    agg.max_discharge_power_W = MIN(agg.max_discharge_power_W, datalayer.battery3.status.max_discharge_power_W);
  }

  /* Same power to current conversion and the same fallback as update_calculated_values() */
  agg.max_charge_current_dA = datalayer.battery.status.max_charge_current_dA;
  agg.max_discharge_current_dA = datalayer.battery.status.max_discharge_current_dA;
  uint16_t conversion_voltage_dV = agg.voltage_dV;
  if (conversion_voltage_dV <= 10) {
    conversion_voltage_dV = agg.max_design_voltage_dV;
  }
  if (conversion_voltage_dV > 10) {
    agg.max_charge_current_dA = power_W_to_current_dA(agg.max_charge_power_W, conversion_voltage_dV);
    agg.max_discharge_current_dA = power_W_to_current_dA(agg.max_discharge_power_W, conversion_voltage_dV);
  }

  /* Apply the remote restrictions if set, otherwise the user settings */
  uint16_t charge_cap_dA = datalayer.battery_settings.remote_settings_limit_charge
                               ? datalayer.battery_settings.max_remote_set_charge_dA
                               : datalayer.battery_settings.max_user_set_charge_dA;
  if (agg.max_charge_current_dA > charge_cap_dA) {
    agg.max_charge_current_dA = charge_cap_dA;
  }
  uint16_t discharge_cap_dA = datalayer.battery_settings.remote_settings_limit_discharge
                                  ? datalayer.battery_settings.max_remote_set_discharge_dA
                                  : datalayer.battery_settings.max_user_set_discharge_dA;
  if (agg.max_discharge_current_dA > discharge_cap_dA) {
    agg.max_discharge_current_dA = discharge_cap_dA;
  }
}
