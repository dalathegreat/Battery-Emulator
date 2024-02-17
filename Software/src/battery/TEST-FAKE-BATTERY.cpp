#include "BATTERIES.h"
#ifdef TEST_FAKE_BATTERY
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "TEST-FAKE-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send
static const int interval10 = 10;            // interval (ms) at which send CAN Messages
static const int interval100 = 100;          // interval (ms) at which send CAN Messages
static const int interval10s = 10000;        // interval (ms) at which send CAN Messages

void print_units(char* header, int value, char* units) {
  Serial.print(header);
  Serial.print(value);
  Serial.print(units);
}

void update_values_battery() { /* This function puts fake values onto the parameters sent towards the inverter */
  SOC = 5000;                  // 50.00%

  StateOfHealth = 9900;  // 99.00%

  //battery_voltage = 3700;  // 370.0V , value set in startup in .ino file, editable via webUI

  battery_current = 0;  // 0 A

  capacity_Wh = 30000;  // 30kWh

  remaining_capacity_Wh = 15000;  // 15kWh

  cell_max_voltage = 3596;

  cell_min_voltage = 3500;

  stat_batt_power = 0;  // 0W

  temperature_min = 50;  // 5.0*C

  temperature_max = 60;  // 6.0*C

  max_target_discharge_power = 5000;  // 5kW

  max_target_charge_power = 5000;  // 5kW

  for (int i = 0; i < 97; ++i) {
    cellvoltages[i] = 3500 + i;
  }

/*Finally print out values to serial if configured to do so*/
#ifdef DEBUG_VIA_USB
  Serial.println("FAKE Values going to inverter");
  print_units("SOH%: ", (StateOfHealth * 0.01), "% ");
  print_units(", SOC%: ", (SOC * 0.01), "% ");
  print_units(", Voltage: ", (battery_voltage * 0.1), "V ");
  print_units(", Max discharge power: ", max_target_discharge_power, "W ");
  print_units(", Max charge power: ", max_target_charge_power, "W ");
  print_units(", Max temp: ", (temperature_max * 0.1), "°C ");
  print_units(", Min temp: ", (temperature_min * 0.1), "°C ");
  print_units(", Max cell voltage: ", cell_max_voltage, "mV ");
  print_units(", Min cell voltage: ", cell_min_voltage, "mV ");
  Serial.println("");
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0xABC:
      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;
    // Put fake messages here incase you want to test sending CAN
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  Serial.println("Test mode with fake battery selected");

  max_voltage = 4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  min_voltage = 2450;  // 245.0V under this, discharging further is disabled
}

#endif
