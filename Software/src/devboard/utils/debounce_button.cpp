#include "debounce_button.h"

// Function to initialize the debounced button with pin, switch type, and debounce delay
void initDebouncedButton(DebouncedButton& button, int pin, SwitchType type, unsigned long debounceDelay) {
  button.pin = pin;
  button.debounceDelay = debounceDelay;
  button.lastDebounceTime = 0;
  button.lastButtonState = (type == NC) ? HIGH : LOW;  // NC starts HIGH, NO starts LOW
  button.buttonState = button.lastButtonState;
  button.switchType = type;
  pinMode(pin, INPUT);  // Setup pin mode
}

ButtonState debounceButton(DebouncedButton& button, unsigned long& timeSincePress) {
  int reading = digitalRead(button.pin);

  // If the button state has changed due to noise or a press
  if (reading != button.lastButtonState) {
    // Reset debounce timer
    button.lastDebounceTime = millis();
  }

  // Check if the state change is stable for the debounce delay
  if ((millis() - button.lastDebounceTime) > button.debounceDelay) {
    if (reading != button.buttonState) {
      button.buttonState = reading;

      // Adjust button logic based on switch type (NC or NO)
      if (button.switchType == NC) {
        if (button.buttonState == LOW) {
          // Button pressed for NC, record the press time
          button.ulPressTime = millis();
          return PRESSED;
        } else {
          // Button released for NC, calculate the time since last press
          timeSincePress = millis() - button.ulPressTime;
          return RELEASED;
        }
      } else {  // NO type
        if (button.buttonState == HIGH) {
          // Button pressed for NO, record the press time
          button.ulPressTime = millis();
          return PRESSED;
        } else {
          // Button released for NO, calculate the time since last press
          timeSincePress = millis() - button.ulPressTime;
          return RELEASED;
        }
      }
    }
  }

  // Remember the last button state
  button.lastButtonState = reading;
  return NONE;  // No change in button state
}
