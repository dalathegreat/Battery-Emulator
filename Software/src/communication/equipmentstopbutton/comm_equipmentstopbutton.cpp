#include "comm_equipmentstopbutton.h"
#include "../../include.h"

// Parameters
#ifdef EQUIPMENT_STOP_BUTTON
const unsigned long equipment_button_long_press_duration =
    15000;                                                     // 15 seconds for long press in case of MOMENTARY_SWITCH
const unsigned long equipment_button_debounce_duration = 200;  // 200ms for debouncing the button
unsigned long timeSincePress = 0;                              // Variable to store the time since the last press
DebouncedButton equipment_stop_button;                         // Debounced button object
#endif                                                         // EQUIPMENT_STOP_BUTTON

// Initialization functions
#ifdef EQUIPMENT_STOP_BUTTON
void init_equipment_stop_button() {
  //using external pullup resistors NC
  pinMode(EQUIPMENT_STOP_PIN, INPUT);
  // Initialize the debounced button with NC switch type and equipment_button_debounce_duration debounce time
  initDebouncedButton(equipment_stop_button, EQUIPMENT_STOP_PIN, NC, equipment_button_debounce_duration);
}
#endif  // EQUIPMENT_STOP_BUTTON

// Main functions

#ifdef EQUIPMENT_STOP_BUTTON
void monitor_equipment_stop_button() {

  ButtonState changed_state = debounceButton(equipment_stop_button, timeSincePress);

  if (equipment_stop_behavior == LATCHING_SWITCH) {
    if (changed_state == PRESSED) {
      // Changed to ON – initiating equipment stop.
      setBatteryPause(true, false, true);
    } else if (changed_state == RELEASED) {
      // Changed to OFF – ending equipment stop.
      setBatteryPause(false, false, false);
    }
  } else if (equipment_stop_behavior == MOMENTARY_SWITCH) {
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
#endif  // EQUIPMENT_STOP_BUTTON
