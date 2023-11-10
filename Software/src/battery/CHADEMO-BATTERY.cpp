#include "CHADEMO-BATTERY.h"
#include "../devboard/can/ESP32CAN.h"
#include "../lib/ThomasBarth-ESP32-CAN-Driver/CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
static const int interval100 = 100;          // interval (ms) at which send CAN Messages
const int rx_queue_size = 10;                // Receive Queue size
static uint8_t CANstillAlive = 12;           //counter for checking if CAN is still alive
static uint8_t errorCode = 0;                //stores if we have an error code active from battery control logic

CAN_frame_t CHADEMO_108 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x108,
                           .data = {0x01, 0xF4, 0x01, 0x0F, 0xB3, 0x01, 0x00, 0x00}};
CAN_frame_t CHADEMO_109 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x109,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x01, 0x20, 0xFF, 0xFF}};
//For chademo v2.0 only
CAN_frame_t CHADEMO_118 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x118,
                           .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame_t CHADEMO_208 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x208,
                           .data = {0xFF, 0xF4, 0x01, 0xF0, 0x00, 0x00, 0xFA, 0x00}};
CAN_frame_t CHADEMO_209 = {.FIR = {.B =
                                       {
                                           .DLC = 8,
                                           .FF = CAN_frame_std,
                                       }},
                           .MsgID = 0x209,
                           .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

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
uint8_t DynamicControlStatus = 0;
uint8_t HighCurrentControlStatus = 0;
uint8_t HighVoltageControlStatus = 0;

void update_values_chademo_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for the inverter
  bms_status = ACTIVE;  //Startout in active mode

  SOC = ChargingRate;

  max_target_discharge_power = (MaximumDischargeCurrent * MaximumBatteryVoltage);  //In Watts, Convert A to P

  battery_voltage = TargetBatteryVoltage;  //Todo, scaling?

  capacity_Wh = ((RatedBatteryCapacity / 0.11) *
                 1000);  //(Added in CHAdeMO v1.0.1), maybe handle hardcoded on lower protocol version?

  remaining_capacity_Wh = (SOC / 100) * capacity_Wh;

  /* Check if the Vehicle is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    errorCode = 7;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

#ifdef DEBUG_VIA_USB
  if (errorCode > 0) {
    Serial.print("ERROR CODE ACTIVE IN SYSTEM. NUMBER: ");
    Serial.println(errorCode);
  }
  Serial.print("BMS Status (3=OK): ");
  Serial.println(bms_status);
  switch (bms_char_dis_status) {
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
#endif
}

void receive_can_chademo_battery(CAN_frame_t rx_frame) {
  CANstillAlive == 12;  //We are getting CAN messages from the vehicle, inform the watchdog

  switch (rx_frame.MsgID) {
    case 0x100:
      MinimumChargeCurrent = rx_frame.data.u8[0];
      MinumumBatteryVoltage = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      MaximumBatteryVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      ConstantOfChargingRateIndication = rx_frame.data.u8[6];
      break;
    case 0x101:
      MaxChargingTime10sBit = rx_frame.data.u8[1];
      MaxChargingTime1minBit = rx_frame.data.u8[2];
      EstimatedChargingTime = rx_frame.data.u8[3];
      RatedBatteryCapacity = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
      break;
    case 0x102:
      ControlProtocolNumberEV = rx_frame.data.u8[0];
      TargetBatteryVoltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
      ChargingCurrentRequest = rx_frame.data.u8[3];
      FaultBatteryOvervoltage = (rx_frame.data.u8[4] & 0x01);
      FaultBatteryUndervoltage = (rx_frame.data.u8[4] & 0x02) >> 1;
      FaultBatteryCurrentDeviation = (rx_frame.data.u8[4] & 0x04) >> 2;
      FaultHighBatteryTemperature = (rx_frame.data.u8[4] & 0x08) >> 3;
      FaultBatteryVoltageDeviation = (rx_frame.data.u8[4] & 0x10) >> 4;
      StatusVehicleCharging = (rx_frame.data.u8[5] & 0x01);
      StatusVehicleShifterPosition = (rx_frame.data.u8[5] & 0x02) >> 1;
      StatusChargingSystem = (rx_frame.data.u8[5] & 0x04) >> 2;
      StatusVehicle = (rx_frame.data.u8[5] & 0x08) >> 3;
      StatusNormalStopRequest = (rx_frame.data.u8[5] & 0x10) >> 4;
      ChargingRate = rx_frame.data.u8[6];
      break;
    case 0x200:  //For V2X
      MaximumDischargeCurrent = rx_frame.data.u8[0];
      MinimumDischargeVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      MinimumBatteryDischargeLevel = rx_frame.data.u8[6];
      MaxRemainingCapacityForCharging = rx_frame.data.u8[7];
      break;
    case 0x201:  //For V2X
      V2HchargeDischargeSequenceNum = rx_frame.data.u8[0];
      ApproxDischargeCompletionTime = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);
      AvailableVehicleEnergy = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
      break;
    case 0x700:
      AutomakerCode = rx_frame.data.u8[0];
      OptionalContent =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2]);  //Actually more bytes, but not needed for our purpose
      break;
    case 0x110:  //Only present on Chademo v2.0
      DynamicControlStatus = (rx_frame.data.u8[0] & 0x01);
      HighCurrentControlStatus = (rx_frame.data.u8[0] & 0x02) >> 1;
      HighVoltageControlStatus = (rx_frame.data.u8[0] & 0x04) >> 2;
    default:
      break;
  }
}
void send_can_chademo_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

    ESP32Can.CANWriteFrame(&CHADEMO_108);
    ESP32Can.CANWriteFrame(&CHADEMO_109);
    ESP32Can.CANWriteFrame(&CHADEMO_208);
    ESP32Can.CANWriteFrame(&CHADEMO_209);

    if (ControlProtocolNumberEV >= 0x03) {  //Only send the following on Chademo 2.0 vehicles?
      ESP32Can.CANWriteFrame(&CHADEMO_118);
    }
  }
}
