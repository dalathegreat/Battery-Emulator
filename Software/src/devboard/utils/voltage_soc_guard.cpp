#include "voltage_soc_guard.h"
#include "../../datalayer/datalayer.h"
#include "events.h"

bool user_selected_voltage_soc_guard = false;
uint16_t user_selected_voltage_soc_cell_ir_dmohm = 5;
uint16_t user_selected_voltage_soc_band_pptt = 1000;

// NMC open-circuit voltage curve, per cell. {cell voltage in mV, SOC in integer-percent x 100}
// Values are typical for NMC/graphite; the guard band absorbs cell-to-cell curve variation.
static const struct {
  uint16_t mV;
  uint16_t soc_pptt;
} NMC_OCV[] = {
    {3000, 0},    {3300, 500},  {3450, 1000}, {3550, 2000}, {3620, 3000},  {3680, 4000},
    {3750, 5000}, {3830, 6000}, {3900, 7000}, {3970, 8000}, {4060, 9000},  {4150, 10000},
};
static const int NMC_OCV_POINTS = sizeof(NMC_OCV) / sizeof(NMC_OCV[0]);

static uint16_t soc_from_ocv_mV(int32_t cell_mV) {
  if (cell_mV <= NMC_OCV[0].mV) {
    return 0;
  }
  if (cell_mV >= NMC_OCV[NMC_OCV_POINTS - 1].mV) {
    return 10000;
  }
  for (int i = 1; i < NMC_OCV_POINTS; i++) {
    if (cell_mV <= NMC_OCV[i].mV) {
      int32_t v0 = NMC_OCV[i - 1].mV;
      int32_t v1 = NMC_OCV[i].mV;
      int32_t s0 = NMC_OCV[i - 1].soc_pptt;
      int32_t s1 = NMC_OCV[i].soc_pptt;
      return (uint16_t)(s0 + ((cell_mV - v0) * (s1 - s0)) / (v1 - v0));
    }
  }
  return 10000;
}

/** Compensate a measured cell voltage for IR drop.
 *  current_dA is positive while charging, so measured voltage sits above OCV. */
static int32_t compensated_cell_mV(uint16_t cell_mV, int16_t current_dA) {
  // mV drop = I[A] * R[mOhm] = (current_dA / 10) * (ir_dmohm / 10)
  int32_t drop_mV = ((int32_t)current_dA * (int32_t)user_selected_voltage_soc_cell_ir_dmohm) / 100;
  return (int32_t)cell_mV - drop_mV;
}

void apply_voltage_soc_guard() {
  if (!user_selected_voltage_soc_guard) {
    clear_event(EVENT_VOLTAGE_SOC_GUARD);
    return;
  }
  // OCV table only valid for NMC chemistry
  if (datalayer.battery.info.chemistry != battery_chemistry_enum::NMC) {
    return;
  }
  // Require live, sane cell data before trusting the estimate
  uint16_t cell_min = datalayer.battery.status.cell_min_voltage_mV;
  uint16_t cell_max = datalayer.battery.status.cell_max_voltage_mV;
  if (datalayer.battery.status.CAN_battery_still_alive == 0 || cell_min < 2000 || cell_max < 2000 ||
      cell_min > 4600 || cell_max > 4600 || cell_max < cell_min) {
    return;
  }

  int16_t current_dA = datalayer.battery.status.current_dA;
  uint16_t band = user_selected_voltage_soc_band_pptt;

  // The weakest cell caps how much charge can really be left (discharge protection),
  // the strongest cell floors how empty the pack can really be (overcharge protection).
  int32_t soc_of_min_cell = soc_from_ocv_mV(compensated_cell_mV(cell_min, current_dA));
  int32_t soc_of_max_cell = soc_from_ocv_mV(compensated_cell_mV(cell_max, current_dA));

  int32_t upper_bound = soc_of_min_cell + band;
  int32_t lower_bound = soc_of_max_cell - band;
  if (upper_bound > 10000) {
    upper_bound = 10000;
  }
  if (lower_bound < 0) {
    lower_bound = 0;
  }
  if (lower_bound > upper_bound) {
    // Extreme cell deviation: bounds cross. Fall back to the conservative (lower) value.
    lower_bound = upper_bound;
  }

  int32_t soc = datalayer.battery.status.real_soc;
  int32_t clamped = soc;
  if (clamped > upper_bound) {
    clamped = upper_bound;
  }
  if (clamped < lower_bound) {
    clamped = lower_bound;
  }

  if (clamped != soc) {
    // Event data byte = deviation in whole percent (capped at 255)
    uint32_t deviation_pct = (uint32_t)((soc > clamped ? soc - clamped : clamped - soc) / 100);
    if (deviation_pct > 255) {
      deviation_pct = 255;
    }
    set_event(EVENT_VOLTAGE_SOC_GUARD, (uint8_t)deviation_pct);
    datalayer.battery.status.real_soc = (uint16_t)clamped;
  } else {
    clear_event(EVENT_VOLTAGE_SOC_GUARD);
  }
}
