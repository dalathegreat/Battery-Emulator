#include "cell_soc_estimator.h"

namespace {

// Define the data points for %SOC depending on cell voltage
const uint8_t numPoints = 100;

const uint16_t SOC[101] = {10000, 9900, 9800, 9700, 9600, 9500, 9400, 9300, 9200, 9100, 9000, 8900, 8800, 8700, 8600,
                           8500,  8400, 8300, 8200, 8100, 8000, 7900, 7800, 7700, 7600, 7500, 7400, 7300, 7200, 7100,
                           7000,  6900, 6800, 6700, 6600, 6500, 6400, 6300, 6200, 6100, 6000, 5900, 5800, 5700, 5600,
                           5500,  5400, 5300, 5200, 5100, 5000, 4900, 4800, 4700, 4600, 4500, 4400, 4300, 4200, 4100,
                           4000,  3900, 3800, 3700, 3600, 3500, 3400, 3300, 3200, 3100, 3000, 2900, 2800, 2700, 2600,
                           2500,  2400, 2300, 2200, 2100, 2000, 1900, 1800, 1700, 1600, 1500, 1400, 1300, 1200, 1100,
                           1000,  900,  800,  700,  600,  500,  400,  300,  200,  100,  0};

// LFP cell voltage lookup table (2900mV - 3600mV) with flat middle section at 3.1V-3.3V
const uint16_t lfpVoltageLookup[101] = {
    3600, 3570, 3540, 3510, 3490, 3470, 3450, 3430, 3410, 3390,  // 100%-91% - Steep drop from full
    3370, 3350, 3330, 3320, 3315, 3310, 3305, 3300, 3295, 3290,  // 90%-81% - Transition to flat zone
    // Flat region - typical LFP working range (80%-20%) - 3.3V to 3.1V
    3285, 3280, 3275, 3270, 3265, 3260, 3255, 3250, 3245, 3240,  // 80%-71%
    3235, 3230, 3225, 3220, 3215, 3210, 3205, 3200, 3195, 3190,  // 70%-61%
    3185, 3180, 3175, 3170, 3165, 3160, 3155, 3150, 3145, 3140,  // 60%-51%
    3135, 3130, 3125, 3120, 3115, 3110, 3105, 3100, 3095, 3090,  // 50%-41%
    3085, 3080, 3075, 3070, 3065, 3060, 3055, 3050, 3045, 3040,  // 40%-31%
    3035, 3030, 3025, 3020, 3015, 3010, 3005, 3000, 2995, 2990,  // 30%-21%
    // End of flat region, steep drop to empty
    2980, 2960, 2940, 2920, 2910, 2905, 2902, 2901, 2900, 2890,  // 20%-11%
    2880, 2870, 2860, 2850, 2840, 2830, 2820, 2810, 2805, 2803,  // 10%-1%
    2800                                                         // 0%
};

// NMC/NCA cell voltage lookup table (3000mV - 4200mV)
const uint16_t nmcVoltageLookup[101] = {
    4200, 4173, 4148, 4124, 4102, 4080, 4060, 4041, 4023, 4007, 3993, 3980, 3969, 3959, 3953, 3950, 3941,
    3932, 3924, 3915, 3907, 3898, 3890, 3881, 3872, 3864, 3855, 3847, 3838, 3830, 3821, 3812, 3804, 3795,
    3787, 3778, 3770, 3761, 3752, 3744, 3735, 3727, 3718, 3710, 3701, 3692, 3684, 3675, 3667, 3658, 3650,
    3641, 3632, 3624, 3615, 3607, 3598, 3590, 3581, 3572, 3564, 3555, 3547, 3538, 3530, 3521, 3512, 3504,
    3495, 3487, 3478, 3470, 3461, 3452, 3444, 3435, 3427, 3418, 3410, 3401, 3392, 3384, 3375, 3367, 3358,
    3350, 3338, 3325, 3313, 3299, 3285, 3271, 3255, 3239, 3221, 3202, 3180, 3156, 3127, 3090, 3000};

uint16_t interpolate_soc(uint16_t cellVoltage, const uint16_t* voltageLookup) {
  if (cellVoltage >= voltageLookup[0]) {
    return SOC[0];
  }
  if (cellVoltage <= voltageLookup[numPoints - 1]) {
    return SOC[numPoints - 1];
  }

  for (int i = 1; i < numPoints; ++i) {
    if (cellVoltage >= voltageLookup[i]) {
      // Cast to float for proper division
      float t = (float)(cellVoltage - voltageLookup[i]) / (float)(voltageLookup[i - 1] - voltageLookup[i]);

      // Calculate interpolated SOC value
      uint16_t socDiff = SOC[i - 1] - SOC[i];
      uint16_t interpolatedValue = SOC[i] + (uint16_t)(t * socDiff);

      return interpolatedValue;
    }
  }
  return 0;  // Default return for safety, should never reach here
}

}  // namespace

uint16_t soc_from_cell_voltage(uint16_t cell_voltage_mV, battery_chemistry_enum chemistry) {
  switch (chemistry) {
    case battery_chemistry_enum::LFP:
      return interpolate_soc(cell_voltage_mV, lfpVoltageLookup);
    case battery_chemistry_enum::NCA:
    case battery_chemistry_enum::NMC:
    case battery_chemistry_enum::ZEBRA:
    case battery_chemistry_enum::Autodetect:
    default:
      // No dedicated curve for these yet, fall back to a generic Li-ion (NMC/NCA) curve
      return interpolate_soc(cell_voltage_mV, nmcVoltageLookup);
  }
}

uint16_t soc_from_min_max_cell_voltage(uint16_t min_cell_voltage_mV, uint16_t max_cell_voltage_mV,
                                       battery_chemistry_enum chemistry) {
  uint16_t soc_from_max = soc_from_cell_voltage(max_cell_voltage_mV, chemistry);
  uint16_t soc_from_min = soc_from_cell_voltage(min_cell_voltage_mV, chemistry);

  if (soc_from_max >= 5000) {  // Upper half of range, trust the highest cell (avoid overcharging it)
    return soc_from_max;
  }
  return soc_from_min;  // Lower half of range, trust the lowest cell (avoid over-discharging it)
}

uint16_t cell_voltage_table_max_mV(battery_chemistry_enum chemistry) {
  switch (chemistry) {
    case battery_chemistry_enum::LFP:
      return lfpVoltageLookup[0];
    case battery_chemistry_enum::NCA:
    case battery_chemistry_enum::NMC:
    case battery_chemistry_enum::ZEBRA:
    case battery_chemistry_enum::Autodetect:
    default:
      return nmcVoltageLookup[0];
  }
}

uint16_t cell_voltage_table_min_mV(battery_chemistry_enum chemistry) {
  switch (chemistry) {
    case battery_chemistry_enum::LFP:
      return lfpVoltageLookup[numPoints - 1];
    case battery_chemistry_enum::NCA:
    case battery_chemistry_enum::NMC:
    case battery_chemistry_enum::ZEBRA:
    case battery_chemistry_enum::Autodetect:
    default:
      return nmcVoltageLookup[numPoints - 1];
  }
}

bool cell_voltage_range_matches_chemistry(uint16_t min_cell_voltage_mV, uint16_t max_cell_voltage_mV,
                                          battery_chemistry_enum chemistry) {
  // Real protection setpoints are often configured a bit past the curve's 0%/100% points
  // (e.g. an LFP pack protecting at 3.65V even though the curve tops out at 3.60V), so this
  // margin absorbs that without flagging correctly-configured packs.
  const uint16_t kMarginMv = 200;

  uint16_t table_max = cell_voltage_table_max_mV(chemistry);
  uint16_t table_min = cell_voltage_table_min_mV(chemistry);

  return max_cell_voltage_mV <= table_max + kMarginMv && min_cell_voltage_mV >= table_min - kMarginMv;
}
