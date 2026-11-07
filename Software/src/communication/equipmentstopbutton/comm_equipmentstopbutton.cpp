#include "comm_equipmentstopbutton.h"
#include "../../datalayer/datalayer.h"
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
#ifdef HW_LILYGO
// The stop pin is selectable on this board, so a stored choice can clash with the MCP2515 INT or
// the 2nd battery contactors. A stop input that cannot be read keeps the battery paused instead.
static bool equipment_stop_pin_failed = false;
#endif  // HW_LILYGO

// Initialization functions
bool init_equipment_stop_button() {
  if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::NOT_CONNECTED) {
    return true;
  }

  auto pin = esp32hal->EQUIPMENT_STOP_PIN();
  if (!esp32hal->alloc_pins("Equipment stop button", pin)) {
#ifdef HW_LILYGO
    equipment_stop_pin_failed = true;
    // Not stored to flash, so fixing the setting and restarting clears it
    setBatteryPause(true, false, EquipmentStop::STOP, false);
#endif  // HW_LILYGO
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

#ifdef HW_LILYGO
  if (equipment_stop_pin_failed) {
    // Re-assert if resumed from the webserver or MQTT: the input still cannot be read
    if (!datalayer.system.info.equipment_stop_active) {
      setBatteryPause(true, false, EquipmentStop::STOP, false);
    }
    return;
  }
#endif  // HW_LILYGO

  ButtonState changed_state = debounceButton(equipment_stop_button, timeSincePress);

  if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::LATCHING_SWITCH) {
    if (changed_state == PRESSED) {
      // Changed to ON – initiating equipment stop.
      setBatteryPause(true, false, EquipmentStop::STOP);
    } else if (changed_state == RELEASED) {
      // Changed to OFF – ending equipment stop.
      setBatteryPause(false, false, EquipmentStop::RESUME);
    }
  } else if (equipment_stop_behavior == STOP_BUTTON_BEHAVIOR::MOMENTARY_SWITCH) {
    if (changed_state == RELEASED) {  // button is released

      if (timeSincePress < equipment_button_long_press_duration) {
        // Short press detected, trigger equipment stop
        setBatteryPause(true, false, EquipmentStop::STOP);
      } else {
        // Long press detected, reset equipment stop state
        setBatteryPause(false, false, EquipmentStop::RESUME);
      }
    }
  }
}
