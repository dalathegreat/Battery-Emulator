/*  Uses a simple CT clamp to measure current
 *  ESP32 ADCs are not very accurate
 *  To start in the linear region use a 150mV offset
 */
#ifdef CHADEMO_CT
#include "CHADEMO-CT.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "Arduino.h"
#include "CHADEMO-BATTERY.h"
#include "hal/adc_types.h"

// Set attenuation to 12dB (allows reading up to ~3.3V)
// Options: 950mV ADC_0db, 1250mV ADC_2_5db, 1750mV ADC_6db, 3100mV ADC_11db
static adc_attenuation_t adc_atten = ADC_0db;  // ADC attenuation setting

static float Amperes;       // Floating point with current in Amperes
static float Voltage = -1;  // Voltage not available

// CT parameters, need to be updated for the specific CT clamp used
// These settings are for a Tamura L03S100D15 CT clamp
// https://www.tamuracorp.com/global/products/download/pdf/06_L03SxxxD15_e.pdf
// At 20A the output is 0.8V, so 0Db attenuation can be used to get better resolution at low currents.  The voltage offset is set to 0.15V to start in the linear region of the CT clamp.
const float CT_V_offset = 0.15;    // in Volts, the voltage corresponding to 0A (the offset of the CT clamp)
const float CT_A_nominal = 100.0;  // in Amperes
const float CT_V_nominal = 4.0;    // in Volts, the voltage corresponding to the nominal current

// use a dedicated variable for the pin so that this file does not rely
// on a class member name that only exists inside ChademoBattery.  The
// value is fetched from the HAL so it will match whatever board is in
// use.
static gpio_num_t ct_pin = esp32hal->CHADEMO_CT_PIN();

uint16_t get_measured_voltage() {
  return (uint16_t)Voltage;
}

uint16_t get_measured_current() {
  // sample the CT pin multiple times and average to reduce noise
  float pin_V = 0;
  for (int i = 0; i < 10; i++) {
    pin_V += (float)analogReadMilliVolts(ct_pin);
  }
  pin_V = (pin_V / 10.0) * 1000.0;
  Amperes = (pin_V - CT_V_offset) * (CT_A_nominal / CT_V_nominal);
  return (uint16_t)Amperes;
}

void setup_ct(void) {
  // reserve the pin with the HAL so we detect conflicts early
  if (!esp32hal->alloc_pins("CHAdeMO CT", ct_pin)) {
    return;
  }

  // configure as adc
  // Set resolution to 12 bits (0-4095) - Default
  analogReadResolution(ADC_BITWIDTH_DEFAULT);
  analogSetPinAttenuation(ct_pin, adc_atten);
}

#endif  // CHADEMO_CT
