#include "../include.h"
#ifdef MEB_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "MEB-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send


#define MAX_CELL_VOLTAGE 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950  //Battery is put into emergency stop if one cell goes below this value

#define PID_SOC 0x028C
#define PID_VOLTAGE 0x1E3B
#define PID_CURRENT 0x1E3D
#define PID_MAX_TEMP 0x1E0E
#define PID_MIN_TEMP 0x1E0F
#define PID_MAX_CHARGE_VOLTAGE 0x5171
#define PID_MIN_DISCHARGE_VOLTAGE 0x5170
#define PID_ALLOWED_CHARGE_POWER 0x1E1B
#define PID_ALLOWED_DISCHARGE_POWER 0x1E1C
static uint32_t poll_pid = 0;

CAN_frame MEB_POLLING_FRAME = {.FD = true,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x1C40007B, // SOC 02 8C
                             .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_ACK_FRAME = {.FD = true,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x1C40007B, // Ack
                             .data = {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;


#ifdef DEBUG_VIA_USB
#endif
}

void receive_can_battery(CAN_frame rx_frame) {
datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x1C42007B: // Reply from battery
      if(rx_frame.data.u8[0] == 0x10){ //PID header
        transmit_can(&MEB_ACK_FRAME, can_config.battery);
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  //Send 500ms message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    switch (poll_pid) {
      case PID_SOC:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_SOC >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_SOC;        // Low byte
      poll_pid = PID_VOLTAGE;
        break;
      case PID_VOLTAGE:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_VOLTAGE >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_VOLTAGE;        // Low byte
      poll_pid = PID_CURRENT;
        break;
      case PID_CURRENT:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CURRENT >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CURRENT;        // Low byte
      poll_pid = PID_MAX_TEMP;
        break;
      case PID_MAX_TEMP:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_TEMP >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_TEMP;        // Low byte
      poll_pid = PID_MIN_TEMP;
        break;
      case PID_MIN_TEMP:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_TEMP >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_TEMP;        // Low byte
      poll_pid = PID_MAX_CHARGE_VOLTAGE;
        break;
      case PID_MAX_CHARGE_VOLTAGE:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_CHARGE_VOLTAGE >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_CHARGE_VOLTAGE;        // Low byte
      poll_pid = PID_MIN_DISCHARGE_VOLTAGE;
        break;
      case PID_MIN_DISCHARGE_VOLTAGE:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_DISCHARGE_VOLTAGE >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_DISCHARGE_VOLTAGE;        // Low byte
      poll_pid = PID_ALLOWED_CHARGE_POWER;
        break;
      case PID_ALLOWED_CHARGE_POWER:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_CHARGE_POWER >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_CHARGE_POWER;        // Low byte
      poll_pid = PID_ALLOWED_DISCHARGE_POWER;
        break;
      case PID_ALLOWED_DISCHARGE_POWER:
      MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_DISCHARGE_POWER >> 8); // High byte
      MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_DISCHARGE_POWER;        // Low byte
      poll_pid = PID_SOC;
        break;
      default:
        poll_pid = PID_SOC;
        break;
    }

    transmit_can(&MEB_POLLING_FRAME, can_config.battery);
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis10 = currentMillis;

    //transmit_can(&MEB_POLLING_FRAME, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Volkswagen Group MEB platform battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;  // 404.0V TODO
  datalayer.battery.info.min_design_voltage_dV = 3100;  // 310.0V TODO
}

#endif
