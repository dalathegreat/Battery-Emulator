#include "soc_scaling.h"

float ScaledSoc::set_real_soc(float soc) {
  real_soc = soc;
  scaled_soc = map_float(real_soc, 0.0f, 100.0f, min_real_soc, max_real_soc);
  real_soc_initialized = true;
  return scaled_soc;
}

float ScaledSoc::get_scaled_soc(void) {
  if (real_soc_initialized) {
    return scaled_soc;
  } else {
    return -1.0f;
  }
}

float ScaledSoc::get_real_soc(void) {
  if (real_soc_initialized) {
    return real_soc;
  } else {
    return -1.0f;
  }
}

float ScaledSoc::get_soc(void) {
  if (real_soc_initialized) {
    if (soc_scaling_active) {
      return scaled_soc;
    } else {
      return real_soc;
    }
  } else {
    return -1.0f;
  }
}
