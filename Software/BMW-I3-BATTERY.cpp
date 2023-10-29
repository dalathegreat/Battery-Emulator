#include "BMW-I3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

//TODO before using
// Map the final values in update_values_i3_battery, set some to static values if not available (current, discharge max , charge max)
// Check if I3 battery stays alive with only 10B and 512. If not, add 12F. If that doesn't help, add more from CAN log (ask Dala)

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20 = 0; // will store last time a 20ms CAN Message was send
static unsigned long previousMillis600 = 0; // will store last time a 600ms CAN Message was send
static const int interval20 = 20; // interval (ms) at which send CAN Messages
static const int interval600 = 600; // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0   //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

CAN_frame_t BMW_10B = {.FIR = {.B = {.DLC = 3,.FF = CAN_frame_std,}},.MsgID = 0x10B,.data = {0xCD, 0x01, 0xFC}};
CAN_frame_t BMW_512 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x512,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}}; //0x512  Network management edme   VCU
//CAN_frame_t BMW_12F //Might be needed,    Wake up ,VCU 100ms
//These CAN messages need to be sent towards the battery to keep it alive

static const uint8_t BMW_10B_0[15] = {0xCD,0x19,0x94,0x6D,0xE0,0x34,0x78,0xDB,0x97,0x43,0x0F,0xF6,0xBA,0x6E,0x81};
static const uint8_t BMW_10B_1[15] = {0x01,0x02,0x33,0x34,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x00};
static uint8_t BMW_10B_counter = 0;

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

void update_values_i3_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE; //Startout in active mode

  //Calculate the SOC% value to send to inverter
  Calculated_SOC = (Display_SOC * 10); //Increase decimal amount
  Calculated_SOC = LB_MIN_SOC + (LB_MAX_SOC - LB_MIN_SOC) * (Calculated_SOC - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE); 
  if (Calculated_SOC < 0)
  { //We are in the real SOC% range of 0-20%, always set SOC sent to inverter as 0%
      Calculated_SOC = 0;
  }
  if (Calculated_SOC > 1000)
  { //We are in the real SOC% range of 80-100%, always set SOC sent to inverter as 100%
      Calculated_SOC = 1000;
  }
  SOC = (Calculated_SOC * 10); //increase LB_SOC range from 0-100.0 -> 100.00

  battery_voltage = Battery_Volts; //Unit V+1 (5000 = 500.0V)

  battery_current = Battery_Current;

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh = (Battery_Capacity_kWh * 1000);

  if(SOC > 9900) //If Soc is over 99%, stop charging
  {
    max_target_charge_power = 0;
  }
  else
  {
    max_target_charge_power = 5000; //Hardcoded value for testing. TODO, read real value from battery when discovered
  }

  if(SOC < 500) //If Soc is under 5%, stop dicharging
  {
    max_target_discharge_power = 0;
  }
  else
  {
    max_target_discharge_power = 5000; //Hardcoded value for testing. TODO, read real value from battery when discovered
  }

  Battery_Power = (Battery_Current * (Battery_Volts/10)); 

  stat_batt_power = Battery_Power; //TODO, is mapping OK?

  temperature_min; //hardcoded to 5*C in startup, TODO, find from battery CAN later
  
  temperature_max; //hardcoded to 6*C in startup, TODO, find from battery CAN later
  
  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if(!CANstillAlive)
  {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  }
  else
  {
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

void receive_can_i3_battery(CAN_frame_t rx_frame)
{
  CANstillAlive = 12;
  switch (rx_frame.MsgID)
  {
  case 0x431: //Battery capacity [200ms]
    Battery_Capacity_kWh = (((rx_frame.data.u8[1] & 0x0F) << 8 | rx_frame.data.u8[5])) / 50;
  break;
  case 0x432: //SOC% charged [200ms]
    Voltage_Setpoint = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
    Low_SOC = (rx_frame.data.u8[2] / 2);
    High_SOC = (rx_frame.data.u8[3] / 2);
    Display_SOC = (rx_frame.data.u8[4] / 2);
  break;
  case 0x112: //BMS status [10ms]
    CANstillAlive = 12;
    Battery_Current = ((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) / 10) - 819; //Amps
    Battery_Volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]); //500.0 V
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
void send_can_i3_battery()
{
  unsigned long currentMillis = millis();
  // Send 600ms CAN Message
  if (currentMillis - previousMillis600 >= interval600)
  {
    previousMillis600 = currentMillis;
    
    ESP32Can.CANWriteFrame(&BMW_512); 
  }
  //Send 20ms message
  if (currentMillis - previousMillis20 >= interval20)
  { 
    previousMillis20 = currentMillis;
    
    BMW_10B.data.u8[0] = BMW_10B_0[BMW_10B_counter];
    BMW_10B.data.u8[1] = BMW_10B_1[BMW_10B_counter];
    BMW_10B_counter++;
    if(BMW_10B_counter > 14)
    {
      BMW_10B_counter = 0;
    }
    
    ESP32Can.CANWriteFrame(&BMW_10B); 
  }
}
