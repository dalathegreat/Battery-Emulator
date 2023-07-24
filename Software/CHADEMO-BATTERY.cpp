#include "CHADEMO-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
static const int interval100 = 100; // interval (ms) at which send CAN Messages
const int rx_queue_size = 10; // Receive Queue size
uint16_t CANerror = 0; //counter on how many CAN errors encountered
static uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 
static uint8_t errorCode = 0; //stores if we have an error code active from battery control logic

CAN_frame_t CHADEMO_108 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x108,.data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame_t CHADEMO_109 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x109,.data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
//For chademo v2.0 only
CAN_frame_t CHADEMO_118 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x118,.data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame_t CHADEMO_208 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x208,.data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame_t CHADEMO_209 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x209,.data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};

//H100
uint8_t MinimumChargeCurrent = 0;
uint16_t MinumumBatteryVoltage = 0;
uint16_t MaximumBatteryVoltage = 0;
uint8_t ConstantOfChargingRateIndication = 0;
//H101
uint8_t MaxChargingTime10sBit = 0;
uint8_t MaxChargingTime1minBit = 0;
uint8_t EstimatedChargingTime = 0;
uint16_t RatedBatteryCapacity = 0;
//H102
uint8_t ControlProtocolNumberEV = 0;
uint16_t TargetBatteryVoltage = 0;
uint8_t ChargingCurrentRequest = 0;
uint8_t FaultBatteryVoltageDeviation = 0;
uint8_t FaultHighBatteryTemperature = 0;
uint8_t FaultBatteryCurrentDeviation = 0;
uint8_t FaultBatteryUndervoltage = 0;
uint8_t FaultBatteryOvervoltage = 0;
uint8_t StatusNormalStopRequest = 0;
uint8_t StatusVehicle = 0;
uint8_t StatusChargingSystem = 0;
uint8_t StatusVehicleShifterPosition = 0;
uint8_t StatusVehicleCharging = 0;
uint8_t ChargingRate = 0;
//H200
uint8_t MaximumDischargeCurrent = 0;
uint16_t MinimumDischargeVoltage = 0;
uint8_t MinimumBatteryDischargeLevel = 0;
uint8_t MaxRemainingCapacityForCharging = 0;
//H201
uint8_t V2HchargeDischargeSequenceNum = 0;
uint16_t ApproxDischargeCompletionTime = 0;
uint16_t AvailableVehicleEnergy = 0;
//H700
uint8_t AutomakerCode = 0;
uint32_t OptionalContent = 0;
//H118
uint8_t DynamicControl = 0;
uint8_t HighCurrentControl = 0;
uint8_t HighVoltageControl = 0;


void update_values_chademo_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for the inverter
  bms_status = ACTIVE; //Startout in active mode
  
 
  /* Check if the Vehicle is still sending CAN messages. If we go 60s without messages we raise an error*/
  if(!CANstillAlive)
  {
    bms_status = FAULT;
    errorCode = 7;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  }
  else
  {
    CANstillAlive--;
  }

  if(printValues)
  {  //values heading towards the modbus registers
    if(errorCode > 0)
      {
        Serial.print("ERROR CODE ACTIVE IN SYSTEM. NUMBER: ");
        Serial.println(errorCode);
      }
    Serial.print("BMS Status (3=OK): ");
    Serial.println(bms_status);
    switch (bms_char_dis_status)
			{
      case 0:
        Serial.println("Battery Idle");
        break;
      case 1:
        Serial.println("Battery Discharging");
        break;
      case 2:
        Serial.println("Battery Charging");
        break;
      default:
        break;
      }
    Serial.print("Power: ");
    Serial.println(LB_Power);
    Serial.print("Max discharge power: ");
    Serial.println(max_target_discharge_power);
    Serial.print("Max charge power: ");
    Serial.println(max_target_charge_power);
    Serial.print("SOH%: ");
    Serial.println(StateOfHealth);
    Serial.print("SOC% to Inverter: ");
    Serial.println(SOC);
    Serial.print("Temperature Min: ");
    Serial.println(temperature_min);
    Serial.print("Temperature Max: ");
    Serial.println(temperature_max);
  }
}

void receive_can_chademo_battery(CAN_frame_t rx_frame)
{
  CANstillAlive == 12; //We are getting CAN messages, inform the watchdog

  switch (rx_frame.MsgID)
  {
  case 0x100:
    break;
  default:
    break;
  }      
}
void send_can_chademo_battery()
{
  unsigned long currentMillis = millis();
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;

		ESP32Can.CANWriteFrame(&LEAF_50B); 
	}
}
