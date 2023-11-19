#include "RENAULT-ZOE-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Do not change code below unless you are sure what you are doing */
#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Fronius
#define LB_MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Fronius

static uint8_t CANstillAlive = 12;  //counter for checking if CAN is still alive
static uint8_t errorCode = 0;       //stores if we have an error code active from battery control logic
static uint16_t LB_SOC = 0;
static uint16_t soc_calculated = 0;
static uint16_t LB_SOH = 0;
static int16_t LB_MIN_TEMPERATURE = 0;
static int16_t LB_MAX_TEMPERATURE = 0;
static uint16_t LB_Discharge_Power_Limit = 0;
static uint32_t LB_Discharge_Power_Limit_Watts = 0;
static uint16_t LB_Charge_Power_Limit = 0;
static uint32_t LB_Charge_Power_Limit_Watts = 0;
static int32_t LB_Current = 0;
static uint16_t LB_kWh_Remaining = 0;
static uint16_t LB_Cell_Max_Voltage = 3700;
static uint16_t LB_Cell_Min_Voltage = 3700;
static uint16_t cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static uint32_t LB_Battery_Voltage = 3700;
static uint8_t LB_Discharge_Power_Limit_Byte1 = 0;
static bool GVB_79B_Continue = false;
static uint8_t GVI_Pollcounter = 0;

CAN_frame_t ZOE_423 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x423,
                       .data = {0x33, 0x00, 0xFF, 0xFF, 0x00, 0xE0, 0x00, 0x00}};
CAN_frame_t ZOE_79B = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x79B,
                       .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0xE0, 0x00, 0x00}};
CAN_frame_t ZOE_79B_Continue = {.FIR = {.B =
                                            {
                                                .DLC = 8,
                                                .FF = CAN_frame_std,
                                            }},
                                .MsgID = 0x79B,
                                .data = {0x030, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
static unsigned long GVL_pause = 0;
static const int interval10 = 10;      // interval (ms) at which send CAN Messages
static const int interval100 = 100;    // interval (ms) at which send CAN Messages
static const int interval1000 = 1000;  // interval (ms) at which send CAN Messages

void update_values_zoe_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE;              //Startout in active mode

  StateOfHealth = (LB_SOH * 100);  //Increase range from 99% -> 99.00%

  //Calculate the SOC% value to send to Fronius
  soc_calculated = LB_SOC;
  soc_calculated =
      LB_MIN_SOC + (LB_MAX_SOC - LB_MIN_SOC) * (soc_calculated - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE);
  if (soc_calculated < 0) {  //We are in the real SOC% range of 0-20%, always set SOC sent to Inverter as 0%
    soc_calculated = 0;
  }
  if (soc_calculated > 1000) {  //We are in the real SOC% range of 80-100%, always set SOC sent to Inverter as 100%
    soc_calculated = 1000;
  }
  SOC = (soc_calculated * 10);  //increase LB_SOC range from 0-100.0 -> 100.00

  battery_voltage = LB_Battery_Voltage;

  battery_current = LB_Current;

  capacity_Wh = BATTERY_WH_MAX;  //Hardcoded to header value

  remaining_capacity_Wh = (uint16_t)((SOC / 10000) * capacity_Wh);

  LB_Discharge_Power_Limit_Watts = (LB_Discharge_Power_Limit * 500);  //Convert value fetched from battery to watts
  /* Define power able to be discharged from battery */
  if (LB_Discharge_Power_Limit_Watts > 30000)  //if >30kW can be pulled from battery
  {
    max_target_discharge_power = 30000;  //cap value so we don't go over the Fronius limits
  } else {
    max_target_discharge_power = LB_Discharge_Power_Limit_Watts;
  }
  if (SOC == 0)  //Scaled SOC% value is 0.00%, we should not discharge battery further
  {
    max_target_discharge_power = 0;
  }

  LB_Charge_Power_Limit_Watts = (LB_Charge_Power_Limit * 500);  //Convert value fetched from battery to watts
  /* Define power able to be put into the battery */
  if (LB_Charge_Power_Limit_Watts > 30000)  //if >30kW can be put into the battery
  {
    max_target_charge_power = 30000;  //cap value so we don't go over the Fronius limits
  }
  if (LB_Charge_Power_Limit_Watts < 0) {
    max_target_charge_power = 0;  //cap calue so we dont do under the Fronius limits
  } else {
    max_target_charge_power = LB_Charge_Power_Limit_Watts;
  }
  if (SOC == 10000)  //Scaled SOC% value is 100.00%
  {
    max_target_charge_power = 0;  //No need to charge further, set max power to 0
  }

  stat_batt_power = (battery_voltage * LB_Current);  //TODO: check if scaling is OK

  temperature_min = convert2uint16(LB_MIN_TEMPERATURE * 10);

  temperature_max = convert2uint16(LB_MAX_TEMPERATURE * 10);

  cell_min_voltage = LB_Cell_Min_Voltage;

  cell_max_voltage = LB_Cell_Max_Voltage;

  cell_deviation_mV = (cell_max_voltage - cell_min_voltage);

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

  if (LB_Cell_Max_Voltage >= ABSOLUTE_CELL_MAX_VOLTAGE) {
    bms_status = FAULT;
    Serial.println("ERROR: CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
  }
  if (LB_Cell_Min_Voltage <= ABSOLUTE_CELL_MIN_VOLTAGE) {
    bms_status = FAULT;
    Serial.println("ERROR: CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
  }
  if (cell_deviation_mV > MAX_CELL_DEVIATION_MV) {
    LEDcolor = YELLOW;
    Serial.println("ERROR: HIGH CELL mV DEVIATION!!! Inspect battery!");
  }

#ifdef DEBUG_VIA_USB
  Serial.println("Values going to inverter:");
  Serial.print("SOH%: ");
  Serial.print(StateOfHealth);
  Serial.print(", SOC% scaled: ");
  Serial.print(SOC);
  Serial.print(", Voltage: ");
  Serial.print(battery_voltage);
  Serial.print(", Max discharge power: ");
  Serial.print(max_target_discharge_power);
  Serial.print(", Max charge power: ");
  Serial.print(max_target_charge_power);
  Serial.print(", Max temp: ");
  Serial.print(temperature_max);
  Serial.print(", Min temp: ");
  Serial.print(temperature_min);
  Serial.print(", BMS Status (3=OK): ");
  Serial.print(bms_status);

  Serial.println("Battery values: ");
  Serial.print("Real SOC: ");
  Serial.print(LB_SOC);
  Serial.print(", Current: ");
  Serial.print(LB_Current);
  Serial.print(", kWh remain: ");
  Serial.print(LB_kWh_Remaining);
  Serial.print(", max mV: ");
  Serial.print(LB_Cell_Max_Voltage);
  Serial.print(", min mV: ");
  Serial.print(LB_Cell_Min_Voltage);

#endif
}

void receive_can_zoe_battery(CAN_frame_t rx_frame)  //GKOE reworked
{

  switch (rx_frame.MsgID) {
    case 0x155:            //BMS1
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_Current = word((rx_frame.data.u8[1] & 0xF), rx_frame.data.u8[2]) * 0.25 - 500;  //OK!

      LB_SOC = ((rx_frame.data.u8[4] << 8) | (rx_frame.data.u8[5])) * 0.0025;  //OK!
      break;

    case 0x424:            //BMS2
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_SOH = (rx_frame.data.u8[5]);
      LB_MIN_TEMPERATURE = ((rx_frame.data.u8[4]) - 40);  //OK!
      LB_MAX_TEMPERATURE = ((rx_frame.data.u8[7]) - 40);  //OK!
      break;

    case 0x425:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_kWh_Remaining = word((rx_frame.data.u8[0] & 0x1), rx_frame.data.u8[1]) / 10;  //OK!
      break;

    case 0x445:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_Cell_Max_Voltage = 1000 + word((rx_frame.data.u8[3] & 0x1), rx_frame.data.u8[4]) * 10;  //OK!
      LB_Cell_Min_Voltage = 1000 + (word(rx_frame.data.u8[5], rx_frame.data.u8[6]) >> 7) * 10;   //OK!

      if ((LB_Cell_Max_Voltage == 6110) or (LB_Cell_Min_Voltage == 6110)) {  //Read Error
        LB_Cell_Max_Voltage = 3880;
        LB_Cell_Min_Voltage = 3880;
        break;
      }

      // LB_Battery_Voltage = (LB_Cell_Max_Voltage * 80 + LB_Cell_Min_Voltage * 20) / 100 * 96; // GKOE just as long as we don't have the real pack voltage... ?
      break;
    case 0x7BB:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS

      if (rx_frame.data.u8[0] == 0x10) {  //1st response Bytes 0-7
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x21) {  //2nd response Bytes 8-15
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x22) {  //3rd response Bytes 16-23
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x23) {                                               //4th response Bytes 16-23
        LB_Charge_Power_Limit = word(rx_frame.data.u8[5], rx_frame.data.u8[6]) * 100;  //OK!
        LB_Discharge_Power_Limit_Byte1 = rx_frame.data.u8[7];
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x24) {  //5th response Bytes 24-31
        LB_Discharge_Power_Limit = word(LB_Discharge_Power_Limit_Byte1, rx_frame.data.u8[1]) * 100;  //OK!
        LB_Battery_Voltage = word(rx_frame.data.u8[2], rx_frame.data.u8[3]) * 10;                    //OK!
        GVB_79B_Continue = false;
      }
      break;
    default:
      break;
  }
}

void send_can_zoe_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message (for 2.4s, then pause 10s)
  if ((currentMillis - previousMillis100) >= (interval100 + GVL_pause)) {
    previousMillis100 = currentMillis;
    ESP32Can.CANWriteFrame(&ZOE_423);
    GVI_Pollcounter++;
    GVL_pause = 0;
    if (GVI_Pollcounter >= 24) {
      GVI_Pollcounter = 0;
      GVL_pause = 10000;
    }
  }
  // 1000ms CAN handling
  if (currentMillis - previousMillis1000 >= interval1000) {
    previousMillis1000 = currentMillis;
    if (GVB_79B_Continue)
      ESP32Can.CANWriteFrame(&ZOE_79B_Continue);
  } else {
    ESP32Can.CANWriteFrame(&ZOE_79B);
  }
}

uint16_t convert2uint16(int16_t signed_value) {
  if (signed_value < 0) {
    return (65535 + signed_value);
  } else {
    return (uint16_t)signed_value;
  }
}
