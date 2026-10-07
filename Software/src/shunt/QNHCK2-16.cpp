#ifndef SMALL_FLASH_DEVICE  // Left out of the small flash devices

#include "QNHCK2-16.h"
#include <Arduino.h>
#include "../battery/BATTERIES.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/logging.h"

#ifdef UNIT_TEST
// The host build has no ADC driver, provide the attenuation it is configured with
typedef enum { ADC_0db = 0, ADC_2_5db, ADC_6db, ADC_11db } adc_attenuation_t;
#endif

uint16_t qnhck_rated_current_A = QNHCK_DEFAULT_RATED_CURRENT_A;
uint16_t qnhck_rated_output_mV = QNHCK_DEFAULT_RATED_OUTPUT_MV;
uint16_t qnhck_zero_mV = QNHCK_NOMINAL_ZERO_MV;
bool qnhck_auto_calibration = true;
int16_t qnhck_zero_tempco_uV_per_C = 0;

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

bool qnhck_battery_temperature_dC(int16_t& temperature_dC) {
  // Pack 1 is the one every installation has. Until it is heard from, its temperatures are the
  // datalayer's defaults rather than readings. The aggregate takes in packs 2 and 3 once they
  // are heard from too.
  if (!battery_detected || !datalayer.battery.status.CAN_battery_still_alive) {
    return false;
  }
  const int32_t midway_dC =
      ((int32_t)datalayer.aggregate.temperature_min_dC + datalayer.aggregate.temperature_max_dC) / 2;
  if (midway_dC < -250 || midway_dC > 850) {
    return false;  // Beyond the sensor's operating range, which no zero point drift covers
  }
  temperature_dC = (int16_t)midway_dC;
  return true;
}

int32_t qnhck_zero_drift_uV(int16_t tempco_uV_per_C, int16_t baseline_dC, int16_t now_dC) {
  return (int32_t)tempco_uV_per_C * ((int32_t)now_dC - baseline_dC) / 10;
}

// A value in tenths with one decimal, for the log: a temperature in deci-°C, a drift in 0.1 mV
static void format_tenths(char* text, size_t size, int32_t tenths) {
  const int32_t magnitude = abs(tenths);
  snprintf(text, size, "%s%ld.%ld", tenths < 0 ? "-" : "", (long)(magnitude / 10), (long)(magnitude % 10));
}

// The rated output the way the models are labelled: 0.625, 1, 1.25, 1.5 or 1.65
static void format_rated_output(char* text, size_t size) {
  snprintf(text, size, "%u.%03u", (unsigned)(qnhck_rated_output_mV / 1000), (unsigned)(qnhck_rated_output_mV % 1000));
  size_t end = strlen(text);
  while (end > 0 && text[end - 1] == '0') {
    end--;
  }
  if (end > 0 && text[end - 1] == '.') {
    end--;
  }
  text[end] = '\0';
}

void Qnhck2_16Shunt::setup() {
  char output_V[8];
  format_rated_output(output_V, sizeof(output_V));

  // Named even if the pin cannot be had, so the main page says what was configured
  snprintf(datalayer.system.info.shunt_protocol, sizeof(datalayer.system.info.shunt_protocol), "QNHCK2-16 (%uA ±%sV)",
           (unsigned)qnhck_rated_current_A, output_V);

  pin = esp32hal->SHUNT_ADC_PIN();
  if (!esp32hal->alloc_pins(Name, pin)) {
    return;  // alloc_pins() has raised the event that says why
  }
  snprintf(interface_label, sizeof(interface_label), "ADC (GPIO%d)", (int)pin);

  // The first read sets the pin up as an ADC input, which setting its attenuation needs. 11 dB
  // gives the widest range, 0-2.9 V on the ESP32-S3 according to its datasheet, and is also what
  // analogReadMilliVolts() has its calibration curve made for.
  analogReadMilliVolts(pin);
  analogSetPinAttenuation(pin, ADC_11db);

  window_start_ms = millis();
  last_sample_ms = window_start_ms;
  register_transmitter(this);

  // The inverter is given this current instead of what the batteries report
  datalayer.shunt.replaces_battery_current = true;

  if (qnhck_auto_calibration) {
    logging.printf("QNHCK2-16 on GPIO%d: %u A ±%s V, zero point measured while the contactors are open\n", (int)pin,
                   (unsigned)qnhck_rated_current_A, output_V);
    if (qnhck_zero_tempco_uV_per_C != 0) {
      char drift_mV[8];
      format_tenths(drift_mV, sizeof(drift_mV), qnhck_zero_tempco_uV_per_C / 100);
      logging.printf("QNHCK2-16: zero point follows the batteries' temperature by %s mV/°C\n", drift_mV);
    }
    const bool all_contactors_ours = contactor_control_enabled &&
                                     (!battery2 || contactor_control_enabled_double_battery) &&
                                     (!battery3 || contactor_control_enabled_triple_battery);
    if (!all_contactors_ours) {
      LOG_SET_NEXT_SEVERITY(4);  // warning
      logging.println(
          "QNHCK2-16: automatic calibration needs contactor control via GPIO for every battery. Its reading is not "
          "used until then.");
    }
  } else {
    logging.printf("QNHCK2-16 on GPIO%d: %u A ±%s V, zero point %u mV\n", (int)pin, (unsigned)qnhck_rated_current_A,
                   output_V, (unsigned)qnhck_zero_mV);
  }
}

void Qnhck2_16Shunt::transmit(unsigned long currentMillis) {
  const uint32_t now = (uint32_t)currentMillis;
  if (now == last_sample_ms) {
    return;  // One sample per millisecond
  }
  last_sample_ms = now;
  add_sample(now, analogReadMilliVolts(pin));
}

void Qnhck2_16Shunt::add_sample(uint32_t now, uint32_t sample_mV) {
  window_sum_mV += sample_mV;
  window_samples++;
  if (qnhck_auto_calibration) {
    track_zero(now, sample_mV);
  }

  if (now - window_start_ms < INTERVAL_1_S) {
    return;
  }

  if (qnhck_auto_calibration) {
    close_zero_bucket();  // Before the current is worked out, so it uses the zero point of this second
  }

  // Averaged in uV, so the mean of a thousand whole mV samples keeps its fraction
  output_uV = (uint32_t)(((uint64_t)window_sum_mV * 1000 + window_samples / 2) / window_samples);
  const int32_t current_mA =
      qnhck_current_mA(compensated_output_uV(), qnhck_zero_mV, qnhck_rated_current_A, qnhck_rated_output_mV);
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
  // With automatic calibration, not before the zero point has been measured either
  datalayer.shunt.available = in_range && (zero_known || !qnhck_auto_calibration);

  window_start_ms = now;
  window_sum_mV = 0;
  window_samples = 0;
}

/* Whether the clamp can be carrying current. Only the contactors the emulator drives say for sure:
   pack 1's are open until the closing ladder starts (0) and once a fault has latched them open (2),
   and packs 2 and 3 join the DC link through a contactor of their own, only ever closed once pack
   1's are. A pack without one under the emulator's control may be on the link regardless. */
bool Qnhck2_16Shunt::no_current_can_flow() {
  if (!contactor_control_enabled) {
    return false;
  }
  const uint8_t state = datalayer.system.status.contactors_engaged;
  if (state != 0 && state != 2) {
    return false;
  }
  if (battery2 && (!contactor_control_enabled_double_battery || datalayer.system.status.contactors_battery2_engaged)) {
    return false;
  }
  if (battery3 && (!contactor_control_enabled_triple_battery || datalayer.system.status.contactors_battery3_engaged)) {
    return false;
  }
  return true;
}

// Called for every sample while the automatic calibration is enabled
void Qnhck2_16Shunt::track_zero(uint32_t now, uint32_t sample_mV) {
  if (!no_current_can_flow()) {
    if (auto_zero_open) {
      // The contactors have just closed, which completes this opening's measurement
      close_zero_bucket();
      if (auto_zero_period_result) {
        LOG_SET_NEXT_SEVERITY(5);  // notice
        if (zero_temperature_known) {
          // Two of these at different temperatures give the drift: their difference in mV over
          // their difference in °C
          char temperature[8];
          format_tenths(temperature, sizeof(temperature), zero_temperature_dC);
          logging.printf("QNHCK2-16 zero point calibrated to %u mV at %s °C\n", (unsigned)qnhck_zero_mV, temperature);
        } else {
          logging.printf("QNHCK2-16 zero point calibrated to %u mV\n", (unsigned)qnhck_zero_mV);
        }
      }
    }
    auto_zero_open = false;
    auto_zero_new_period = true;
    auto_zero_period_result = false;
    return;
  }
  if (!auto_zero_open) {
    auto_zero_open = true;
    auto_zero_open_since_ms = now;
  }
  if ((now - auto_zero_open_since_ms) < AUTO_ZERO_SETTLE_MS) {
    return;  // Let whatever was flowing as the contactors opened die away
  }

  if (auto_zero_new_period) {
    // First settled sample since the contactors opened: what the previous opening measured goes
    auto_zero_new_period = false;
    auto_zero_rejected = false;
    memset(auto_zero_bucket_sum_mV, 0, sizeof(auto_zero_bucket_sum_mV));
    memset(auto_zero_bucket_count, 0, sizeof(auto_zero_bucket_count));
    auto_zero_next_bucket = 0;
    auto_zero_sum_mV = 0;
    auto_zero_samples = 0;
  }
  auto_zero_sum_mV += sample_mV;
  auto_zero_samples++;
}

// Closes the bucket filled since the previous call and works the zero point out afresh
void Qnhck2_16Shunt::close_zero_bucket() {
  if (auto_zero_samples == 0) {
    return;  // Nothing measured this second, so the zero point holds
  }
  auto_zero_bucket_sum_mV[auto_zero_next_bucket] = auto_zero_sum_mV;
  auto_zero_bucket_count[auto_zero_next_bucket] = auto_zero_samples;
  auto_zero_next_bucket = (auto_zero_next_bucket + 1) % AUTO_ZERO_BUCKETS;
  auto_zero_sum_mV = 0;
  auto_zero_samples = 0;

  uint32_t sum_mV = 0;
  uint32_t count = 0;
  for (uint8_t i = 0; i < AUTO_ZERO_BUCKETS; i++) {
    sum_mV += auto_zero_bucket_sum_mV[i];
    count += auto_zero_bucket_count[i];
  }
  const uint32_t zero_mV = (sum_mV + count / 2) / count;
  if (!qnhck_zero_plausible(zero_mV)) {
    // An unpowered sensor, the output on another pin, or current flowing after all
    if (!auto_zero_rejected) {
      auto_zero_rejected = true;
      LOG_SET_NEXT_SEVERITY(4);  // warning
      logging.printf(
          "QNHCK2-16: GPIO%d reads %u mV with the contactors open, too far from 1.65 V to be its zero point.\n",
          (int)pin, (unsigned)zero_mV);
    }
    return;
  }
  qnhck_zero_mV = (uint16_t)zero_mV;
  zero_known = true;
  // Taken with every second the zero point is worked out afresh, so as the contactors close it is
  // the temperature at the end of the measurement
  zero_temperature_known = qnhck_battery_temperature_dC(zero_temperature_dC);
  auto_zero_period_result = true;
}

/* The averaged output with the zero point's temperature drift since it was measured taken out of
   it, which is the same as moving the zero point by that drift. Without a temperature from the
   batteries, then or now, the zero point is used as measured. While the contactors are open it is
   measured every second, so there is nothing to take out. */
uint32_t Qnhck2_16Shunt::compensated_output_uV() const {
  int16_t now_dC;
  if (qnhck_zero_tempco_uV_per_C == 0 || !zero_temperature_known || !qnhck_battery_temperature_dC(now_dC)) {
    return output_uV;
  }
  const int64_t compensated_uV =
      (int64_t)output_uV - qnhck_zero_drift_uV(qnhck_zero_tempco_uV_per_C, zero_temperature_dC, now_dC);
  return compensated_uV > 0 ? (uint32_t)compensated_uV : 0;
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

#endif  // SMALL_FLASH_DEVICE
