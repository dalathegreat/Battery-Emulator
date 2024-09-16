#include "../include.h"
#ifdef ECMP_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "ECMP-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

//Actual content messages
CAN_frame PSA_XXX = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x301,
                     .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

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
    case 0x123:
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
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("PSA battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;  // 404.0V, charging over this is not possible
  datalayer.battery.info.min_design_voltage_dV = 3100;  // 310.0V, under this, discharging further is disabled
}

#endif
