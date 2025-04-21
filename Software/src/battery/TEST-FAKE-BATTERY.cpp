#include "../include.h"
#ifdef TEST_FAKE_BATTERY
#include "../communication/can/comm_can.h"
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

static void print_units(char* header, int value, char* units) {
  logging.print(header);
  logging.print(value);
  logging.print(units);
}

void TestFakeBattery::
    update_values() { /* This function puts fake values onto the parameters sent towards the inverter */

  m_target->status.real_soc = 5000;  // 50.00%

  m_target->status.soh_pptt = 9900;  // 99.00%

  //m_target->status.voltage_dV = 3700;  // 370.0V , value set in startup in .ino file, editable via webUI

  m_target->status.current_dA = 0;  // 0 A

  m_target->info.total_capacity_Wh = 30000;  // 30kWh

  m_target->status.remaining_capacity_Wh = 15000;  // 15kWh

  m_target->status.cell_max_voltage_mV = 3596;

  m_target->status.cell_min_voltage_mV = 3500;

  m_target->status.temperature_min_dC = 50;  // 5.0*C

  m_target->status.temperature_max_dC = 60;  // 6.0*C

  m_target->status.max_discharge_power_W = 5000;  // 5kW

  m_target->status.max_charge_power_W = 5000;  // 5kW

  for (int i = 0; i < 97; ++i) {
    m_target->status.cell_voltages_mV[i] = 3700 + random(-20, 21);
  }

  //Fake that we get CAN messages
  m_target->status.CAN_battery_still_alive = CAN_STILL_ALIVE;

  /*Finally print out values to serial if configured to do so*/
  // TODO: Should distinguish battery 1 and 2
#ifdef DEBUG_LOG
  logging.println("FAKE Values going to inverter");
  print_units("SOH%: ", (m_target->status.soh_pptt * 0.01), "% ");
  print_units(", SOC%: ", (m_target->status.reported_soc * 0.01), "% ");
  print_units(", Voltage: ", (m_target->status.voltage_dV * 0.1), "V ");
  print_units(", Max discharge power: ", m_target->status.max_discharge_power_W, "W ");
  print_units(", Max charge power: ", m_target->status.max_charge_power_W, "W ");
  print_units(", Max temp: ", (m_target->status.temperature_max_dC * 0.1), "°C ");
  print_units(", Min temp: ", (m_target->status.temperature_min_dC * 0.1), "°C ");
  print_units(", Max cell voltage: ", m_target->status.cell_max_voltage_mV, "mV ");
  print_units(", Min cell voltage: ", m_target->status.cell_min_voltage_mV, "mV ");
  logging.println("");
#endif
}

void TestFakeBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  m_target->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
}

void TestFakeBattery::transmit_can() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    // Put fake messages here incase you want to test sending CAN
    //transmit_can_frame(&TEST, m_can_interface);
  }
}

void TestFakeBattery::setup(void) {  // Performs one time setup at startup
  randomSeed(analogRead(0));

  m_target->info.max_design_voltage_dV =
      4040;  // 404.4V, over this, charging is not possible (goes into forced discharge)
  m_target->info.min_design_voltage_dV = 2450;  // 245.0V under this, discharging further is disabled
  m_target->info.number_of_cells = 96;
  allow_contactor_closing();
}

#endif
