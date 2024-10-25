#include "../include.h"
#ifdef BMW_IX_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BMW-IX-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
static unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
static unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

#define ALIVE_MAX_VALUE 14  // BMW CAN messages contain alive counter, goes from 0...14

enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

static CmdState cmdState = SOC;

CAN_frame BMW_6F1_CELL = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0xDD, 0xBF}};
CAN_frame BMW_6F1_SOH = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0x63, 0x35}};
CAN_frame BMW_6F1_SOC = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0xDD, 0xBC}};
CAN_frame BMW_6F1_CELL_VOLTAGE_AVG = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 5,
                                      .ID = 0x6F1,
                                      .data = {0x07, 0x03, 0x22, 0xDF, 0xA0}};
CAN_frame BMW_6F1_CONTINUE = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x6F1, .data = {0x07, 0x30, 0x00, 0x02}};
CAN_frame BMW_6F4_CELL_VOLTAGE_CELLNO = {.FD = false,
                                         .ext_ID = false,
                                         .DLC = 7,
                                         .ID = 0x6F4,
                                         .data = {0x07, 0x05, 0x31, 0x01, 0xAD, 0x6E, 0x01}};
CAN_frame BMW_6F4_CELL_CONTINUE = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 6,
                                   .ID = 0x6F4,
                                   .data = {0x07, 0x04, 0x31, 0x03, 0xAD, 0x6E}};

static bool battery_awake = false;

static uint8_t current_cell_polled = 0;

static uint8_t increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.info.total_capacity_Wh;

  datalayer.battery.status.remaining_capacity_Wh;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.active_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;
}

void receive_can_battery(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x112:
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();

  if (battery_awake) {
    //Send 20ms message
    if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
      // Check if sending of CAN messages has been delayed too much.
      if ((currentMillis - previousMillis20 >= INTERVAL_20_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
        set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis20));
      } else {
        clear_event(EVENT_CAN_OVERRUN);
      }
      previousMillis20 = currentMillis;
    }
    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;
    }
    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;
    }
    // Send 500ms CAN Message
    if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
      previousMillis500 = currentMillis;
    }
    // Send 640ms CAN Message
    if (currentMillis - previousMillis640 >= INTERVAL_640_MS) {
      previousMillis640 = currentMillis;
    }
    // Send 1000ms CAN Message
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;

      switch (cmdState) {
        case SOC:
          transmit_can(&BMW_6F1_CELL, can_config.battery);
          cmdState = CELL_VOLTAGE_MINMAX;
          break;
        case CELL_VOLTAGE_MINMAX:
          transmit_can(&BMW_6F1_SOH, can_config.battery);
          cmdState = SOH;
          break;
        case SOH:
          transmit_can(&BMW_6F1_CELL_VOLTAGE_AVG, can_config.battery);
          cmdState = CELL_VOLTAGE_CELLNO;
          current_cell_polled = 0;

          break;
        case CELL_VOLTAGE_CELLNO:
          current_cell_polled++;
          if (current_cell_polled > 96) {
            datalayer.battery.info.number_of_cells = 97;
            cmdState = CELL_VOLTAGE_CELLNO_LAST;
          } else {
            cmdState = CELL_VOLTAGE_CELLNO;

            BMW_6F4_CELL_VOLTAGE_CELLNO.data.u8[6] = current_cell_polled;
            transmit_can(&BMW_6F4_CELL_VOLTAGE_CELLNO, can_config.battery);
          }
          break;
        case CELL_VOLTAGE_CELLNO_LAST:
          transmit_can(&BMW_6F1_SOC, can_config.battery);
          cmdState = SOC;
          break;
      }
    }
    // Send 5000ms CAN Message
    if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
      previousMillis5000 = currentMillis;
    }
    // Send 10000ms CAN Message
    if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
      previousMillis10000 = currentMillis;
    }
  } else {
    previousMillis20 = currentMillis;
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    previousMillis500 = currentMillis;
    previousMillis640 = currentMillis;
    previousMillis1000 = currentMillis;
    previousMillis5000 = currentMillis;
    previousMillis10000 = currentMillis;
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("BMW iX battery selected");
#endif  //DEBUG_VIA_USB

  //Before we have started up and detected which battery is in use, use 60AH values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  pinMode(WUP_PIN, OUTPUT);
  digitalWrite(WUP_PIN, HIGH);  // Wake up the battery
}

#endif
