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
// zero point is calibrated, by hand or automatically.
extern uint16_t qnhck_rated_current_A;
extern uint16_t qnhck_rated_output_mV;
extern uint16_t qnhck_zero_mV;
// Measure the zero point whenever the contactors are open, instead of using the one set by hand.
// On unless switched off, and only the off state is stored.
extern bool qnhck_auto_calibration;

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

  // One ADC reading, taken at now. transmit() feeds these; public so the host tests can too.
  void add_sample(uint32_t now, uint32_t sample_mV);

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

  /* Automatic calibration, the way the Nissan LEAF learns its current sensor's offset. While
     every contactor the emulator drives is open, no current can flow through the clamp, so what
     it reads then is its zero point. The samples from AUTO_ZERO_SETTLE_MS after the contactors
     opened until they close again are gathered in 1 s buckets, closed with each window, and the
     zero point is the mean of the last AUTO_ZERO_BUCKETS of them: all of a short opening, the
     latest 10 s of one that lasts. It holds while the contactors are closed, and the next opening
     measures afresh. At boot the contactors are open, so it is known within the first seconds.
     Until then the reading is not passed on. */
  static const uint8_t AUTO_ZERO_BUCKETS = 10;
  static const uint32_t AUTO_ZERO_SETTLE_MS = 300;
  uint32_t auto_zero_bucket_sum_mV[AUTO_ZERO_BUCKETS] = {};
  uint16_t auto_zero_bucket_count[AUTO_ZERO_BUCKETS] = {};
  uint8_t auto_zero_next_bucket = 0;
  uint32_t auto_zero_sum_mV = 0;  // The bucket being filled
  uint16_t auto_zero_samples = 0;
  uint32_t auto_zero_open_since_ms = 0;
  bool auto_zero_open = false;           // Contactors seen open at the previous sample
  bool auto_zero_new_period = true;      // The next settled sample starts a new measurement
  bool auto_zero_period_result = false;  // This opening has measured a zero point, to log at its end
  bool auto_zero_rejected = false;       // This opening measured one that cannot be, already logged
  bool zero_known = false;               // Measured since boot. Only needed with automatic calibration

  static bool no_current_can_flow();
  void track_zero(uint32_t now, uint32_t sample_mV);
  void close_zero_bucket();
};

#endif
