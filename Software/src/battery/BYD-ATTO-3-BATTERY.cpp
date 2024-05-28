#include "../include.h"
#ifdef BYD_ATTO_3_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "BYD-ATTO-3-BATTERY.h"

/* TODO: 
- Get contactor closing working
- Figure out which CAN messages need to be sent towards the battery to keep it alive
- Map all values from battery CAN messages
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static uint8_t counter_50ms = 0;
static uint8_t counter_100ms = 0;
static uint8_t frame6_counter = 0xB;
static uint8_t frame7_counter = 0x5;

static int BMS_SOC = 0;

CAN_frame_t ATTO_3_12D = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_std,
                                      }},
                          .MsgID = 0x12D,
                          .data = {0xA0, 0x28, 0x02, 0xA0, 0x0C, 0x71, 0xCF, 0x49}};
CAN_frame_t ATTO_3_411 = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_std,
                                      }},
                          .MsgID = 0x411,
                          .data = {0x98, 0x3A, 0x88, 0x13, 0x9D, 0x00, 0xFF, 0x8C}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.active_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.MsgID) {
    case 0xABC:  // TODO: Add all switchcases for messages coming from battery
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // Let system know battery is sending CAN
      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();
  //Send 50ms message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis50 >= INTERVAL_50_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis50));
    }
    previousMillis50 = currentMillis;

    counter_50ms++;
    if (counter_50ms > 23) {
      ATTO_3_12D.data.u8[2] = 0x00;  // Goes from 02->00
      ATTO_3_12D.data.u8[3] = 0x22;  // Goes from A0->22
      ATTO_3_12D.data.u8[5] = 0x31;  // Goes from 71->31
      // TODO: handle more variations after more seconds have passed if needed
    }

    // Update the counters in frame 6 & 7 (they are not in sync)
    if (frame6_counter == 0x0) {
      frame6_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame6_counter--;  // Decrement the counter
    }
    if (frame7_counter == 0x0) {
      frame7_counter = 0xF;  // Reset to 0xF after reaching 0x0
    } else {
      frame7_counter--;  // Decrement the counter
    }

    ATTO_3_12D.data.u8[6] = (0x0F | (frame6_counter << 4));
    ATTO_3_12D.data.u8[7] = (0x09 | (frame7_counter << 4));

    ESP32Can.CANWriteFrame(&ATTO_3_12D);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    counter_100ms++;

    if (counter_100ms > 3) {
      ATTO_3_411.data.u8[5] = 0x01;
      ATTO_3_411.data.u8[7] = 0xF5;
    }

    ESP32Can.CANWriteFrame(&ATTO_3_411);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("BYD Atto 3 battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;  // Over this charging is not possible
  datalayer.battery.info.min_design_voltage_dV = 3100;  // Under this discharging is disabled
}

#endif
