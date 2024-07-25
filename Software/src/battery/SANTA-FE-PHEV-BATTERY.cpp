#include "../include.h"
#ifdef SANTA_FE_PHEV_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "SANTA-FE-PHEV-BATTERY.h"

/* Credits go to maciek16c for these findings!
https://github.com/maciek16c/hyundai-santa-fe-phev-battery
https://openinverter.org/forum/viewtopic.php?p=62256

TODO: Check if CRC function works like it should
TODO: Map all values from battery CAN messages
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send

static uint16_t SOC_BMS = 0;
static uint16_t batterySOH = 1000;
static uint16_t CellVoltMax_mV = 3700;
static uint16_t CellVoltMin_mV = 3700;
static uint16_t allowedDischargePower = 0;
static uint16_t allowedChargePower = 0;
static uint16_t batteryVoltage = 0;
static int16_t leadAcidBatteryVoltage = 120;
static int16_t temperatureMax = 0;
static int16_t temperatureMin = 0;
static int16_t batteryAmps = 0;
static uint8_t counter_200 = 0;
static uint8_t checksum_200 = 0;
static uint8_t StatusBattery = 0;

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

  datalayer.battery.status.real_soc = SOC_BMS;  //TODO: Scaling?

  datalayer.battery.status.voltage_dV = batteryVoltage;  //TODO: Scaling?

  datalayer.battery.status.current_dA = batteryAmps;  //TODO: Scaling?

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = allowedDischargePower * 10;

  datalayer.battery.status.max_charge_power_W = allowedChargePower * 10;

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  if (temperatureMax > temperatureMin) {
    datalayer.battery.status.temperature_min_dC = temperatureMin;

    datalayer.battery.status.temperature_max_dC = temperatureMax;
  } else {
    datalayer.battery.status.temperature_min_dC = temperatureMax;

    datalayer.battery.status.temperature_max_dC = temperatureMin;
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.MsgID) {
    case 0x1FF:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      StatusBattery = (rx_frame.data.u8[0] & 0x0F);
      break;
    case 0x4DE:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperatureMax = (rx_frame.data.u8[2] - 40);
      break;
    case 0x542:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_BMS = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]) / 2;
      break;
    case 0x588:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batteryVoltage = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[0]);
      break;
    case 0x5AD:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];  // Best guess, signed?
      break;
    case 0x620:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      leadAcidBatteryVoltage = rx_frame.data.u8[1];
      break;
    case 0x670:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      break;
    case 0x671:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperatureMin = (rx_frame.data.u8[0] - 40);
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
    } else {
      clear_event(EVENT_CAN_OVERRUN);
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

  datalayer.battery.info.max_design_voltage_dV =
      4040;  // 404.0V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 3100;  // 310.0V under this, discharging further is disabled
}

#endif
