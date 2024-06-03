#include "../include.h"
#ifdef BYD_ATTO_3_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "BYD-ATTO-3-BATTERY.h"

/* TODO: 
- Get contactor closing working
  - NOTE: Some packs can be locked hard? after a crash has occured. Bypassing contactors manually might be required
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
  switch (rx_frame.MsgID) { //Log values taken with 422V from battery
    case 0x244: //00,00,00,04,41,0F,20,8B - 0x441 = 1089 , 0xF20 = 3872 , 0x410F / 4 = 4163
      break;
    case 0x245: //01,00,02,19,3A,25,90,F4
                //02,00,90,01,79,79,90,EA
                //03,C6,88,12,FD,48,90,5C
                //04,00,FF,FF,00,00,90,6D
      break;
    case 0x286: //01,FF,FF,FF,FF,FF,FF,04
      break;
    case 0x334: //FF,FF,FF,FC,3F,00,F0,D7
      break;
    case 0x338: //01,52,02,00,88,13,00,0F
      break;
    case 0x344: //00,52,02,CC,1F,FF,04,BD
      break;
    case 0x345: //27,0B,00,00,00,E0,01,EC
      break;
    case 0x347: //FF,00,00,F9,FF,FF,FF,0A
      break;
    case 0x34A: //00,52,02,CC,1F,FF,04,BD
      break;
    case 0x35E: //01,00,C8,32,00,63,00,A1 - Flickering between A0 and A1, Could be temperature?
      break;
    case 0x360: //30,19,DE,D1,0B,C3,4B,EE - Last and first byte has F-0 counters - 0x10BC (4284) 
      break;
    case 0x36C: //01,57,13,DC,08,70,17,29
                //02,03,DC,05,C0,0F,0F,3B
                //03,86,01,40,06,5C,02,D1
                //04,57,13,73,04,01,FF,1A
                //05,FF,FF,FF,FF,FF,FF,00
      break;
    case 0x438: //55,55,01,F6,47,2E,10,D9 - 0x10D9 = 4313
      break;
    case 0x43A: //7E,0A,B0,1C,63,E1,03,64
      break;
    case 0x43B: //01,3B,06,39,FF,64,64,BD
      break;
    case 0x43C: //00,3B,3A,3A,3A,3A,39,A3
                //01,3A,3A,39,3A,FF,FF,19
      break;
    case 0x43D: //Varies a lot
      break;
    case 0x444: //9E,01,88,13,64,64,98,65
      break;
    case 0x445: //00,98,FF,FF,63,20,4E,98
      break;
    case 0x446: //2C,D4,0C,4D,21,DC,0C,9D - 0,1,7th frame varies a lot
      break;
    case 0x447: //06,39,01,3B,E0,03,3A,67
      break;
    case 0x47B: //01,FF,FF,FF,FF,FF,FF,FF
      break;
    case 0x524: //24,40,00,00,00,00,00,9B
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
