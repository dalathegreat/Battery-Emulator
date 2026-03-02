/*  Uses a simple CT clamp to measure current
 *  ESP32 ADCs are not very accurate outside of linear region
 *  To start in the linear region use a 150mV offset
 *  Use a 100 nF capacitor on the input pin to reduce noise (optional but recommended)
 */

#include "CHADEMO-CT.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "Arduino.h"
#include "CHADEMO-BATTERY.h"
#include "hal/adc_types.h"
#include "shunt.h"

// Ensure valid values at run-time
// User can update all these values via Settings page
uint16_t ct_clamp_offset_mV = 150;
uint16_t ct_clamp_nominal_voltage_dV = 40;
uint16_t ct_clamp_nominal_current_A = 100;

// Set attenuation to 12dB (allows reading up to ~3.3V)
// Options: 950mV ADC_0db, 1250mV ADC_2_5db, 1750mV ADC_6db, 3100mV ADC_11db
static adc_attenuation_t adc_atten = ADC_0db;  // ADC attenuation setting

static float Amperes;       // Floating point with current in Amperes
static float Voltage = -1;  // Voltage not available

// CT parameters, need to be updated for the specific CT clamp used
// These settings are for a Tamura L03S100D15 CT clamp
// https://www.tamuracorp.com/global/products/download/pdf/06_L03SxxxD15_e.pdf
// At 20A the output is 0.8V, so 0Db attenuation can be used to get better resolution at low currents.  The voltage offset is set to 0.15V to start in the linear region of the CT clamp.
float CT_V_offset = (float)ct_clamp_offset_mV / 1000.0f;          // Convert from mV to Volts
float CT_V_nominal = (float)ct_clamp_nominal_voltage_dV / 10.0f;  // Convert from dV to Volts
float CT_A_nominal = (float)ct_clamp_nominal_current_A;           // in Amperes

// use a dedicated variable for the pin so that this file does not rely
// on a class member name that only exists inside ChademoBattery.  The
// value is fetched from the HAL so it will match whatever board is in
// use.
static gpio_num_t ct_pin;
static bool ct_pin_initialized = false;

// Included for future use
uint16_t get_measured_voltage_ct() {
  return (uint16_t)Voltage;
}

uint16_t get_measured_current_ct() {
  if (!ct_pin_initialized) {
    return 0;
  }
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
  // Initialize ct_pin from HAL if not already done
  if (!ct_pin_initialized) {
    ct_pin = esp32hal->CHADEMO_CT_PIN();
    ct_pin_initialized = true;
  }

  // reserve the pin with the HAL so we detect conflicts early
  if (!esp32hal->alloc_pins("CHAdeMO CT", ct_pin)) {
    return;
  }

  // Configure as adc
  // Set resolution to 12 bits (0-4095) - Default
  pinMode(ct_pin, INPUT);
  analogRead(ct_pin);  // Avoids error if attenuation is set before first read
  analogReadResolution(ADC_BITWIDTH_DEFAULT);
  analogSetPinAttenuation(ct_pin, adc_atten);

  char shunt_protocol[32];
  snprintf(shunt_protocol, sizeof(shunt_protocol), "%dA CT Clamp", (int)CT_A_nominal);
  strncpy(datalayer.system.info.shunt_protocol, shunt_protocol, 31);
  datalayer.system.info.shunt_protocol[31] = '\0';
}
