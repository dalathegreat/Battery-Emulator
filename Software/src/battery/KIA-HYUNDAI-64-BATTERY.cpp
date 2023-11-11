#include "KIA-HYUNDAI-64-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

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

void update_values_kiaHyundai_64_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE;                        //Startout in active mode

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

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

#ifdef DEBUG_VIA_USB
  Serial.print("SOC% candidate 1: ");
  Serial.println(SOC_1);
  Serial.print("SOC% candidate 2: ");
  Serial.println(SOC_2);
  Serial.print("SOC% candidate 3: ");
  Serial.println(SOC_3);
#endif
}

void receive_can_kiaHyundai_64_battery(CAN_frame_t rx_frame) {
  CANstillAlive = 12;
  switch (rx_frame.MsgID) {
    case 0x3F6:
      break;
    case 0x491:
      break;
    case 0x493:
      break;
    case 0x497:
      break;
    case 0x498:
      break;
    case 0x4DD:
      break;
    case 0x4DE:
      break;
    case 0x4E2:
      break;
    case 0x542:
      SOC_1 = rx_frame.data.u8[0];
      break;
    case 0x594:
      SOC_2 = rx_frame.data.u8[5];
      break;
    case 0x595:
      break;
    case 0x596:
      break;
    case 0x597:
      break;
    case 0x598:
      SOC_3 = (rx_frame.data.u8[4] * 256.0 + rx_frame.data.u8[5]);
      break;
    case 0x599:
      break;
    case 0x59C:
      break;
    case 0x59E:
      break;
    case 0x5A3:
      break;
    case 0x5D5:
      break;
    case 0x5D6:
      break;
    case 0x5D7:
      break;
    case 0x5D8:
      break;
    default:
      break;
  }
}
void send_can_kiaHyundai_64_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;
  }
  //Send 10ms message
  if (currentMillis - previousMillis10 >= interval10) {
    previousMillis10 = currentMillis;
  }
}
