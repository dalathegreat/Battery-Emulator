#include "battery_aggregate.h"

#include "../battery/BATTERIES.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/value_mapping.h"
#include "datalayer.h"

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
      datalayer.battery.settings.soc_scaling_active && (datalayer.system.info.configured_batteries < 2);

  if (!window_applies) {
    pack.status.reported_soc = pack.status.real_soc;
    pack.info.reported_total_capacity_Wh = pack.info.total_capacity_Wh;
    pack.status.reported_remaining_capacity_Wh = pack.status.remaining_capacity_Wh;
    return;
  }

  int32_t delta_pct = datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage;
  int32_t clamped_soc = CONSTRAIN(pack.status.real_soc, datalayer.battery.settings.min_percentage,
                                  datalayer.battery.settings.max_percentage);
  int32_t scaled_soc = 0;
  if (delta_pct != 0) {  //Safeguard against division by 0
    scaled_soc = 10000 * (clamped_soc - datalayer.battery.settings.min_percentage) / delta_pct;
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
  if (!datalayer.battery.settings.soc_scaling_active) {
    agg.reported_soc = agg.real_soc;
    agg.reported_total_capacity_Wh = agg.total_capacity_Wh;
    agg.reported_remaining_capacity_Wh = agg.remaining_capacity_Wh;
    return;
  }

  int32_t delta_pct = datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage;
  int32_t clamped_soc =
      CONSTRAIN(agg.real_soc, datalayer.battery.settings.min_percentage, datalayer.battery.settings.max_percentage);
  int32_t scaled_soc = 0;
  if (delta_pct != 0) {  //Safeguard against division by 0
    scaled_soc = 10000 * (clamped_soc - datalayer.battery.settings.min_percentage) / delta_pct;
  }
  agg.reported_soc = scaled_soc;

  if (agg.total_capacity_Wh > 0 && agg.real_soc > 0) {
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
 * Packs that are configured but have not joined the DC link yet are counted for energy exactly
 * as they were before, so the inverter's picture of the installation does not jump when the
 * contactors finally close. Their cell and temperature extremes, and their state of health, are
 * only folded in once the pack is actually talking: a configured but silent pack still holds
 * its power-on defaults (3700 mV cells, 0 dC, 99.00%), which would otherwise drag the aggregate
 * somewhere the real installation never went.
 *
 * The limit fields are deliberately not touched here - see update_aggregate_limits().
 */
void update_aggregate_values() {
  DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;

  agg.voltage_dV = datalayer.battery.status.voltage_dV;
  agg.current_dA = datalayer.battery.status.reported_current_dA;  // Already the sum of every pack
  agg.active_power_W = datalayer.battery.status.active_power_W;
  agg.real_soc = datalayer.battery.status.real_soc;
  agg.cell_max_voltage_mV = datalayer.battery.status.cell_max_voltage_mV;
  agg.cell_min_voltage_mV = datalayer.battery.status.cell_min_voltage_mV;
  agg.temperature_max_dC = datalayer.battery.status.temperature_max_dC;
  agg.temperature_min_dC = datalayer.battery.status.temperature_min_dC;
  agg.total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
  agg.remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
  agg.total_charged_battery_Wh = datalayer.battery.status.total_charged_battery_Wh;
  agg.total_discharged_battery_Wh = datalayer.battery.status.total_discharged_battery_Wh;

  /* SOC is weighted by the capacity each pack brings: a small pack sitting at 39% cannot move
     the installation as far as a large one at 41%. 64 bit on the way in - three packs at
     10000 pptt times a six figure capacity walks straight off the end of 32. */
  uint64_t soc_weighted = (uint64_t)datalayer.battery.status.real_soc * datalayer.battery.info.total_capacity_Wh;
  uint32_t soh_sum = datalayer.battery.status.soh_pptt;
  uint8_t soh_packs = 1;

  /* A pack sitting at an extreme takes the reported SOC over, so the inverter and the safety
     layer stop charging or discharging the whole installation with it. Weighting makes this
     load bearing rather than cosmetic: a full pack beside a half empty one averages out to
     something comfortable, and without the hand-over nothing would stop the charge. Pack 1 is
     checked too, for the same reason. */
  uint16_t soc_override = 0;
  bool soc_overridden = (datalayer.battery.status.real_soc < 100) || (datalayer.battery.status.real_soc > 9900);
  if (soc_overridden) {
    soc_override = datalayer.battery.status.real_soc;
  }

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
      agg.active_power_W += pack->status.active_power_W;
      agg.total_charged_battery_Wh += pack->status.total_charged_battery_Wh;
      agg.total_discharged_battery_Wh += pack->status.total_discharged_battery_Wh;
      soc_weighted += (uint64_t)pack->status.real_soc * pack->info.total_capacity_Wh;

      if (!pack_detected[i]) {
        continue;  // Never seen on the bus, its defaults are not measurements
      }

      agg.cell_max_voltage_mV = MAX(agg.cell_max_voltage_mV, pack->status.cell_max_voltage_mV);
      agg.cell_min_voltage_mV = MIN(agg.cell_min_voltage_mV, pack->status.cell_min_voltage_mV);
      agg.temperature_max_dC = MAX(agg.temperature_max_dC, pack->status.temperature_max_dC);
      agg.temperature_min_dC = MIN(agg.temperature_min_dC, pack->status.temperature_min_dC);
      soh_sum += pack->status.soh_pptt;
      soh_packs++;

      if (pack_joined[i] && ((pack->status.real_soc < 100) || (pack->status.real_soc > 9900))) {
        soc_override = pack->status.real_soc;
        soc_overridden = true;
      }
    }
  }

  if (agg.total_capacity_Wh > 0) {
    agg.real_soc = (uint16_t)(soc_weighted / agg.total_capacity_Wh);
  }
  agg.soh_pptt = soh_sum / soh_packs;

  apply_soc_window(agg);

  if (soc_overridden) {
    agg.reported_soc = soc_override;
    agg.reported_remaining_capacity_Wh = ((uint64_t)agg.reported_total_capacity_Wh * agg.reported_soc) / 10000;
  }
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
    conversion_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  }
  if (conversion_voltage_dV > 10) {
    agg.max_charge_current_dA = power_W_to_current_dA(agg.max_charge_power_W, conversion_voltage_dV);
    agg.max_discharge_current_dA = power_W_to_current_dA(agg.max_discharge_power_W, conversion_voltage_dV);
  }

  /* Apply the remote restrictions if set, otherwise the user settings */
  uint16_t charge_cap_dA = datalayer.battery.settings.remote_settings_limit_charge
                               ? datalayer.battery.settings.max_remote_set_charge_dA
                               : datalayer.battery.settings.max_user_set_charge_dA;
  if (agg.max_charge_current_dA > charge_cap_dA) {
    agg.max_charge_current_dA = charge_cap_dA;
  }
  uint16_t discharge_cap_dA = datalayer.battery.settings.remote_settings_limit_discharge
                                  ? datalayer.battery.settings.max_remote_set_discharge_dA
                                  : datalayer.battery.settings.max_user_set_discharge_dA;
  if (agg.max_discharge_current_dA > discharge_cap_dA) {
    agg.max_discharge_current_dA = discharge_cap_dA;
  }
}
