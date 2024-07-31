#include "../include.h"
#ifdef SMA_RESU
#include "../datalayer/datalayer.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "SMA-RESU.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000ms = 0;  // will store last time a 1000ms CAN Message was send
static uint8_t counter_B0 = 0;

//Actual content messages
static const CAN_frame_t SMA_558 = {
    .FIR = {.B =
                {
                    .DLC = 8,
                    .FF = CAN_frame_std,
                }},
    .MsgID = 0x558,
    .data = {0x13, 0xFF, 0x00, 0x04, 0x00, 0x62, 0x01, 0x02}};  //RESU10H, Vendor ID 2 LG
static const CAN_frame_t SMA_598 = {.FIR = {.B =
                                                {
                                                    .DLC = 8,
                                                    .FF = CAN_frame_std,
                                                }},
                                    .MsgID = 0x598,
                                    .data = {0x6B, 0x69, 0x35, 0x5C, 0xFF, 0xFF, 0xFF, 0xFF}};  //Identifier?
static const CAN_frame_t SMA_5D8_1 = {.FIR = {.B =
                                                  {
                                                      .DLC = 8,
                                                      .FF = CAN_frame_std,
                                                  }},
                                      .MsgID = 0x5D8,
                                      .data = {0x00, 0x4C, 0x47, 0x20, 0x43, 0x48, 0x45, 0x4D}};  //LG CHEM
static const CAN_frame_t SMA_5D8_2 = {.FIR = {.B =
                                                  {
                                                      .DLC = 8,
                                                      .FF = CAN_frame_std,
                                                  }},
                                      .MsgID = 0x5D8,
                                      .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //1
static const CAN_frame_t SMA_618_1 = {.FIR = {.B =
                                                  {
                                                      .DLC = 8,
                                                      .FF = CAN_frame_std,
                                                  }},
                                      .MsgID = 0x618,
                                      .data = {0x00, 0x52, 0x45, 0x53, 0x55, 0x31, 0x30, 0x48}};  //0 RESU10H
static const CAN_frame_t SMA_618_2 = {.FIR = {.B =
                                                  {
                                                      .DLC = 8,
                                                      .FF = CAN_frame_std,
                                                  }},
                                      .MsgID = 0x618,
                                      .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //1
CAN_frame_t SMA_3D8 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3D8,
                       .data = {0x04, 0x10, 0x27, 0x10, 0x00, 0x18, 0xF9, 0x00}};
CAN_frame_t SMA_158 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x158,
                       .data = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x6A, 0xAA, 0xAA}};  //TODO: AA or 00 ?
CAN_frame_t SMA_B0 = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xB0,
                      .data = {0x00, 0xC8, 0x2B, 0xE2, 0x00, 0xE0, 0x00, 0x00}};

static int16_t discharge_current = 0;
static int16_t charge_current = 0;
static int16_t temperature_average = 0;
static uint16_t ampere_hours_remaining = 0;
static uint16_t ampere_hours_full = 0;

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  //Calculate values

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    discharge_current =
        ((datalayer.battery.status.max_discharge_power_W * 10) /
         datalayer.battery.status.voltage_dV);     //Charge power in W , max volt in V+1decimal (P=UI, solve for I)
    discharge_current = (discharge_current * 10);  //Value needs a decimal before getting sent to inverter (81.0A)
    charge_current =
        ((datalayer.battery.status.max_charge_power_W * 10) /
         datalayer.battery.status.voltage_dV);  //Charge power in W , max volt in V+1decimal (P=UI, solve for I)
    charge_current = (charge_current * 10);     //Value needs a decimal before getting sent to inverter (81.0A)
  }

  if (charge_current > datalayer.battery.info.max_charge_amp_dA) {
    charge_current =
        datalayer.battery.info
            .max_charge_amp_dA;  //Cap the value to the max allowed Amp. Some inverters cannot handle large values.
  }

  if (discharge_current > datalayer.battery.info.max_discharge_amp_dA) {
    discharge_current =
        datalayer.battery.info
            .max_discharge_amp_dA;  //Cap the value to the max allowed Amp. Some inverters cannot handle large values.
  }

  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    ampere_hours_remaining = ((datalayer.battery.status.remaining_capacity_Wh / datalayer.battery.status.voltage_dV) *
                              100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
    ampere_hours_full = ((datalayer.battery.status.total_capacity_Wh / datalayer.battery.status.voltage_dV) *
                         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
  }

  //Map values to CAN messages

  //SOC (100.00%)
  SMA_3D8.data.u8[0] = (datalayer.battery.status.reported_soc >> 8);
  SMA_3D8.data.u8[1] = (datalayer.battery.status.reported_soc & 0x00FF);
  //StateOfHealth (100.00%)
  SMA_3D8.data.u8[2] = (datalayer.battery.status.soh_pptt >> 8);
  SMA_3D8.data.u8[3] = (datalayer.battery.status.soh_pptt & 0x00FF);
  //Battery capacity actual (AH, 0.1)
  SMA_3D8.data.u8[4] = (ampere_hours_remaining >> 8);
  SMA_3D8.data.u8[5] = (ampere_hours_remaining & 0x00FF);
  //Battery capacity actual (AH, 0.1)
  SMA_3D8.data.u8[6] = (ampere_hours_full >> 8);
  SMA_3D8.data.u8[7] = (ampere_hours_full & 0x00FF);
}

void receive_can_inverter(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0x360:  //Message originating from SMA inverter - Voltage and current
                 /*SG_ Battery_current : 23|16@0- (0.1,0) [0|0] "A" Vector__XXX
   SG_ Battery_voltage : 7|16@0- (0.1,0) [0|0] "V" Vector__XXX
   SG_ Battery_temperature : 39|16@0- (0.1,0) [0|1] "°C" Vector__XXX
   SG_ Inverter_command : 55|8@0+ (1,0) [0|1] "" Vector__XXX
   SG_ Battery_control_flags : 63|8@0+ (1,0) [0|1] "" Vector__XXX */
      break;
    case 0x3E0:  //Message originating from SMA inverter - Timestamp
      /*   SG_ Inverter_SOC : 7|16@0- (0.01,0) [0|1] "%" Vector__XXX
   SG_ Inverter_active_error_message : 23|16@0+ (1,0) [0|1] "" Vector__XXX
   SG_ Inverter_reserve : 47|8@0+ (1,0) [0|1] "" Vector__XXX
   SG_ Inverter_state : 55|8@0+ (1,0) [0|1] "" Vector__XXX*/
      break;
    case 0x420:
      //    SG_ Inverter_time_unixtime : 7|32@0+ (1,0) [0|0] "" Vector__XXX
      break;
    case 0x5E0:  //Message originating from SMA inverter - String
      break;
    case 0x560:  //Message originating from SMA inverter - Init
      // Inverter sends info, reply with battery info
      ESP32Can.CANWriteFrame(&SMA_558);
      ESP32Can.CANWriteFrame(&SMA_598);
      ESP32Can.CANWriteFrame(&SMA_5D8_1);
      ESP32Can.CANWriteFrame(&SMA_5D8_2);
      ESP32Can.CANWriteFrame(&SMA_618_1);
      ESP32Can.CANWriteFrame(&SMA_618_2);
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();

  // Send CAN Message every 1000ms
  if (currentMillis - previousMillis1000ms >= INTERVAL_1_S) {
    previousMillis100ms = currentMillis;

    ESP32Can.CANWriteFrame(&SMA_90);  //Sent by BMS or SMA?
    SMA_B0.data.u8[0] = counter_B0;
    ESP32Can.CANWriteFrame(&SMA_B0);   //BMS Energy
    ESP32Can.CANWriteFrame(&SMA_F0);   //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_D0);   //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_110);  //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_130);  //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_150);  //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_170);  //Sent by BMS or SMA?

    ESP32Can.CANWriteFrame(&SMA_250);  // Only starts after 80 seconds. Sent by BMS or SMA?

    counter_B0++;
    if (counter_B0 > 0x0F) {
      counter_B0 = 0;
    }
  }

  // Send CAN Message every 5s
  if (currentMillis - previousMillis5000ms >= INTERVAL_5_S) {
    previousMillis100ms = currentMillis;

    ESP32Can.CANWriteFrame(&SMA_3D8);
    ESP32Can.CANWriteFrame(&SMA_158);  //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_198);  //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_218);  //Sent by BMS or SMA?
    ESP32Can.CANWriteFrame(&SMA_560);  //Sent by BMS or SMA?
  }

  // Irregular
  // 0x720 sent every 30seconds, only 3x
}
#endif
