#include "BMW-I3-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

//TODO: before using
// Map the final values in update_values_i3_battery, set some to static values if not available (current, discharge max , charge max)
// Continue with 13E

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was send
static unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was send
static unsigned long previousMillis30 = 0;    // will store last time a 30ms CAN Message was send
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;   // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;   // will store last time a 500ms CAN Message was send
static unsigned long previousMillis600 = 0;   // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis5000 = 0;  // will store last time a 5000ms CAN Message was send
static const int interval10 = 10;             // interval (ms) at which send CAN Messages
static const int interval20 = 20;             // interval (ms) at which send CAN Messages
static const int interval30 = 30;             // interval (ms) at which send CAN Messages
static const int interval100 = 100;           // interval (ms) at which send CAN Messages
static const int interval200 = 200;           // interval (ms) at which send CAN Messages
static const int interval500 = 500;           // interval (ms) at which send CAN Messages
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
                      .data = {0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_AD = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xAD,
                      .data = {0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0xF0, 0xFF}};
CAN_frame_t BMW_100_0 = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x100,
                         .data = {0xD6, 0xF1, 0x7F, 0xC0, 0x5D, 0x00, 0x00, 0x00}};
CAN_frame_t BMW_100_1 = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x100,
                         .data = {0xDB, 0xF1, 0x7F, 0xC0, 0x5D, 0x1D, 0x80, 0x70}};
CAN_frame_t BMW_108 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x108,
                       .data = {0x00, 0x7D, 0xFF, 0xFF, 0x07, 0xF1, 0xFF, 0xFF}};
CAN_frame_t BMW_10B = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x10B,
                       .data = {0xCD, 0x01, 0xFC}};
CAN_frame_t BMW_13E = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x13E,
                       .data = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t BMW_153 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x153,
                       .data = {0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0}};
CAN_frame_t BMW_197 = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x197,
                       .data = {0x64, 0xE0, 0x0E, 0xC0}};
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
                       .data = {0xE8, 0x28, 0x8A, 0x1C, 0xF1, 0x31, 0x33, 0x00}};  //0x12F Wakeup VCU
CAN_frame_t BMW_13D = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x13D,
                       .data = {0xFF, 0xFF, 0x00, 0x60, 0x41, 0x16, 0xF4, 0xFF}};
CAN_frame_t BMW_1A1 = {.FIR = {.B =
                                   {
                                       .DLC = 5,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x1A1,
                       .data = {0x08, 0xC1, 0x00, 0x00, 0x81}};  //0x1A1 Vehicle speed
CAN_frame_t BMW_1D0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x1D0,
                       .data = {0x4D, 0xF0, 0xAE, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_19E = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x19E,
                       .data = {0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_105 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x105,
                       .data = {0x03, 0xF0, 0x7F, 0xE0, 0x2E, 0x00, 0xFC, 0x0F}};
CAN_frame_t BMW_2FA = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2FA,
                       .data = {0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2FC = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2FC,
                       .data = {0x81, 0x00, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2A0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2A0,
                       .data = {0x88, 0x88, 0xF8, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF}};
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
CAN_frame_t BMW_3E9 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E9,
                       .data = {0xB0, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t BMW_328 = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x328,
                       .data = {0xB0, 0x33, 0xBF, 0x0C, 0xD3, 0x20}};
CAN_frame_t BMW_330 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x330,
                       .data = {0x27, 0x33, 0x01, 0x00, 0x00, 0x00, 0x4D, 0x02}};
CAN_frame_t BMW_397 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x397,
                       .data = {0x00, 0x2A, 0x00, 0x6C, 0x0F, 0x55, 0x00}};
CAN_frame_t BMW_433 = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x433,
                       .data = {0xFF, 0x00, 0x0F, 0xFF}};
CAN_frame_t BMW_51A = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x51A,
                       .data = {0x0, 0x0, 0x0, 0x0, 0x50, 0x0, 0x0, 0x1A}};
CAN_frame_t BMW_510 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x510,
                       .data = {0x40, 0x10, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t BMW_540 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x540,
                       .data = {0x0, 0x0, 0x0, 0x0, 0xFD, 0x3C, 0xFF, 0x40}};
CAN_frame_t BMW_560 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x560,
                       .data = {0x0, 0x0, 0x0, 0x0, 0xFE, 0x00, 0x00, 0x60}};

//These CAN messages need to be sent towards the battery to keep it alive

static const uint8_t BMW_10B_0[15] = {0xCD, 0x19, 0x94, 0x6D, 0xE0, 0x34, 0x78, 0xDB,
                                      0x97, 0x43, 0x0F, 0xF6, 0xBA, 0x6E, 0x81};
static const uint8_t BMW_10B_1[15] = {0x01, 0x02, 0x33, 0x34, 0x05, 0x06, 0x07, 0x08,
                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x00};
static const uint8_t BMW_197_0[15] = {0x89, 0x06, 0x8A, 0x05, 0x8F, 0x00, 0x8C, 0x03,
                                      0x85, 0x0A, 0x86, 0x09, 0x83, 0x0C, 0x80};
static const uint8_t BMW_105_0[15] = {0x03, 0x5E, 0xB9, 0xE4, 0x6A, 0x37, 0xD0, 0x8D,
                                      0xD1, 0x8C, 0x6B, 0x36, 0xB8, 0xE5, 0x02};
static const uint8_t BMW_100_0_0[15] = {0xD6, 0x31, 0x6C, 0xE2, 0xB9, 0x5E, 0x03, 0x5F,
                                        0x02, 0xE5, 0xB8, 0x36, 0x6B, 0x8C, 0x8D};
static const uint8_t BMW_100_0_1[15] = {0x86, 0xDB, 0x3C, 0x61, 0xEF, 0xB2, 0x55, 0x08,
                                        0x54, 0x09, 0xEE, 0xB3, 0x3D, 0x60, 0x87};
static const uint8_t BMW_1D0_0[15] = {0x4D, 0x10, 0xF7, 0xAA, 0x24, 0x79, 0x9E, 0xC3,
                                      0x9F, 0xC2, 0x25, 0x78, 0xF6, 0xAB, 0x4C};
static const uint8_t BMW_F0_FE[15] = {0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
                                      0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE};
static const uint8_t BMW_19E_0[6] = {0x05, 0x00, 0x05, 0x07, 0x0A, 0x0A};
static uint8_t BMW_10B_counter = 0;
static uint8_t BMW_100_0_counter = 0;
static uint8_t BMW_100_1_counter = 0;
static uint8_t BMW_105_counter = 0;
static uint8_t BMW_153_counter = 0;
static uint8_t BMW_1D0_counter = 0;
static uint8_t BMW_197_counter = 0;
static uint8_t BMW_19E_counter = 0;
static uint8_t BMW_10_counter = 0;
static uint8_t BMW_13D_counter = 0;
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
    case 0x430:  //BMS
      break;
    case 0x1FA:  //BMS
      break;
    case 0x40D:  //BMS
      break;
    case 0x2FF:  //BMS
      break;
    case 0x239:  //BMS
      break;
    case 0x2BD:  //BMS
      break;
    case 0x2F5:  //BMS
      break;
    case 0x3EB:  //BMS
      break;
    case 0x363:  //BMS
      break;
    case 0x507:
      break;
    case 0x41C:  //BMS
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

    BMW_105.data.u8[0] = BMW_105_0[BMW_105_counter];
    BMW_105.data.u8[1] = BMW_F0_FE[BMW_105_counter];

    BMW_105_counter++;
    if (BMW_105_counter > 14) {
      BMW_105_counter = 0;
    }

    BMW_10_counter++;  //The three first frames of these messages are special
    if (BMW_10_counter > 3) {
      BMW_10_counter = 3;

      BMW_BB.data.u8[0] = 0x7D;

      BMW_AD.data.u8[2] = 0xFE;
      BMW_AD.data.u8[3] = 0xE7;
      BMW_AD.data.u8[4] = 0x7F;
      BMW_AD.data.u8[5] = 0xFE;
    }

    BMW_100_0_counter++;  //The initial first 15 messages
    if (BMW_100_0_counter < 15) {
      BMW_100_0.data.u8[0] = BMW_100_0_0[(BMW_100_0_counter - 1)];
      BMW_100_0.data.u8[1] = BMW_F0_FE[(BMW_100_0_counter - 1)];
      if (BMW_100_0_counter < 4) {
        BMW_100_0.data.u8[3] = 0xC0;
        BMW_100_0.data.u8[4] = 0x5D;
        BMW_100_0.data.u8[5] = 0x00;
        BMW_100_0.data.u8[6] = 0x00;
        BMW_100_0.data.u8[7] = 0x00;
      } else {  // 5-14
        BMW_100_0.data.u8[3] = 0xFF;
        BMW_100_0.data.u8[4] = 0xFF;
        BMW_100_0.data.u8[5] = 0x1D;
        BMW_100_0.data.u8[6] = 0x80;
        BMW_100_0.data.u8[7] = 0x70;
      }
      ESP32Can.CANWriteFrame(&BMW_100_0);
    } else {                   // The rest of the looping 0x100 messages
      BMW_100_0_counter = 15;  //Make sure we never go back to the initial 15 messages

      BMW_100_1.data.u8[0] = BMW_100_0_1[BMW_100_1_counter];
      BMW_100_1.data.u8[1] = BMW_F0_FE[BMW_100_1_counter];

      BMW_100_1_counter++;
      if (BMW_100_1_counter > 14) {
        BMW_100_1_counter = 0;
      }
      ESP32Can.CANWriteFrame(&BMW_100_1);
    }

    BMW_13D_counter++;

    if (BMW_13D_counter > 3) {
      BMW_13D_counter = 5;
      BMW_13D.data.u8[0] = 0xFF;
      BMW_13D.data.u8[1] = 0xFF;
      BMW_13D.data.u8[2] = 0xFE;
      BMW_13D.data.u8[3] = 0x27;
      BMW_13D.data.u8[4] = 0x9F;
      BMW_13D.data.u8[5] = 0x0A;
      BMW_13D.data.u8[6] = 0xF6;
      BMW_13D.data.u8[7] = 0xFF;
    }

    ESP32Can.CANWriteFrame(&BMW_BB);
    ESP32Can.CANWriteFrame(&BMW_105);
    ESP32Can.CANWriteFrame(&BMW_AD);
    ESP32Can.CANWriteFrame(&BMW_13D);
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

    BMW_153_counter++;

    if (BMW_153_counter > 2) {
      BMW_153.data.u8[2] = 0x01;
      BMW_153.data.u8[6] = 0xF0;
    }
    if (BMW_153_counter > 4) {
      BMW_153.data.u8[2] = 0x01;
      BMW_153.data.u8[6] = 0xF6;
    }
    if (BMW_153_counter > 30) {
      BMW_153.data.u8[2] = 0x01;
      BMW_153.data.u8[5] = 0x20;
      BMW_153.data.u8[6] = 0xF7;
      BMW_153_counter == 31;  //Stop the counter, maybe this is enough
    }

    ESP32Can.CANWriteFrame(&BMW_10B);
    ESP32Can.CANWriteFrame(&BMW_1A1);
    ESP32Can.CANWriteFrame(&BMW_153);
  }
  //Send 30ms message
  if (currentMillis - previousMillis30 >= interval30) {
    previousMillis30 = currentMillis;

    BMW_197.data.u8[0] = BMW_197_0[BMW_197_counter];
    BMW_197.data.u8[1] = BMW_197_counter;
    BMW_197_counter++;
    if (BMW_197_counter > 14) {
      BMW_197_counter = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_197);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_12F);
    ESP32Can.CANWriteFrame(&BMW_2FC);
    ESP32Can.CANWriteFrame(&BMW_108);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= interval200) {
    previousMillis200 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_03C);
    ESP32Can.CANWriteFrame(&BMW_3E9);
    ESP32Can.CANWriteFrame(&BMW_2A0);
    ESP32Can.CANWriteFrame(&BMW_397);
    ESP32Can.CANWriteFrame(&BMW_510);
    ESP32Can.CANWriteFrame(&BMW_540);
    ESP32Can.CANWriteFrame(&BMW_560);
  }
  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= interval500) {
    previousMillis500 = currentMillis;

    BMW_19E.data.u8[0] = BMW_19E_0[BMW_19E_counter];
    BMW_19E.data.u8[3] = 0x04;
    if (BMW_19E_counter == 1) {
      BMW_19E.data.u8[3] = 0x00;
    }
    if (BMW_19E_counter < 5) {
      BMW_19E_counter++;
    }

    ESP32Can.CANWriteFrame(&BMW_19E);
  }
  // Send 600ms CAN Message
  if (currentMillis - previousMillis600 >= interval600) {
    previousMillis600 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_512);
    ESP32Can.CANWriteFrame(&BMW_51A);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= interval1000) {
    previousMillis1000 = currentMillis;

    BMW_328_counter++;
    BMW_328.data.u8[0] = BMW_328_counter;  //rtc msg. needs to be every 1 sec. first 32 bits are 1 second wrap counter
    BMW_328.data.u8[1] = BMW_328_counter << 8;
    BMW_328.data.u8[2] = BMW_328_counter << 16;
    BMW_328.data.u8[3] = BMW_328_counter << 24;

    BMW_1D0.data.u8[0] = BMW_1D0_0[BMW_1D0_counter];
    BMW_1D0.data.u8[1] = BMW_F0_FE[BMW_1D0_counter];

    if (BMW_328_counter > 1) {
      BMW_433.data.u8[1] = 0x01;
    }

    ESP32Can.CANWriteFrame(&BMW_328);
    ESP32Can.CANWriteFrame(&BMW_3F9);
    ESP32Can.CANWriteFrame(&BMW_3E8);
    ESP32Can.CANWriteFrame(&BMW_2FA);
    ESP32Can.CANWriteFrame(&BMW_1D0);
    ESP32Can.CANWriteFrame(&BMW_433);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= interval5000) {
    previousMillis5000 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_3A0);
    ESP32Can.CANWriteFrame(&BMW_330);
  }
}
