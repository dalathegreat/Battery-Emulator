#include "RENAULT-ZOE-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
const int rx_queue_size = 10; // Receive Queue size
uint16_t CANerror = 0; //counter on how many CAN errors encountered
uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 
uint8_t errorCode = 0; //stores if we have an error code active from battery control logic

static int16_t LB_SOC = 0;
static int16_t LB_SOH = 0;
static int16_t LB_TEMPERATURE = 0;

void update_values_zoe_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE; //Startout in active mode
  
  StateOfHealth;

  //Calculate the SOC% value to send to Fronius
  LB_SOC = LB_MIN_SOC + (LB_MAX_SOC - LB_MIN_SOC) * (LB_SOC - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE); 
  if (LB_SOC < 0)
  { //We are in the real SOC% range of 0-20%, always set SOC sent to Fronius as 0%
      LB_SOC = 0;
  }
  if (LB_SOC > 1000)
  { //We are in the real SOC% range of 80-100%, always set SOC sent to Fronius as 100%
      LB_SOC = 1000;
  }
  SOC = (LB_SOC * 10); //increase LB_SOC range from 0-100.0 -> 100.00

  battery_voltage;
  battery_current;
	capacity_Wh;
	remaining_capacity_Wh;

  /* Define power able to be discharged from battery */
  if(LB_Discharge_Power_Limit > 30) //if >30kW can be pulled from battery
  {
    max_target_discharge_power = 30000; //cap value so we don't go over the Fronius limits
  }
  else
  {
    max_target_discharge_power = (LB_Discharge_Power_Limit * 1000); //kW to W
  }
  if(SOC == 0) //Scaled SOC% value is 0.00%, we should not discharge battery further
  {
    max_target_discharge_power = 0;
  }
	
  /* Define power able to be put into the battery */
  if(LB_Charge_Power_Limit > 30) //if >30kW can be put into the battery
  {
    max_target_charge_power = 30000; //cap value so we don't go over the Fronius limits
  }
  if(LB_Charge_Power_Limit < 0) //LB_MAX_POWER_FOR_CHARGER can actually go to -10kW
  {
    max_target_charge_power = 0; //cap calue so we dont do under the Fronius limits
  }
  else
  {
    max_target_charge_power = (LB_Charge_Power_Limit * 1000); //kW to W
  }
  if(SOC == 10000) //Scaled SOC% value is 100.00%
  {
    max_target_charge_power = 0; //No need to charge further, set max power to 0
  }
  

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
	
  stat_batt_power;
	temperature_min;
	temperature_max;

  if(printValues)
  {  //values heading towards the modbus registers
    Serial.print("BMS Status (3=OK): ");
    Serial.println(bms_status);
    Serial.print("Max discharge power: ");
    Serial.println(max_target_discharge_power);
    Serial.print("Max charge power: ");
    Serial.println(max_target_charge_power);
    Serial.print("SOH%: ");
    Serial.println(StateOfHealth);
    Serial.print("SOC% to Fronius: ");
    Serial.println(SOC);
    Serial.print("Temperature Min: ");
    Serial.println(temperature_min);
    Serial.print("Temperature Max: ");
    Serial.println(temperature_max);
  }
}

void handle_can_zoe_battery()
{
  CAN_frame_t rx_frame;
  unsigned long currentMillis = millis();

  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
    if (rx_frame.FIR.B.FF == CAN_frame_std)
    {
      //printf("New standard frame");
      switch (rx_frame.MsgID)
			{
      case 0x654:
        CANstillAlive = 12; //Indicate that we are still getting CAN messages from the BMS
        LB_SOC = rx_frame.data.u8[4];
        break;
      case 0x658:
        LB_SOH = (rx_frame.data.u8[4] & 0x7F);
        break;
      case 0x42E:
        LB_TEMPERATURE = ((rx_frame.data.u8[5] & 0x7F) - 40);
        break;
      default:
				break;
      }      
    }
    else
    {
      //printf("New extended frame");
    }
  }
}

uint16_t convert2unsignedint16(uint16_t signed_value)
{
	if(signed_value < 0)
	{
		return(65535 + signed_value);
	}
	else
	{
		return signed_value;
	}
}
