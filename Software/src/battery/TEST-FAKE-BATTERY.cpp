#include "TEST-FAKE-BATTERY.h"
#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../devboard/utils/logging.h"

void TestFakeBattery::
    update_values() { /* This function puts fake values onto the parameters sent towards the inverter */

  datalayer_battery->status.real_soc = 5000;  // 50.00%

  datalayer_battery->status.soh_pptt = 9900;  // 99.00%

  //datalayer_battery->status.voltage_dV = 3700;  // 370.0V , value set in startup in .ino file, editable via webUI

  datalayer_battery->status.current_dA = 0;  // 0 A

  datalayer_battery->info.total_capacity_Wh = 30000;  // 30kWh

  datalayer_battery->status.remaining_capacity_Wh = 15000;  // 15kWh

  datalayer_battery->status.cell_max_voltage_mV = 3596;

  datalayer_battery->status.cell_min_voltage_mV = 3500;

  datalayer_battery->status.temperature_min_dC = 50;  // 5.0*C

  datalayer_battery->status.temperature_max_dC = 60;  // 6.0*C

  datalayer_battery->status.max_discharge_power_W = 5000;  // 5kW

  datalayer_battery->status.max_charge_power_W = 5000;  // 5kW

  for (int i = 0; i < 97; ++i) {
    datalayer_battery->status.cell_voltages_mV[i] = 3700 + random(-20, 21);
  }

  //Fake that we get CAN messages
  datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void TestFakeBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void TestFakeBattery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    // Put fake messages here incase you want to test sending CAN
    //transmit_can_frame(&TEST);
  }
}

void TestFakeBattery::setup(void) {  // Performs one time setup at startup
  randomSeed(analogRead(0));

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer_battery->info.max_design_voltage_dV =
      4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  datalayer_battery->info.min_design_voltage_dV = 2450;  // 245.0V under this, discharging further is disabled
  datalayer_battery->info.number_of_cells = 96;

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
