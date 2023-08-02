#include "BMW-I3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis20 = 0; // will store last time a 20ms CAN Message was send
static unsigned long previousMillis600 = 0; // will store last time a 600ms CAN Message was send
static const int interval20 = 20; // interval (ms) at which send CAN Messages
static const int interval600 = 600; // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0   //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

CAN_frame_t BMW_10B = {.FIR = {.B = {.DLC = 3,.FF = CAN_frame_std,}},.MsgID = 0x10B,.data = {0xCD, 0x01, 0xFC}};
CAN_frame_t BMW_512 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x512,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}};
//These CAN messages need to be sent towards the battery to keep it alive

static const uint8_t BMW_10B_0[15] = {0xCD,0x19,0x94,0x6D,0xE0,0x34,0x78,0xDB,0x97,0x43,0x0F,0xF6,0xBA,0x6E,0x81};
static const uint8_t BMW_10B_1[15] = {0x01,0x02,0x33,0x34,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x00};
static uint8_t BMW_10B_counter = 0;


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
  	case 0x431: //Battery capacity
	break;
	case 0x432: //SOC% charged
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
