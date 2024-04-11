#ifndef __SOC_SCALING_H__
#define __SOC_SCALING_H__

#include "value_mapping.h"

class ScaledSoc {
 private:
  float min_real_soc, max_real_soc, real_soc, scaled_soc;
  bool soc_scaling_active;
  bool real_soc_initialized = false;

 public:
  ScaledSoc() : min_real_soc(20.0f), max_real_soc(80.0f), soc_scaling_active(true) {}
  ScaledSoc(float min_soc, float max_soc, bool scaling_active) {
    min_real_soc = min_soc;
    max_real_soc = max_soc;
    soc_scaling_active = scaling_active;
  }

  // Set the real soc
  float set_real_soc(float soc);
  // Get scaled soc regardless of setting
  float get_scaled_soc(void);
  // Get real soc regardless of setting
  float get_real_soc(void);
  // Get scaled or real soc based on the scaling setting
  float get_soc(void);
};
#endif
