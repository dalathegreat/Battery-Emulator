#include "../include.h"
#ifdef RJXZS_BMS
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "PYLON-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

//Actual content messages
CAN_frame RJXZS_F4 = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0xF4, .data = {0x1C, 0x00, 0x02}};

static int16_t celltemperature_max_dC = 0;
static int16_t celltemperature_min_dC = 0;
static int16_t current_dA = 0;
static uint16_t voltage_dV = 0;
static uint16_t cellvoltage_max_mV = 0;
static uint16_t cellvoltage_min_mV = 0;
static uint16_t charge_cutoff_voltage = 0;
static uint16_t discharge_cutoff_voltage = 0;
static int16_t max_charge_current = 0;
static int16_t max_discharge_current = 0;
static uint8_t voltage_level = 0;
static uint8_t ah_number = 0;
static uint8_t SOC = 0;
static uint8_t SOH = 0;

void update_values_battery() {

  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.info.max_design_voltage_dV;

  datalayer.battery.info.min_design_voltage_dV;
}

void receive_can_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x1:
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {

    previousMillis1000 = currentMillis;

    transmit_can(&RJXZS_F4, can_config.battery);  // Start broadcast
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("RJXZS BMS selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4500;
  datalayer.battery.info.min_design_voltage_dV = 2500;
}

#endif  // RJXZS_BMS
