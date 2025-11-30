#include "comm_equipmentstopbutton.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/safety/safety.h"
#include "../../devboard/utils/debounce_button.h"

const STOP_BUTTON_BEHAVIOR stop_button_default_behavior = STOP_BUTTON_BEHAVIOR::NOT_CONNECTED;
STOP_BUTTON_BEHAVIOR equipment_stop_behavior = stop_button_default_behavior;

// Parameters
const unsigned long equipment_button_long_press_duration =
    15000;                                                     // 15 seconds for long press in case of MOMENTARY_SWITCH
const unsigned long equipment_button_debounce_duration = 200;  // 200ms for debouncing the button
unsigned long timeSincePress = 0;                              // Variable to store the time since the last press
DebouncedButton equipment_stop_button;                         // Debounced button object

// Initialization functions
bool init_equipment_stop_button() {
  if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::NOT_CONNECTED) {
    return true;
  }

  auto pin = esp32hal->EQUIPMENT_STOP_PIN();
  if (!esp32hal->alloc_pins("Equipment stop button", pin)) {
    return false;
  }

  //using external pullup resistors NC
  pinMode(pin, INPUT);
  // Initialize the debounced button with NC switch type and equipment_button_debounce_duration debounce time
  initDebouncedButton(equipment_stop_button, pin, NC, equipment_button_debounce_duration);

  return true;
}

// Main functions

void monitor_equipment_stop_button() {

  if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::NOT_CONNECTED) {
    return;
  }

  ButtonState changed_state = debounceButton(equipment_stop_button, timeSincePress);

  if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::LATCHING_SWITCH) {
    if (changed_state == PRESSED) {
      // Changed to ON – initiating equipment stop.
      setBatteryPause(true, false, true);
    } else if (changed_state == RELEASED) {
      // Changed to OFF – ending equipment stop.
      setBatteryPause(false, false, false);
    }
  } else if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::MOMENTARY_SWITCH) {
    if (changed_state == RELEASED) {  // button is released

      if (timeSincePress < equipment_button_long_press_duration) {
        // Short press detected, trigger equipment stop
        setBatteryPause(true, false, true);
      } else {
        // Long press detected, reset equipment stop state
        setBatteryPause(false, false, false);
      }
    }
  }
}
