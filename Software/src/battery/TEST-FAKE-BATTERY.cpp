#include "../include.h"
#ifdef TEST_FAKE_BATTERY
#include "../datalayer/datalayer.h"
#include "TEST-FAKE-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send

CAN_frame TEST = {.FD = false,
                  .ext_ID = false,
                  .DLC = 8,
                  .ID = 0x123,
                  .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};

void print_units(char* header, int value, char* units) {
  logging.print(header);
  logging.print(value);
  logging.print(units);
}

void update_values_battery() { /* This function puts fake values onto the parameters sent towards the inverter */

  datalayer.battery.status.real_soc = 5000;  // 50.00%

  datalayer.battery.status.soh_pptt = 9900;  // 99.00%

  //datalayer.battery.status.voltage_dV = 3700;  // 370.0V , value set in startup in .ino file, editable via webUI

  datalayer.battery.status.current_dA = 0;  // 0 A

  datalayer.battery.info.total_capacity_Wh = 30000;  // 30kWh

  datalayer.battery.status.remaining_capacity_Wh = 15000;  // 15kWh

  datalayer.battery.status.cell_max_voltage_mV = 3596;

  datalayer.battery.status.cell_min_voltage_mV = 3500;

  datalayer.battery.status.temperature_min_dC = 50;  // 5.0*C

  datalayer.battery.status.temperature_max_dC = 60;  // 6.0*C

  datalayer.battery.status.max_discharge_power_W = 5000;  // 5kW

  datalayer.battery.status.max_charge_power_W = 5000;  // 5kW

  for (int i = 0; i < 97; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = 3700 + random(-20, 21);
  }

  //Fake that we get CAN messages
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

/*Finally print out values to serial if configured to do so*/
#ifdef DEBUG_LOG
  logging.println("FAKE Values going to inverter");
  print_units("SOH%: ", (datalayer.battery.status.soh_pptt * 0.01), "% ");
  print_units(", SOC%: ", (datalayer.battery.status.reported_soc * 0.01), "% ");
  print_units(", Voltage: ", (datalayer.battery.status.voltage_dV * 0.1), "V ");
  print_units(", Max discharge power: ", datalayer.battery.status.max_discharge_power_W, "W ");
  print_units(", Max charge power: ", datalayer.battery.status.max_charge_power_W, "W ");
  print_units(", Max temp: ", (datalayer.battery.status.temperature_max_dC * 0.1), "°C ");
  print_units(", Min temp: ", (datalayer.battery.status.temperature_min_dC * 0.1), "°C ");
  print_units(", Max cell voltage: ", datalayer.battery.status.cell_max_voltage_mV, "mV ");
  print_units(", Min cell voltage: ", datalayer.battery.status.cell_min_voltage_mV, "mV ");
  logging.println("");
#endif
}

#ifdef DOUBLE_BATTERY

void update_values_battery2() {  // Handle the values coming in from battery #2

  datalayer.battery2.info.number_of_cells = 96;

  datalayer.battery2.status.real_soc = 5000;  // 50.00%

  datalayer.battery2.status.soh_pptt = 9900;  // 99.00%

  //datalayer.battery.status.voltage_dV = 3700;  // 370.0V , value set in startup in .ino file, editable via webUI

  datalayer.battery2.status.current_dA = 0;  // 0 A

  datalayer.battery2.info.total_capacity_Wh = 30000;  // 30kWh

  datalayer.battery2.status.remaining_capacity_Wh = 15000;  // 15kWh

  datalayer.battery2.status.cell_max_voltage_mV = 3596;

  datalayer.battery2.status.cell_min_voltage_mV = 3500;

  datalayer.battery2.status.temperature_min_dC = 50;  // 5.0*C

  datalayer.battery2.status.temperature_max_dC = 60;  // 6.0*C

  datalayer.battery2.status.max_discharge_power_W = 5000;  // 5kW

  datalayer.battery2.status.max_charge_power_W = 5000;  // 5kW

  for (int i = 0; i < 97; ++i) {
    datalayer.battery2.status.cell_voltages_mV[i] = 3700 + random(-20, 21);
  }

  //Fake that we get CAN messages
  datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

/*Finally print out values to serial if configured to do so*/
#ifdef DEBUG_LOG
  logging.println("FAKE Values  battery 2 going to inverter");
  print_units("SOH 2 %: ", (datalayer.battery2.status.soh_pptt * 0.01), "% ");
  print_units(", SOC 2 %: ", (datalayer.battery2.status.reported_soc * 0.01), "% ");
  print_units(", Voltage 2: ", (datalayer.battery2.status.voltage_dV * 0.1), "V ");
  print_units(", Max discharge power 2: ", datalayer.battery2.status.max_discharge_power_W, "W ");
  print_units(", Max charge power 2: ", datalayer.battery2.status.max_charge_power_W, "W ");
  print_units(", Max temp 2: ", (datalayer.battery2.status.temperature_max_dC * 0.1), "°C ");
  print_units(", Min temp 2: ", (datalayer.battery2.status.temperature_min_dC * 0.1), "°C ");
  print_units(", Max cell voltage 2: ", datalayer.battery2.status.cell_max_voltage_mV, "mV ");
  print_units(", Min cell voltage 2: ", datalayer.battery2.status.cell_min_voltage_mV, "mV ");
  logging.println("");
#endif
}

void handle_incoming_can_frame_battery2(CAN_frame rx_frame) {
  datalayer.battery2.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}
#endif  // DOUBLE_BATTERY

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}
void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    // Put fake messages here incase you want to test sending CAN
    //transmit_can_frame(&TEST, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  randomSeed(analogRead(0));

  strncpy(datalayer.system.info.battery_protocol, "Fake battery for testing purposes", 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer.battery.info.max_design_voltage_dV =
      4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 2450;  // 245.0V under this, discharging further is disabled
  datalayer.battery.info.number_of_cells = 96;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

#endif
