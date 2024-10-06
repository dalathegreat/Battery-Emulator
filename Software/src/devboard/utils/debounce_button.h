#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <Arduino.h>

// Enum to define switch type (Normally Closed - NC, Normally Open - NO)
enum SwitchType {
  NC,  // Normally Closed
  NO   // Normally Open
};

// Enum to define button state
enum ButtonState {
  NONE,     // No change in button state
  PRESSED,  // Button is pressed down
  RELEASED  // Button is released
};

// Struct to hold button state and debounce parameters
struct DebouncedButton {
  int pin;                         // GPIO pin number
  unsigned long lastDebounceTime;  // Time of last state change
  unsigned long debounceDelay;     // Debounce delay time
  unsigned long ulPressTime;       // Time when the button was last pressed
  bool lastButtonState;            // Previous button state
  bool buttonState;                // Current button state
  SwitchType switchType;           // Switch type (NC or NO)
};

// Function to initialize the debounced button
void initDebouncedButton(DebouncedButton& button, int pin, SwitchType type, unsigned long debounceDelay = 50);

// Function to debounce button and return the button state (PRESSED, RELEASED, or NONE)
ButtonState debounceButton(DebouncedButton& button, unsigned long& timeSincePress);

#endif  // DEBOUNCE_H
