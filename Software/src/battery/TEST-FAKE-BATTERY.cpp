#include "../include.h"
#ifdef TEST_FAKE_BATTERY
#include "../datalayer/datalayer.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "TEST-FAKE-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send

void print_units(char* header, int value, char* units) {
  Serial.print(header);
  Serial.print(value);
  Serial.print(units);
}

void update_values_battery() {  /* This function puts fake values onto the parameters sent towards the inverter */
  system_real_SOC_pptt = 5000;  // 50.00%

  datalayer.battery.status.soh_pptt = 9900;  // 99.00%

  //system_battery_voltage_dV = 3700;  // 370.0V , value set in startup in .ino file, editable via webUI

  system_battery_current_dA = 0;  // 0 A

  datalayer.battery.info.total_capacity_Wh = 30000;  // 30kWh

  datalayer.battery.status.remaining_capacity_Wh = 15000;  // 15kWh

  system_cell_max_voltage_mV = 3596;

  system_cell_min_voltage_mV = 3500;

  datalayer.battery.status.active_power_W = 0;  // 0W

  system_temperature_min_dC = 50;  // 5.0*C

  system_temperature_max_dC = 60;  // 6.0*C

  system_max_discharge_power_W = 5000;  // 5kW

  system_max_charge_power_W = 5000;  // 5kW

  for (int i = 0; i < 97; ++i) {
    system_cellvoltages_mV[i] = 3500 + i;
  }

/*Finally print out values to serial if configured to do so*/
#ifdef DEBUG_VIA_USB
  Serial.println("FAKE Values going to inverter");
  print_units("SOH%: ", (datalayer.battery.status.soh_pptt * 0.01), "% ");
  print_units(", SOC%: ", (system_scaled_SOC_pptt * 0.01), "% ");
  print_units(", Voltage: ", (system_battery_voltage_dV * 0.1), "V ");
  print_units(", Max discharge power: ", system_max_discharge_power_W, "W ");
  print_units(", Max charge power: ", system_max_charge_power_W, "W ");
  print_units(", Max temp: ", (system_temperature_max_dC * 0.1), "°C ");
  print_units(", Min temp: ", (system_temperature_min_dC * 0.1), "°C ");
  print_units(", Max cell voltage: ", system_cell_max_voltage_mV, "mV ");
  print_units(", Min cell voltage: ", system_cell_min_voltage_mV, "mV ");
  Serial.println("");
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {  // All CAN messages recieved will be logged via serial
  Serial.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  Serial.print("  ");
  Serial.print(rx_frame.MsgID, HEX);
  Serial.print("  ");
  Serial.print(rx_frame.FIR.B.DLC);
  Serial.print("  ");
  for (int i = 0; i < rx_frame.FIR.B.DLC; ++i) {
    Serial.print(rx_frame.data.u8[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
}
void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    // Put fake messages here incase you want to test sending CAN
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Test mode with fake battery selected");
#endif

  system_max_design_voltage_dV = 4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  system_min_design_voltage_dV = 2450;  // 245.0V under this, discharging further is disabled
}

#endif
