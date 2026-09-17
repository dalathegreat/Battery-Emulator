#include "Battery.h"
#include "../datalayer/datalayer.h"

float Battery::get_voltage() {
  return static_cast<float>(datalayer.battery.status.voltage_dV) / 10.0f;
}

void Battery::safety_current_range_dA(int16_t& max_dA, int16_t& min_dA) {
  int16_t current_dA = datalayer.battery.status.current_dA;
  if (battery_index == 2) {
    current_dA = datalayer.battery2.status.current_dA;
  } else if (battery_index == 3) {
    current_dA = datalayer.battery3.status.current_dA;
  }
  max_dA = current_dA;
  min_dA = current_dA;
}
