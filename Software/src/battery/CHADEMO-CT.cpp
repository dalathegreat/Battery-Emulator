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
#include "Shunt.h"

// Ensure valid values at run-time
// User can update all these values via Settings page
uint16_t ct_clamp_nominal_voltage_dV = 40;
uint16_t ct_clamp_nominal_current_A = 100;
bool ct_invert_current = false;

static float Amperes;          // Floating point with current in Amperes
static float Voltage = -1.0f;  // Voltage not available

// CT parameters, need to be updated for the specific CT clamp used
// These settings are for a Tamura L03S100D15 CT clamp
// https://www.tamuracorp.com/global/products/download/pdf/06_L03SxxxD15_e.pdf
// At 20A the output is 0.8V, use an offset voltage of 1V to read positive and negative currents in the linear region of the ADC.  The nominal voltage is 0.8V at the nominal current of 100A, so use that for scaling.  Adjust the nominal current if using a different CT clamp.
// To accurately measure voltage the offset voltage must also be read and subtracted from the reading, so the offset pin must be connected to the same voltage as the CT output at 0A (ie. 1V in this example).  This can be done with a simple voltage divider from the 5V supply if the clamp does not have a built-in offset output.

// use a dedicated variable for the pin so that this file does not rely
// on a class member name that only exists inside ChademoBattery.  The
// value is fetched from the HAL so it will match whatever board is in
// use.
static gpio_num_t ct_pin;
static gpio_num_t ct_0V_pin;
static bool ct_pin_initialized = false;

// Included for future use
float get_measured_voltage_ct() {
  return Voltage;
}

// +ve is charging, -ve is discharging, use invert setting to flip if needed
float get_measured_current_ct() {
  if (!ct_pin_initialized) {
    return 0;
  }

  float CT_V_nominal = (float)ct_clamp_nominal_voltage_dV / 10.0f;  // Convert from dV to Volts
  float CT_A_nominal = (float)ct_clamp_nominal_current_A;           // in Amperes

  // sample the CT pin multiple times and average to reduce noise
  float pin_V = 0.0f;
  float pin_0V = 0.0f;
  float pin_diff_V = 0.0f;
  int samples = 50;
  for (int i = 0; i < samples; i++) {
    pin_V = (float)analogReadMilliVolts(ct_pin);
    pin_0V = (float)analogReadMilliVolts(ct_0V_pin);
    pin_diff_V += pin_V - pin_0V;
  }
  pin_diff_V = (pin_diff_V / (float)samples) / 1000.0f;
  Amperes = (pin_diff_V / (float)samples) * (CT_A_nominal / CT_V_nominal);
  if (ct_invert_current) {
    Amperes = -Amperes;
  }
  return Amperes;
}

void setup_ct(void) {
  // Initialize ct_pin from HAL if not already done
  if (!ct_pin_initialized) {
    ct_pin = esp32hal->CHADEMO_CT_PIN();
    ct_0V_pin = esp32hal->CHADEMO_CT_0V_PIN();
    ct_pin_initialized = true;
  }

  // reserve the pin with the HAL so we detect conflicts early
  if (!esp32hal->alloc_pins("CHAdeMO CT", ct_pin, ct_0V_pin)) {
    return;
  }

  // Configure as adc
  // Set resolution to 12 bits (0-4095) - Default
  pinMode(ct_pin, INPUT);
  pinMode(ct_0V_pin, INPUT);
  analogRead(ct_pin);  // Avoids error if attenuation is set before first read
  analogRead(ct_0V_pin);

  char shunt_protocol[32];
  snprintf(shunt_protocol, sizeof(shunt_protocol), "%dA CT Clamp", (int)ct_clamp_nominal_current_A);
  strncpy(datalayer.system.info.shunt_protocol, shunt_protocol, 31);
  datalayer.system.info.shunt_protocol[31] = '\0';
}
