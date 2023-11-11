#include "RENAULT-ZOE-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Do not change code below unless you are sure what you are doing */
#define LB_MAX_SOC 1000             //BMS never goes over this value. We use this info to rescale SOC% sent to Fronius
#define LB_MIN_SOC 0                //BMS never goes below this value. We use this info to rescale SOC% sent to Fronius
const int rx_queue_size = 10;       // Receive Queue size
static uint8_t CANstillAlive = 12;  //counter for checking if CAN is still alive
static uint8_t errorCode = 0;       //stores if we have an error code active from battery control logic
static int16_t LB_SOC = 0;
static int16_t LB_SOH = 0;
static int16_t LB_MIN_TEMPERATURE = 0;
static int16_t LB_MAX_TEMPERATURE = 0;
static uint16_t LB_Discharge_Power_Limit = 0;
static uint32_t LB_Discharge_Power_Limit_Watts = 0;
static uint16_t LB_Charge_Power_Limit = 0;
static uint32_t LB_Charge_Power_Limit_Watts = 0;

CAN_frame_t ZOE_423 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x423,
                       .data = {0x33, 0x00, 0xFF, 0xFF, 0x00, 0xE0, 0x00, 0x00}};

static unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
static const int interval10 = 10;            // interval (ms) at which send CAN Messages
static const int interval100 = 100;          // interval (ms) at which send CAN Messages
static int BMSPollCounter = 0;

void update_values_zoe_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE;              //Startout in active mode

  StateOfHealth = (LB_SOH * 100);  //Increase range from 99% -> 99.00%

  //Calculate the SOC% value to send to Fronius
  LB_SOC = LB_MIN_SOC + (LB_MAX_SOC - LB_MIN_SOC) * (LB_SOC - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE);
  if (LB_SOC < 0) {  //We are in the real SOC% range of 0-20%, always set SOC sent to Fronius as 0%
    LB_SOC = 0;
  }
  if (LB_SOC > 1000) {  //We are in the real SOC% range of 80-100%, always set SOC sent to Fronius as 100%
    LB_SOC = 1000;
  }
  SOC = (LB_SOC * 10);  //increase LB_SOC range from 0-100.0 -> 100.00

  battery_voltage;
  battery_current;
  capacity_Wh = BATTERY_WH_MAX;
  remaining_capacity_Wh;

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

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

  stat_batt_power;
  temperature_min;
  temperature_max;

#ifdef DEBUG_VIA_USB
  Serial.print("BMS Status (3=OK): ");
  Serial.println(bms_status);
  Serial.print("Max discharge power: ");
  Serial.println(max_target_discharge_power);
  Serial.print("Max charge power: ");
  Serial.println(max_target_charge_power);
  Serial.print("SOH%: ");
  Serial.println(LB_SOH);
  Serial.print("SOH% to Fronius: ");
  Serial.println(StateOfHealth);
  Serial.print("LB_SOC: ");
  Serial.println(LB_SOC);
  Serial.print("SOC% to Fronius: ");
  Serial.println(SOC);
  Serial.print("Temperature Min: ");
  Serial.println(temperature_min);
  Serial.print("Temperature Max: ");
  Serial.println(temperature_max);
#endif
}

void receive_can_zoe_battery(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0x155:            //BMS1
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      //LB_Max_Charge_Amps =
      //LB_Current = (((rx_frame.data.u8[1] & 0xF8) << 5) | (rx_frame.data.u8[2]));
      LB_SOC = ((rx_frame.data.u8[4] << 8) | (rx_frame.data.u8[5]));
      break;
    case 0x424:  //BMS2
      LB_Charge_Power_Limit = (rx_frame.data.u8[2]);
      LB_Discharge_Power_Limit = (rx_frame.data.u8[3]);
      LB_SOH = (rx_frame.data.u8[5]);
      LB_MIN_TEMPERATURE = ((rx_frame.data.u8[4] & 0x7F) - 40);
      LB_MAX_TEMPERATURE = ((rx_frame.data.u8[7] & 0x7F) - 40);
      break;
    case 0x425:  //BMS3 (could also be 445?)
      //LB_kWh_Remaining =
      //LB_Cell_Max_Voltage =
      //LB_Cell_Min_Voltage =
      break;
    default:
      break;
  }
}
void send_can_zoe_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;
    BMSPollCounter++;  //GKOE

    if (BMSPollCounter < 46)  //GKOE
    {  //The Kangoo batteries (also Zoe?) dont like getting this message continously, so we pause after 4.6s, and resume after 40s
      ESP32Can.CANWriteFrame(&ZOE_423);  //Send 0x423 to keep BMS happy
    }
    if (BMSPollCounter > 400)  //GKOE
    {
      BMSPollCounter = 0;
    }
  }
}
