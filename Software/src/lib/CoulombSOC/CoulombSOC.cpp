#include "CoulombSOC.h"
#include <math.h>

// Library designed by BMB IT - RO
template <typename T>
inline T myMax(T a, T b) {
    return a > b ? a : b;
}

// ================== SOC vs VOLTAGE LUT (mV/cell) - NCM ==================
static const uint16_t NMC_SOC_LUT[101] = {
   10000,9900,9900,9700,9600,9500,9400,9300,9200,9100,
   9000,8900,8800,8700,8600,8500,8400,8300,8200,8100,
   8000,7900,7800,7700,7600,7500,7400,7300,7200,7100,
   7000,6900,6800,6700,6600,6500,6400,6300,6200,6100,
   6000,5900,5800,5700,5600,5500,5400,5300,5200,5100,
   5000,4900,4800,4700,4600,4500,4400,4300,4200,4100,
   4000,3900,3800,3700,3600,3500,3400,3300,3200,3100,
   3000,2900,2800,2700,2600,2500,2400,2300,2200,2100,
   2000,1900,1800,1700,1600,1500,1400,1300,1200,1100,
   1000, 900, 800, 700, 600, 500, 400, 300, 200, 100, 0
};



static const uint16_t NMC_VOLT_LUT_mV[101] = {
  4200,4173,4148,4124,4102,4080,4060,4041,4023,4007,
  3993,3980,3969,3959,3953,3950,3941,3932,3924,3915,
  3907,3898,3890,3881,3872,3864,3855,3847,3838,3830,
  3821,3812,3804,3795,3787,3778,3770,3761,3752,3744,
  3735,3727,3718,3710,3701,3692,3684,3675,3667,3658,
  3650,3641,3632,3624,3615,3607,3598,3590,3581,3572,
  3564,3555,3547,3538,3530,3521,3512,3504,3495,3487,
  3478,3470,3461,3452,3444,3435,3427,3418,3410,3401,
  3392,3384,3375,3367,3358,3350,3338,3325,3313,3299,
  3285,3271,3255,3239,3221,3202,3180,3156,3127,3090,3000
};



// ================== SOC vs VOLTAGE LUT (mV/cell) - LFP ==================
static const uint16_t LFP_SOC_LUT[101] = {
// (≥3520 mV) → 100%
  10000,10000,10000,10000,10000,10000,10000,10000,10000,10000,
  10000,10000,10000,10000,10000,10000,10000,10000,10000,10000,
  10000,10000,10000,10000,10000,10000,10000,10000,10000,10000,
  10000,10000,10000,10000,10000,10000,10000,10000,10000,10000,
  10000,10000,10000,10000,10000,10000,10000,10000,10000,10000,
  10000,10000,10000,10000,10000,10000,10000,10000,10000,
  //(3450–3515 mV) → 99%
  9900,9900,9900,9900,9900,9900,9900,9900,9900,9900,
  // Below 3450 mV → descend to 0%
  9800,9500,9200,8900,8600,8200,7900,7600,7300,7000,
  6700,6300,6000,5700,5400,5100,4800,4400,4100,3800,
  3500,3200,2900,2500,1200,1000,800,600,500,400,
  300,0
};
static const uint16_t LFP_VOLT_LUT_mV[101] = {
  3650,3645,3640,3637,3635,3632,3630,3628,3626,3624,
  3622,3620,3618,3616,3614,3612,3610,3608,3606,3604,
  3602,3600,3598,3596,3594,3592,3590,3588,3586,3584,
  3582,3580,3578,3576,3574,3572,3570,3568,3566,3564,
  3562,3560,3558,3556,3554,3552,3550,3548,3546,3544,
  3542,3540,3538,3536,3534,3532,3530,3525,3520,3515,
  3510,3505,3500,3495,3490,3480,3470,3460,3450,3440,
  3430,3420,3410,3400,3390,3380,3370,3360,3350,3340,
  3330,3320,3310,3300,3280,3260,3240,3220,3200,3170,
  3140,3110,3080,3050,3000,2950,2900,2850,2800,2700,2500
};

// =================================================================

CoulombSOC::CoulombSOC(const CoulombSOCConfig& cfg) : cfg_(cfg) {}

void CoulombSOC::begin(uint16_t cell_max_voltage_mV,
                       uint16_t cell_min_voltage_mV) {
  avg_cell_mV_ = avgCellmV(cell_max_voltage_mV, cell_min_voltage_mV);
  lastUpdateMs_ = millis();
  
  lastActiveMs_ = lastUpdateMs_; // Initialize timer for OCV recalibration

  if (avg_cell_mV_ >= cfg_.init_min_cell_mV) {
    soc_percent_ = estimateSoCFromCellmV(avg_cell_mV_);
    initialized_ = true;
    fullLatched_  = (avg_cell_mV_ >= cfg_.vfull_on_mV);
    emptyLatched_ = (avg_cell_mV_ <= cfg_.vempty_on_mV);
  } else {
    initialized_ = false;
  }
}

void CoulombSOC::update(uint16_t cell_max_voltage_mV,
                        uint16_t cell_min_voltage_mV,
                        float    current_A) {
  const unsigned long now = millis();
  unsigned long dt_ms = now - lastUpdateMs_;
  lastUpdateMs_ = now;

  avg_cell_mV_ = avgCellmV(cell_max_voltage_mV, cell_min_voltage_mV);

  if (!initialized_) {
    if (avg_cell_mV_ >= cfg_.init_min_cell_mV) {
      soc_percent_ = estimateSoCFromCellmV(avg_cell_mV_);
      initialized_ = true;
      fullLatched_  = (avg_cell_mV_ >= cfg_.vfull_on_mV);
      emptyLatched_ = (avg_cell_mV_ <= cfg_.vempty_on_mV);
    }
    return;
  }

  applyVoltageOneShots(avg_cell_mV_);

  const unsigned long dt_cap = myMax<uint32_t>(cfg_.sample_period_ms * 3UL, 2000UL);
  if (dt_ms > dt_cap) dt_ms = dt_cap;
  if (dt_ms == 0) return;

  const float I = readCurrentProcessed(current_A);

  // Check for low current and boundary conditions for OCV recalibration
  checkAndRecalibrate(I, cell_max_voltage_mV, cell_min_voltage_mV);

  const float dt_h = (float)dt_ms / 3600000.0f;
  float dAh = I * dt_h;
  if (dAh < 0.0f) dAh *= cfg_.charge_efficiency;
  const float dPct = (dAh / cfg_.capacity_Ah) * 100.0f;

  soc_percent_ = clampf(soc_percent_ - dPct, cfg_.soc_min_percent, cfg_.soc_max_percent);

  // Cycle Counting Logic with conditional stop
  // Cycle Counting Logic
  // Only accumulate discharged Amp-hours if...
  // 1. The battery is discharging (I > current_deadband_A)
  // 2. The SoC is still above the configured minimum threshold.
  if (I > cfg_.current_deadband_A && soc_percent_ > cfg_.soc_min_percent) {
    discharged_Ah_accumulator_ += dAh;

    if (discharged_Ah_accumulator_ >= cfg_.capacity_Ah) {
      cycle_count_++;
      discharged_Ah_accumulator_ -= cfg_.capacity_Ah;
    }
  }
}

void CoulombSOC::checkAndRecalibrate(float I, uint16_t cell_max_mV, uint16_t cell_min_mV) {
  const unsigned long now = millis();

  // 1. Check instantaneous current. 
  // If the battery is active (>= cfg_.recal_rest_current_A), reset the rest stopwatch and exit.
  if (fabsf(I) >= cfg_.recal_rest_current_A) {
    lastActiveMs_ = now; 
    return;
  }

  // 2. Check how long the battery has been resting.
  // If the current has been below cfg_.recal_rest_current_A, but for less than cfg_.recal_rest_ms, exit.
  if (now - lastActiveMs_ < cfg_.recal_rest_ms) {
    return;
  }

  // 3. Check the cell voltage boundary condition:
  // TOP    = highest cell at or above vfull_off_mV
  // BOTTOM = lowest cell at or below vempty_off_mV
  const bool is_top_zone    = (cell_max_mV >= cfg_.vfull_off_mV);
  const bool is_bottom_zone = (cell_min_mV <= cfg_.vempty_off_mV);

  // 4. Check the SOC boundary condition: soc_min..recal_soc_low_pct or recal_soc_high_pct..soc_max
  // Needed when SOC drifted: e.g. SOC shows 100% so the inverter stops charging before cells reach vfull_off_mV.
  const float soc = soc_percent_;
  const bool is_low_soc  = (soc >= cfg_.soc_min_percent && soc <= cfg_.recal_soc_low_pct);
  const bool is_high_soc = (soc >= cfg_.recal_soc_high_pct && soc <= cfg_.soc_max_percent);

  if (is_top_zone || is_bottom_zone || is_low_soc || is_high_soc) {
    // We have rested for recal_rest_ms AND we are in the voltage or SOC boundary. Recalibration is needed!
    
    // Reset the rest timer immediately. 
    // This acts as a cooldown so it doesn't spam recalibrations every single 
    // loop iteration while the battery continues to rest.
    lastActiveMs_ = now; 

    // Estimate new SOC from average cell voltage using the LUTs
    float estimated_soc = estimateSoCFromCellmV(avg_cell_mV_);

    // Apply the recalibration, clamping to the user-defined limits
    soc_percent_ = clampf(estimated_soc, cfg_.soc_min_percent, cfg_.soc_max_percent);
  }
}

float CoulombSOC::getSoCPercent() const { return soc_percent_; }
uint16_t CoulombSOC::getAvgCellmV() const { return avg_cell_mV_; }

float CoulombSOC::getCapacity() const {
  return cfg_.capacity_Ah;
}

unsigned long CoulombSOC::getCycleCount() const {
  return cycle_count_;
}

void CoulombSOC::setCycleCount(unsigned long count) {
  cycle_count_ = count;
}

void CoulombSOC::setSoCPercent(float soc_pct) {
  soc_percent_ = clampf(soc_pct, cfg_.soc_min_percent, cfg_.soc_max_percent);
  fullLatched_  = (soc_percent_ >= cfg_.soc_max_percent);
  emptyLatched_ = (soc_percent_ <= cfg_.soc_min_percent);
  initialized_  = true;
}

void CoulombSOC::resetToFull() {
  soc_percent_ = cfg_.soc_max_percent;
  fullLatched_  = true;
  emptyLatched_ = false;
  initialized_  = true;
}

void CoulombSOC::resetToEmpty() {
  soc_percent_ = cfg_.soc_min_percent;
  emptyLatched_ = true;
  fullLatched_  = false;
  initialized_  = true;
}

inline float CoulombSOC::clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

inline bool CoulombSOC::isFiniteF(float x) {
  return !isnan(x) && !isinf(x);
}

uint16_t CoulombSOC::avgCellmV(uint16_t vmax, uint16_t vmin) const {
  return (uint16_t)((uint32_t)vmax + (uint32_t)vmin + 1U) / 2U;
}

float CoulombSOC::estimateSoCFromCellmV(uint16_t cell_mV) const {
	if (cfg_.LFP_Chemestry_Type == true){ 
		if (cell_mV >= LFP_VOLT_LUT_mV[0])   return 100.0f;
		if (cell_mV <= LFP_VOLT_LUT_mV[100]) return 0.0f;
	}
	else {
		if (cell_mV >= NMC_VOLT_LUT_mV[0])   return 100.0f;
		if (cell_mV <= NMC_VOLT_LUT_mV[100]) return 0.0f;
	}
  for (int i = 0; i < 100; i++) {
	      uint16_t v_hi ;
          uint16_t v_lo ;
	  if (cfg_.LFP_Chemestry_Type == true){
		v_hi = LFP_VOLT_LUT_mV[i];
		v_lo = LFP_VOLT_LUT_mV[i+1];
	  }
	  else {
		v_hi = NMC_VOLT_LUT_mV[i];
		v_lo = NMC_VOLT_LUT_mV[i+1];		  
	  }
    if (cell_mV <= v_hi && cell_mV >= v_lo) {
      const float span = float(v_hi) - float(v_lo);
      const float f = span > 0.0f ? (float(cell_mV) - float(v_lo)) / span : 0.0f;
	        float soc_x100 = 0.0;
	  if (cfg_.LFP_Chemestry_Type == true){
		soc_x100 =
         float(LFP_SOC_LUT[i+1]) + f * (float(int(LFP_SOC_LUT[i]) - int(LFP_SOC_LUT[i+1])));  
	  }
	  else {
		soc_x100 =
        float(NMC_SOC_LUT[i+1]) + f * (float(int(NMC_SOC_LUT[i]) - int(NMC_SOC_LUT[i+1])));
		}
      return clampf(soc_x100 / 100.0f, 0.0f, 100.0f);
    }
  }
  return 50.0f; // fallback
}

float CoulombSOC::readCurrentProcessed(float Iraw) const {
  float I = Iraw;
  if (cfg_.invert_current_sign) I = -I;
  if (!isFiniteF(I)) I = 0.0f;
  if (fabsf(I) < cfg_.current_deadband_A) I = 0.0f;
  return I;
}

void CoulombSOC::applyVoltageOneShots(uint16_t cell_mV) {
  if (!fullLatched_) {
    if (cell_mV >= cfg_.vfull_on_mV && soc_percent_ < cfg_.soc_max_percent) {
      soc_percent_ = cfg_.soc_max_percent;
      fullLatched_  = true;
      emptyLatched_ = false;
    }
  } else {
    if (cell_mV < cfg_.vfull_off_mV) fullLatched_ = false;
  }

  if (!emptyLatched_) {
    if (cell_mV <= cfg_.vempty_on_mV && soc_percent_ > cfg_.soc_min_percent) {
      soc_percent_  = cfg_.soc_min_percent;
      emptyLatched_ = true;
      fullLatched_  = false;
    }
  } else {
    if (cell_mV > cfg_.vempty_off_mV) emptyLatched_ = false;
  }
}

void CoulombSOC::setCapacity(float new_capacity_Ah) {
  // Only update if the new capacity is a valid, positive number
  if (new_capacity_Ah > 0.0f) {
    cfg_.capacity_Ah = new_capacity_Ah;
  }
}
