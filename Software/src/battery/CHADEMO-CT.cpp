/*  Uses a simple CT clamp to measure current
 *  
 *
 */
#ifdef CHADEMO_CT
#include "CHADEMO-CT.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "CHADEMO-BATTERY.h"

static float Amperes;       // Floating point with current in Amperes
static float Voltage = -1;  // Voltage not available

// CT parameters, need to be updated for the specific CT clamp used
const float CT_V_offset = 0.0;     // in Volts, the voltage corresponding to 0A (the offset of the CT clamp)
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
  // make sure we always read the configured pin
  float pin_V = (float)digitalRead(ct_pin);
  Amperes = (pin_V - CT_V_offset) * (CT_A_nominal / CT_V_nominal);
  return (uint16_t)Amperes;
}

void setup_ct(void) {
  // reserve the pin with the HAL so we detect conflicts early
  if (!esp32hal->alloc_pins("CHAdeMO CT", ct_pin)) {
    return;
  }

  // configure as input
  pinMode(ct_pin, INPUT);
}

#endif  // CHADEMO_CT
