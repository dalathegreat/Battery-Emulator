#include "BMW-I3-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

//TODO: before using
// Map the final values in update_values_i3_battery, set some to static values if not available (current, discharge max , charge max)
// Check if I3 battery stays alive with only 10B and 512. If not, add 12F. If that doesn't help, add more from CAN log (ask Dala)

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;    // will store last time a 20ms CAN Message was send
static unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was send
static unsigned long previousMillis100 = 0;   // will store last time a 20ms CAN Message was send
static unsigned long previousMillis200 = 0;   // will store last time a 20ms CAN Message was send
static unsigned long previousMillis600 = 0;   // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis5000 = 0;  // will store last time a 5000ms CAN Message was send
static const int interval10 = 10;             // interval (ms) at which send CAN Messages
static const int interval20 = 20;             // interval (ms) at which send CAN Messages
static const int interval100 = 100;           // interval (ms) at which send CAN Messages
static const int interval200 = 200;           // interval (ms) at which send CAN Messages
static const int interval600 = 600;           // interval (ms) at which send CAN Messages
static const int interval1000 = 1000;         // interval (ms) at which send CAN Messages
static const int interval5000 = 5000;         // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;            //counter for checking if CAN is still alive

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

CAN_frame_t BMW_BB = {.FIR = {.B =
                                  {
                                      .DLC = 3,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xBB,
                      .data = {0x7D, 0xFF, 0xFF}};
CAN_frame_t BMW_10B = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x10B,
                       .data = {0xCD, 0x01, 0xFC}};
CAN_frame_t BMW_512 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x512,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}};  //0x512 Network management edme VCU
CAN_frame_t BMW_03C = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x03C,
                       .data = {0xFF, 0x5F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_12F = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x12F,
                       .data = {0xE8, 0x28, 0x86, 0x1C, 0xF1, 0x31, 0x33, 0x00}};  //0x12F Wakeup VCU
CAN_frame_t BMW_1A1 = {.FIR = {.B =
                                   {
                                       .DLC = 5,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x1A1,
                       .data = {0x08, 0xC1, 0x00, 0x00, 0x81}};  //0x1A1 Vehicle speed
CAN_frame_t BMW_3F9 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3F9,
                       .data = {0x1F, 0x34, 0x00, 0xE2, 0xA6, 0x30, 0xC3, 0xFF}};
CAN_frame_t BMW_3A0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3A0,
                       .data = {0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC}};
CAN_frame_t BMW_3E8 = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E8,
                       .data = {0xF1, 0xFF}};  //1000ms OBD reset
CAN_frame_t BMW_328 = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x328,
                       .data = {0xB0, 0x33, 0xBF, 0x0C, 0xD3, 0x20}};
CAN_frame_t BMW_51A = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x51A,
                       .data = {0x0, 0x0, 0x0, 0x0, 0x50, 0x0, 0x0, 0x1A}};

//These CAN messages need to be sent towards the battery to keep it alive

static const uint8_t BMW_10B_0[15] = {0xCD, 0x19, 0x94, 0x6D, 0xE0, 0x34, 0x78, 0xDB,
                                      0x97, 0x43, 0x0F, 0xF6, 0xBA, 0x6E, 0x81};
static const uint8_t BMW_10B_1[15] = {0x01, 0x02, 0x33, 0x34, 0x05, 0x06, 0x07, 0x08,
                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x00};
static uint8_t BMW_10B_counter = 0;
static uint32_t BMW_328_counter = 0;

static int16_t Battery_Current = 0;
static uint16_t Battery_Capacity_kWh = 0;
static uint16_t Voltage_Setpoint = 0;
static uint16_t Low_SOC = 0;
static uint16_t High_SOC = 0;
static uint16_t Display_SOC = 0;
static uint16_t Calculated_SOC = 0;
static uint16_t Battery_Volts = 0;
static uint16_t HVBatt_SOC = 0;
static uint16_t Battery_Status = 0;
static uint16_t DC_link = 0;
static int16_t Battery_Power = 0;

void update_values_i3_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE;             //Startout in active mode

  //Calculate the SOC% value to send to inverter
  Calculated_SOC = (Display_SOC * 10);  //Increase decimal amount
  Calculated_SOC =
      LB_MIN_SOC + (LB_MAX_SOC - LB_MIN_SOC) * (Calculated_SOC - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE);
  if (Calculated_SOC < 0) {  //We are in the real SOC% range of 0-20%, always set SOC sent to inverter as 0%
    Calculated_SOC = 0;
  }
  if (Calculated_SOC > 1000) {  //We are in the real SOC% range of 80-100%, always set SOC sent to inverter as 100%
    Calculated_SOC = 1000;
  }
  SOC = (Calculated_SOC * 10);  //increase LB_SOC range from 0-100.0 -> 100.00

  battery_voltage = Battery_Volts;  //Unit V+1 (5000 = 500.0V)

  battery_current = Battery_Current;

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh = (Battery_Capacity_kWh * 1000);

  if (SOC > 9900)  //If Soc is over 99%, stop charging
  {
    max_target_charge_power = 0;
  } else {
    max_target_charge_power = 5000;  //Hardcoded value for testing. TODO: read real value from battery when discovered
  }

  if (SOC < 500)  //If Soc is under 5%, stop dicharging
  {
    max_target_discharge_power = 0;
  } else {
    max_target_discharge_power =
        5000;  //Hardcoded value for testing. TODO: read real value from battery when discovered
  }

  Battery_Power = (Battery_Current * (Battery_Volts / 10));

  stat_batt_power = Battery_Power;  //TODO:, is mapping OK?

  temperature_min;  //hardcoded to 5*C in startup, TODO:, find from battery CAN later

  temperature_max;  //hardcoded to 6*C in startup, TODO:, find from battery CAN later

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

#ifdef DEBUG_VIA_USB
  Serial.print("SOC% battery: ");
  Serial.print(Display_SOC);
  Serial.print(" SOC% sent to inverter: ");
  Serial.print(SOC);
  Serial.print(" Battery voltage: ");
  Serial.print(battery_voltage);
  Serial.print(" Remaining Wh: ");
  Serial.print(remaining_capacity_Wh);
  Serial.print(" Max charge power: ");
  Serial.print(max_target_charge_power);
  Serial.print(" Max discharge power: ");
  Serial.print(max_target_discharge_power);
#endif
}

void receive_can_i3_battery(CAN_frame_t rx_frame) {
  CANstillAlive = 12;
  switch (rx_frame.MsgID) {
    case 0x431:  //Battery capacity [200ms]
      Battery_Capacity_kWh = (((rx_frame.data.u8[1] & 0x0F) << 8 | rx_frame.data.u8[5])) / 50;
      break;
    case 0x432:  //SOC% charged [200ms]
      Voltage_Setpoint = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      Low_SOC = (rx_frame.data.u8[2] / 2);
      High_SOC = (rx_frame.data.u8[3] / 2);
      Display_SOC = (rx_frame.data.u8[4] / 2);
      break;
    case 0x112:  //BMS status [10ms]
      CANstillAlive = 12;
      Battery_Current = ((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) / 10) - 819;  //Amps
      Battery_Volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);                 //500.0 V
      HVBatt_SOC = ((rx_frame.data.u8[5] & 0x0F) << 4 | rx_frame.data.u8[4]) / 10;
      Battery_Status = (rx_frame.data.u8[6] & 0x0F);
      DC_link = rx_frame.data.u8[7];
      break;
    case 0x430:
      break;
    case 0x1FA:
      break;
    case 0x40D:
      break;
    case 0x2FF:
      break;
    case 0x239:
      break;
    case 0x2BD:
      break;
    case 0x2F5:
      break;
    case 0x3EB:
      break;
    case 0x363:
      break;
    case 0x507:
      break;
    case 0x41C:
      break;
    default:
      break;
  }
}
void send_can_i3_battery() {
  unsigned long currentMillis = millis();
  //Send 10ms message
  if (currentMillis - previousMillis10 >= interval10) {
    previousMillis10 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_BB);
  }
  //Send 20ms message
  if (currentMillis - previousMillis20 >= interval20) {
    previousMillis20 = currentMillis;

    BMW_10B.data.u8[0] = BMW_10B_0[BMW_10B_counter];
    BMW_10B.data.u8[1] = BMW_10B_1[BMW_10B_counter];
    BMW_10B_counter++;
    if (BMW_10B_counter > 14) {
      BMW_10B_counter = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_10B);
    ESP32Can.CANWriteFrame(&BMW_1A1);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_12F);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= interval200) {
    previousMillis200 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_03C);
  }
  // Send 600ms CAN Message
  if (currentMillis - previousMillis600 >= interval600) {
    previousMillis600 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_512);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= interval1000) {
    previousMillis1000 = currentMillis;

    BMW_328_counter++;
    BMW_328.data.u8[0] = BMW_328_counter;  //rtc msg. needs to be every 1 sec. first 32 bits are 1 second wrap counter
    BMW_328.data.u8[1] = BMW_328_counter << 8;
    BMW_328.data.u8[2] = BMW_328_counter << 16;
    BMW_328.data.u8[3] = BMW_328_counter << 24;

    ESP32Can.CANWriteFrame(&BMW_328);
    ESP32Can.CANWriteFrame(&BMW_51A);
    ESP32Can.CANWriteFrame(&BMW_3F9);
    ESP32Can.CANWriteFrame(&BMW_3E8);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= interval5000) {
    previousMillis5000 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_3A0);
  }
}
