#include "BATTERIES.h"
#ifdef RENAULT_ZOE_BATTERY
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "RENAULT-ZOE-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Fronius
#define LB_MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Fronius

static uint8_t CANstillAlive = 12;  //counter for checking if CAN is still alive
static uint8_t errorCode = 0;       //stores if we have an error code active from battery control logic
static uint16_t LB_SOC = 50;
static uint16_t soc_calculated = 0;
static uint16_t LB_SOH = 99;
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

CAN_frame_t ZOE_423 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x423,
                       .data = {0x33, 0x00, 0xFF, 0xFF, 0x00, 0xE0, 0x00, 0x00}};

static unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
static unsigned long GVL_pause = 0;
static const int interval10 = 10;      // interval (ms) at which send CAN Messages
static const int interval100 = 100;    // interval (ms) at which send CAN Messages
static const int interval1000 = 1000;  // interval (ms) at which send CAN Messages

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
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

  capacity_Wh = BATTERY_WH_MAX;  //Use the configured value to avoid overflows

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  remaining_capacity_Wh = static_cast<int>((static_cast<double>(SOC) / 10000) * BATTERY_WH_MAX);

  max_target_discharge_power;

  max_target_charge_power;

  stat_batt_power;

  temperature_min;

  temperature_max;

  cell_min_voltage;

  cell_max_voltage;

  cell_deviation_mV = (cell_max_voltage - cell_min_voltage);

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

  if (LB_Cell_Max_Voltage >= ABSOLUTE_CELL_MAX_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (LB_Cell_Min_Voltage <= ABSOLUTE_CELL_MIN_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }
  if (cell_deviation_mV > MAX_CELL_DEVIATION_MV) {
    set_event(EVENT_CELL_DEVIATION_HIGH, 0);
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
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

void receive_can_battery(CAN_frame_t rx_frame) {

  switch (rx_frame.MsgID) {
    case 0x42E:  //HV SOC & Battery Temp & Charging Power
      break;
    case 0x430:  //HVBatteryCoolingState & HVBatteryEvapTemp & HVBatteryEvapSetpoint
      break;
    case 0x432:  //BatVEShutDownAlert & HVBatCondPriorityLevel & HVBatteryLevelAlert & HVBatCondPriorityLevel & HVBatteryConditioningMode
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;
    //ESP32Can.CANWriteFrame(&ZOE_423);
  }
  // 1000ms CAN handling
  if (currentMillis - previousMillis1000 >= interval1000) {
    previousMillis1000 = currentMillis;
    //ESP32Can.CANWriteFrame(&ZOE_423);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  Serial.println("Renault Zoe battery selected");

  max_voltage = 4040;  // 404.0V, over this, charging is not possible (goes into forced discharge)
  min_voltage = 3100;  // 310.0V under this, discharging further is disabled
}

#endif
