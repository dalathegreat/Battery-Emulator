#include "BATTERIES.h"
#ifdef SANTA_FE_PHEV_BATTERY
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "SANTA-FE-PHEV-BATTERY.h"

/* Credits go to maciek16c for these findings!
https://github.com/maciek16c/hyundai-santa-fe-phev-battery
https://openinverter.org/forum/viewtopic.php?p=62256

TODO: Check if contactors close
TODO: Check if CRC function works like it should
TODO: Map all values from battery CAN messages
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static uint8_t CANstillAlive = 12;           //counter for checking if CAN is still alive

static int SOC_1 = 0;
static int SOC_2 = 0;
static int SOC_3 = 0;
static uint8_t counter_200 = 0;
static uint8_t checksum_200 = 0;

CAN_frame_t SANTAFE_200 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x200,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x00, 0x00}};
CAN_frame_t SANTAFE_2A1 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x2A1,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x02}};
CAN_frame_t SANTAFE_2F0 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x2F0,
                           .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00}};
CAN_frame_t SANTAFE_523 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x523,
                           .data = {0x60, 0x00, 0x60, 0, 0, 0, 0, 0}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  system_real_SOC_pptt;

  system_battery_voltage_dV;

  system_battery_current_dA;

  system_capacity_Wh = BATTERY_WH_MAX;

  system_remaining_capacity_Wh;

  system_max_discharge_power_W;

  system_max_charge_power_W;

  system_active_power_W;

  system_temperature_min_dC;

  system_temperature_max_dC;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  CANstillAlive = 12;
  switch (rx_frame.MsgID) {
    case 0x200:
      break;
    case 0x201:
      break;
    case 0x202:
      break;
    case 0x203:
      break;
    case 0x2A0:
      break;
    case 0x2A1:
      break;
    case 0x2A2:
      break;
    case 0x2B0:
      break;
    case 0x2B5:
      break;
    case 0x2F0:
      break;
    case 0x2F1:
      break;
    case 0x523:
      break;
    case 0x524:
      break;
    case 0x620:
      break;
    default:
      break;
  }
}
void send_can_battery() {
  unsigned long currentMillis = millis();
  //Send 10ms message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    }
    previousMillis10 = currentMillis;

    SANTAFE_200.data.u8[6] = (counter_200 << 1);

    checksum_200 = CalculateCRC8(SANTAFE_200);

    SANTAFE_200.data.u8[7] = checksum_200;

    ESP32Can.CANWriteFrame(&SANTAFE_200);

    ESP32Can.CANWriteFrame(&SANTAFE_2A1);

    ESP32Can.CANWriteFrame(&SANTAFE_2F0);

    counter_200++;
    if (counter_200 > 0xF) {
      counter_200 = 0;
    }
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    ESP32Can.CANWriteFrame(&SANTAFE_523);
  }
}

uint8_t CalculateCRC8(CAN_frame_t rx_frame) {
  int crc = 0;

  for (uint8_t framepos = 0; framepos < 8; framepos++) {
    crc ^= rx_frame.data.u8[framepos];

    for (uint8_t j = 0; j < 8; j++) {
      if ((crc & 0x80) != 0) {
        crc = (crc << 1) ^ 0x1;
      } else {
        crc <<= 1;
      }
    }
  }
  return (uint8_t)crc;
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Hyundai Santa Fe PHEV battery selected");
#endif

  system_max_design_voltage_dV = 4040;  // 404.0V, over this, charging is not possible (goes into forced discharge)
  system_min_design_voltage_dV = 3100;  // 310.0V under this, discharging further is disabled
}

#endif
