#ifndef QNHCK2_16_H
#define QNHCK2_16_H

#include <soc/gpio_num.h>
#include "Shunt.h"

/* QNHCK2-16, 3.3 V version: an open loop, split core Hall effect current sensor. Its output sits at
   1.65 V with no current flowing, and moves by the rated output at the rated current - upwards
   for current in the direction marked on the sensor. It is read through an ADC pin, so it is
   only offered on boards whose HAL provides one (SHUNT_ADC_PIN). */

struct QnhckModelOption {
  uint16_t value;
  const char* label;
};

// Rated current (Ipn) of the models, in A. Each measures up to 1.1 x Ipn in both directions.
inline constexpr QnhckModelOption QNHCK_RATED_CURRENTS[] = {{10, "10 A (±11 A)"},    {20, "20 A (±22 A)"},
                                                            {30, "30 A (±33 A)"},    {50, "50 A (±55 A)"},
                                                            {100, "100 A (±110 A)"}, {150, "150 A (±165 A)"}};

// Rated output of the models: how far the output moves from 1.65 V at Ipn, in mV.
inline constexpr QnhckModelOption QNHCK_RATED_OUTPUTS[] = {{625, "1.65 ± 0.625 V"},
                                                           {1000, "1.65 ± 1 V"},
                                                           {1250, "1.65 ± 1.25 V"},
                                                           {1500, "1.65 ± 1.5 V"},
                                                           {1650, "1.65 ± 1.65 V"}};

template <size_t N>
constexpr bool qnhck_is_model(const QnhckModelOption (&options)[N], uint32_t value) {
  for (const auto& option : options) {
    if (option.value == value) {
      return true;
    }
  }
  return false;
}

static constexpr uint16_t QNHCK_DEFAULT_RATED_CURRENT_A = 50;
static constexpr uint16_t QNHCK_DEFAULT_RATED_OUTPUT_MV = 625;
// Output at 0 A per the datasheet. Only a calibrated value that differs from it is stored.
static constexpr uint16_t QNHCK_NOMINAL_ZERO_MV = 1650;
// How far a zero point may sit from nominal. The datasheet allows 1% offset, 25 mV magnetic
// offset and 1 mV/°C of drift; this leaves room for the ADC's own error too, yet rejects an
// unpowered sensor, a floating pin or a 5 V version (2.5 V at 0 A).
static constexpr uint16_t QNHCK_ZERO_TOLERANCE_MV = 200;

// Settings page values, loaded from NVM at boot. qnhck_zero_mV also changes at runtime, when the
// zero point is calibrated.
extern uint16_t qnhck_rated_current_A;
extern uint16_t qnhck_rated_output_mV;
extern uint16_t qnhck_zero_mV;

// Whether a reading taken with no current flowing can be the sensor's zero point.
bool qnhck_zero_plausible(uint32_t zero_mV);

// Current through the sensor in mA, from its averaged output in uV. Positive in the direction
// marked on the sensor.
int32_t qnhck_current_mA(uint32_t output_uV, uint16_t zero_mV, uint16_t rated_current_A, uint16_t rated_output_mV);

// Whether a current can have come from a working sensor: within its measuring range of 1.1 x Ipn,
// plus a margin for its own and the ADC's error. Beyond that it is saturated, or not working at
// all - an unpowered one reads 0 V, which is -2.64 x Ipn on the ±0.625 V model.
bool qnhck_current_in_range(int32_t current_mA, uint16_t rated_current_A);

class Qnhck2_16Shunt : public Shunt, public Transmitter {
 public:
  static constexpr const char* Name = "QNHCK2-16 (3.3V)";

  void setup() override;
  const char* interface_name() override { return interface_label; }
  bool calibrate_zero(uint16_t& reading_mV) override;

  // Called on every pass of the core task. Nothing is transmitted: the tick paces the sampling.
  void transmit(unsigned long currentMillis) override;

 private:
  gpio_num_t pin = GPIO_NUM_NC;
  char interface_label[16] = "ADC";

  // One sample per millisecond, averaged over one second. A whole second holds whole periods of
  // 50/100 Hz ripple, so the ripple drops out of the mean.
  uint32_t window_start_ms = 0;
  uint32_t last_sample_ms = 0;
  uint32_t window_sum_mV = 0;
  uint16_t window_samples = 0;

  // Sensor output averaged over the last complete window, in uV. 0 until one has completed.
  uint32_t output_uV = 0;

  // The last window read beyond the sensor's range, so the batteries' own current is in use
  bool out_of_range = false;
};

#endif
