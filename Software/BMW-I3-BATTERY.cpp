#include "BMW-I3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
static const int interval10 = 10; // interval (ms) at which send CAN Messages
static const int interval100 = 100; // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0   //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

static int BMS_SOC = 0;

void update_values_i3_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
	bms_status = ACTIVE; //Startout in active mode

	SOC;

	battery_voltage;

	battery_current;

	capacity_Wh = BATTERY_WH_MAX;

	remaining_capacity_Wh;

	max_target_discharge_power;

	max_target_charge_power;

	stat_batt_power;

	temperature_min;
	
	temperature_max;
	
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

  if(printValues)
  {  //values heading towards the inverter
    Serial.print("SOC%: ");
    Serial.println(BMS_SOC);
  }
}

void receive_can_i3_battery(CAN_frame_t rx_frame)
{
	CANstillAlive = 12;
	switch (rx_frame.MsgID)
	{
  	case 0x5D8:
	break;
	default:
	break;
	}      
}
void send_can_i3_battery()
{
  unsigned long currentMillis = millis();
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;

	}
  //Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;
	}
}
