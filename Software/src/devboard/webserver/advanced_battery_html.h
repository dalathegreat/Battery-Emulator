#ifndef ADVANCEDBATTERY_H
#define ADVANCEDBATTERY_H

#include <Arduino.h>
#include <functional>
#include <vector>

class AsyncWebServerRequest;
void send_advanced_battery_page(AsyncWebServerRequest* request);

class Battery;

// Each BatteryCommand defines a command that can be performed by a battery.
// Whether the selected battery supports the command is determined at run-time
// by calling the condition callback.
struct BatteryCommand {
  // The unique name of the route in the API to execute the command or a function in Javascript
  const char* identifier;

  // Display name for the command. Can be used in the UI.
  const char* title;

  // Are you sure? prompt text. If null, no confirmation is asked.
  const char* prompt;

  // Function to determine whether the given battery supports this command.
  std::function<bool(Battery*)> condition;

  // Function that executes the command for the given battery.
  std::function<void(Battery*)> action;

  // Reload page after running (for read commands).
  bool reload_after = false;
};

extern std::vector<BatteryCommand> battery_commands;

#endif
