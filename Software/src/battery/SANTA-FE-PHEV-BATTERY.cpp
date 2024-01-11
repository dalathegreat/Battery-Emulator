#include "SANTA-FE-PHEV-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

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
static const int interval10 = 10;            // interval (ms) at which send CAN Messages
static const int interval100 = 100;          // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;           //counter for checking if CAN is still alive

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

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

void update_values_santafe_phev_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  SOC;

  battery_voltage;

  battery_current;

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh;

  max_target_discharge_power;

  max_target_charge_power;

  stat_batt_power;

  temperature_min;

  temperature_max;

  bms_status = ACTIVE;  //Startout in active mode, then check safeties

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_santafe_phev_battery(CAN_frame_t rx_frame) {
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
void send_can_santafe_phev_battery() {
  unsigned long currentMillis = millis();
  //Send 10ms message
  if (currentMillis - previousMillis10 >= interval10) {
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
  if (currentMillis - previousMillis100 >= interval100) {
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
