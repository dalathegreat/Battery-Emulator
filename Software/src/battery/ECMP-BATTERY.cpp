#include "../include.h"
#ifdef ECMP_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "ECMP-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

//Actual content messages
CAN_frame ECMP_XXX = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x301,
                      .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static uint16_t battery_voltage = 37000;
static uint16_t battery_soc = 0;

void update_values_battery() {

  datalayer.battery.status.real_soc = battery_soc * 100;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = (battery_voltage / 10);

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
    case 0x125:
      break;
    case 0x127:
      break;
    case 0x129:
      break;
    case 0x31B:
      break;
    case 0x358:
      break;
    case 0x359:
      break;
    case 0x361:
      battery_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case 0x362:
      break;
    case 0x454:
      break;
    case 0x494:
      break;
    case 0x594:
      break;
    case 0x6D0:
      battery_soc = (100 - rx_frame.data.u8[0]);
      break;
    case 0x6D1:
      break;
    case 0x6D2:
      break;
    case 0x6D3:
      break;
    case 0x6D4:
      break;
    case 0x6E0:
      break;
    case 0x6E1:
      break;
    case 0x6E2:
      break;
    case 0x6E3:
      break;
    case 0x6E4:
      break;
    case 0x6E5:
      break;
    case 0x6E6:
      break;
    case 0x6E7:
      break;
    case 0x6E8:
      break;
    case 0x6E9:
      break;
    case 0x6EB:
      break;
    case 0x6ED:
      break;
    case 0x6EE:
      break;
    case 0x6EF:
      break;
    case 0x6F0:
      break;
    case 0x6F1:
      break;
    case 0x6F2:
      break;
    case 0x6F3:
      break;
    case 0x6F4:
      break;
    case 0x6F5:
      break;
    case 0x6F6:
      break;
    case 0x6F7:
      break;
    case 0x6F8:
      break;
    case 0x6F9:
      break;
    case 0x6FA:
      break;
    case 0x6FB:
      break;
    case 0x6FC:
      break;
    case 0x6FD:
      break;
    case 0x6FE:
      break;
    case 0x6FF:
      break;
    case 0x794:
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
