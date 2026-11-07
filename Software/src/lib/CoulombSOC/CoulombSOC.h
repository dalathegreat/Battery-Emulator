#pragma once
#include <Arduino.h>

struct CoulombSOCConfig {
  // USER CONFIG
  float    capacity_Ah         = 100.0f; // rated capacity
  float    charge_efficiency   = 0.99f;  // for charging (I < 0)
  float    current_deadband_A  = 0.05f;  // ignore tiny currents
  bool     invert_current_sign = false;  // flip if BMS sign opposite
  float    soc_max_percent     = 100.0f; // upper clamp/reset limit
  float    soc_min_percent     = 0.0f;   // lower clamp/reset limit

  // One-shot hysteresis thresholds (mV per cell)
  uint16_t vfull_on_mV         = 4200;   // enter FULL zone → SoC=max
  uint16_t vfull_off_mV        = 4160;   // exit FULL zone (re-arm)
  uint16_t vempty_on_mV        = 3000;   // enter EMPTY zone → SoC=min
  uint16_t vempty_off_mV       = 3040;   // exit EMPTY zone (re-arm)

  uint32_t sample_period_ms    = 100;
  uint32_t recal_rest_ms       = 10000;  // continuous rest (|I| < recal_rest_current_A) before voltage recalibration
  float    recal_rest_current_A = 0.1f;  // below this |I| counts as rest (values below current_deadband_A act as the deadband)
  float    recal_soc_low_pct    = 10.0f; // SOC window soc_min_percent..this also triggers recalibration (catches SOC drift)
  float    recal_soc_high_pct   = 95.0f; // SOC window this..soc_max_percent also triggers recalibration (catches SOC drift)
  uint16_t init_min_cell_mV    = 1000;
  bool     LFP_Chemestry_Type = false;  // If we have LFP Battery = true , NCM = default FALSE for SOC Estimation at start
};

class CoulombSOC {
public:
  explicit CoulombSOC(const CoulombSOCConfig& cfg = CoulombSOCConfig());

  void begin(uint16_t cell_max_voltage_mV, uint16_t cell_min_voltage_mV);
  void update(uint16_t cell_max_voltage_mV, uint16_t cell_min_voltage_mV, float current_A);

  // Output
  float    getSoCPercent() const;
  uint16_t getAvgCellmV() const;
  float    getCapacity() const;

  // Public methods for cycle count
  unsigned long getCycleCount() const;
  void setCycleCount(unsigned long count);

  // Optional manual overrides
  void     setSoCPercent(float soc_pct);
  void     resetToFull();
  void     resetToEmpty();
  void     setCapacity(float new_capacity_Ah);

private:
  CoulombSOCConfig cfg_;

  // State
  float    soc_percent_  = 100.0f;
  bool     fullLatched_  = false;
  bool     emptyLatched_ = false;
  uint16_t avg_cell_mV_  = 0;
  bool     initialized_  = false;
  unsigned long cycle_count_ = 0;
  float discharged_Ah_accumulator_ = 0.0f;

  unsigned long lastUpdateMs_ = 0;
  //unsigned long lastRecalibrateMs_ = 0;   // Timer for periodic OCV check
  unsigned long lastActiveMs_ = 0;			// Timer for periodic OCV check

  // Helpers
  static inline float clampf(float x, float lo, float hi);
  static inline bool  isFiniteF(float x);
  uint16_t            avgCellmV(uint16_t vmax, uint16_t vmin) const;
  float estimateSoCFromCellmV(uint16_t cell_mV) const;
  float readCurrentProcessed(float Iraw) const;
  void  applyVoltageOneShots(uint16_t cell_mV);
  void  checkAndRecalibrate(float I, uint16_t cell_max_mV, uint16_t cell_min_mV); // New recalibration check
};

// Assuming the original file uses an unnamed namespace or similar closure
// }
