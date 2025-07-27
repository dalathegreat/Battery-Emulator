#ifndef _SMA_INVERTER_BASE_H
#define _SMA_INVERTER_BASE_H

#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "CanInverterProtocol.h"

class SmaInverterBase : public CanInverterProtocol {
 public:
  SmaInverterBase() { contactorEnablePin = esp32hal->INVERTER_CONTACTOR_ENABLE_PIN(); }
  bool allows_contactor_closing() override { return digitalRead(contactorEnablePin) == 1; }

  virtual bool setup() override {
    datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first

    if (!esp32hal->alloc_pins("SMA inverter", contactorEnablePin)) {
      return false;
    }
    pinMode(contactorEnablePin, INPUT);

    contactorLedPin = esp32hal->INVERTER_CONTACTOR_ENABLE_LED_PIN();
    if (contactorLedPin != GPIO_NUM_NC) {
      if (!esp32hal->alloc_pins("SMA inverter", contactorLedPin)) {
        return false;
      }
      pinMode(contactorLedPin, OUTPUT);
      digitalWrite(contactorLedPin, LOW);  // Turn LED off, until inverter allows contactor closing
    }

    return true;
  }

 protected:
  void control_contactor_led() {
    if (contactorLedPin != GPIO_NUM_NC) {
      if (datalayer.system.status.inverter_allows_contactor_closing) {
        digitalWrite(contactorLedPin,
                     HIGH);  // Turn on LED to indicate that SMA inverter allows contactor closing
      } else {
        digitalWrite(contactorLedPin,
                     LOW);  // Turn off LED to indicate that SMA inverter does not allow contactor closing
      }
    }
  }

 private:
  gpio_num_t contactorEnablePin;
  gpio_num_t contactorLedPin;
};

#endif
