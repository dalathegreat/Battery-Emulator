#include "QNHCK2-16.h"
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "../devboard/utils/logging.h"

#ifdef UNIT_TEST
// The host build has no ADC driver, provide the attenuation it is configured with
typedef enum { ADC_0db = 0, ADC_2_5db, ADC_6db, ADC_11db } adc_attenuation_t;
#endif

uint16_t qnhck_rated_current_A = QNHCK_DEFAULT_RATED_CURRENT_A;
uint16_t qnhck_rated_output_mV = QNHCK_DEFAULT_RATED_OUTPUT_MV;
uint16_t qnhck_zero_mV = QNHCK_NOMINAL_ZERO_MV;

bool qnhck_zero_plausible(uint32_t zero_mV) {
  constexpr uint32_t lowest_mV = QNHCK_NOMINAL_ZERO_MV - QNHCK_ZERO_TOLERANCE_MV;
  constexpr uint32_t highest_mV = QNHCK_NOMINAL_ZERO_MV + QNHCK_ZERO_TOLERANCE_MV;
  return zero_mV >= lowest_mV && zero_mV <= highest_mV;
}

int32_t qnhck_current_mA(uint32_t output_uV, uint16_t zero_mV, uint16_t rated_current_A, uint16_t rated_output_mV) {
  if (rated_output_mV == 0) {
    return 0;
  }
  // uV x A / mV comes out in mA. Rounded half away from zero, so a reading sitting exactly on
  // the zero point stays 0 and positive and negative currents round alike.
  const int64_t scaled = ((int64_t)output_uV - (int64_t)zero_mV * 1000) * rated_current_A;
  const int64_t half = rated_output_mV / 2;
  return (int32_t)((scaled >= 0 ? scaled + half : scaled - half) / rated_output_mV);
}

bool qnhck_current_in_range(int32_t current_mA, uint16_t rated_current_A) {
  const int32_t limit_mA = (int32_t)rated_current_A * 1200;  // 1.2 x Ipn
  return current_mA >= -limit_mA && current_mA <= limit_mA;
}

void Qnhck2_16Shunt::setup() {
  // Rated output as volts with three decimals, the way the models are specified
  const unsigned output_V = qnhck_rated_output_mV / 1000;
  const unsigned output_mV = qnhck_rated_output_mV % 1000;

  // Named even if the pin cannot be had, so the main page says what was configured
  snprintf(datalayer.system.info.shunt_protocol, sizeof(datalayer.system.info.shunt_protocol),
           "QNHCK2-16 %u A, ±%u.%03u V", (unsigned)qnhck_rated_current_A, output_V, output_mV);

  pin = esp32hal->SHUNT_ADC_PIN();
  if (!esp32hal->alloc_pins(Name, pin)) {
    return;  // alloc_pins() has raised the event that says why
  }
  snprintf(interface_label, sizeof(interface_label), "ADC (GPIO%d)", (int)pin);

  // The first read sets the pin up as an ADC input, which setting its attenuation needs. 11 dB
  // gives the widest range, about 0.1-3.1 V, and is also what analogReadMilliVolts() has its
  // calibration curve made for.
  analogReadMilliVolts(pin);
  analogSetPinAttenuation(pin, ADC_11db);

  window_start_ms = millis();
  last_sample_ms = window_start_ms;
  register_transmitter(this);

  // The inverter is given this current instead of what the batteries report
  datalayer.shunt.replaces_battery_current = true;

  logging.printf("QNHCK2-16 on GPIO%d: %u A, ±%u.%03u V, zero point %u mV\n", (int)pin, (unsigned)qnhck_rated_current_A,
                 output_V, output_mV, (unsigned)qnhck_zero_mV);
}

void Qnhck2_16Shunt::transmit(unsigned long currentMillis) {
  const uint32_t now = (uint32_t)currentMillis;
  if (now == last_sample_ms) {
    return;  // One sample per millisecond
  }
  last_sample_ms = now;
  window_sum_mV += analogReadMilliVolts(pin);
  window_samples++;

  if (now - window_start_ms < INTERVAL_1_S) {
    return;
  }

  // Averaged in uV, so the mean of a thousand whole mV samples keeps its fraction
  output_uV = (uint32_t)(((uint64_t)window_sum_mV * 1000 + window_samples / 2) / window_samples);
  const int32_t current_mA = qnhck_current_mA(output_uV, qnhck_zero_mV, qnhck_rated_current_A, qnhck_rated_output_mV);
  datalayer.shunt.measured_amperage_mA = current_mA;
  datalayer.shunt.measured_avg1S_amperage_mA = current_mA;  // It is a one second mean already

  // A reading no working sensor can give is not passed on to the inverter: the batteries' own
  // current stands in until the reading is back within range.
  const bool in_range = qnhck_current_in_range(current_mA, qnhck_rated_current_A);
  if (in_range == out_of_range) {
    out_of_range = !in_range;
    if (out_of_range) {
      LOG_SET_NEXT_SEVERITY(4);  // warning
      logging.printf("QNHCK2-16 reads %ld mA, beyond its range. Using the current the batteries report.\n",
                     (long)current_mA);
    } else {
      LOG_SET_NEXT_SEVERITY(5);  // notice
      logging.println("QNHCK2-16 reading back within range.");
    }
  }
  datalayer.shunt.available = in_range;

  window_start_ms = now;
  window_sum_mV = 0;
  window_samples = 0;
}

bool Qnhck2_16Shunt::calibrate_zero(uint16_t& reading_mV) {
  // The mean of the last complete second, which the core task keeps up to date. Nothing is
  // sampled here, so the web server never touches the ADC.
  reading_mV = (uint16_t)((output_uV + 500) / 1000);
  if (reading_mV == 0 || !qnhck_zero_plausible(reading_mV)) {
    return false;
  }
  qnhck_zero_mV = reading_mV;  // In use from the next window on
  return true;
}
