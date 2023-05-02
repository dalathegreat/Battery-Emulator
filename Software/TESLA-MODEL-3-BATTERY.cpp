#include "TESLA-MODEL-3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* User definable settings for the Tesla Model 3 battery */


/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
static const int interval10 = 10; // interval (ms) at which send CAN Messages
static const int interval100 = 100; // interval (ms) at which send CAN Messages
static uint8_t stillAliveCAN = 6; //counter for checking if CAN is still alive

void update_values_tesla_model_3_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
	StateOfHealth; 
	
	SOC;

	battery_voltage;

  battery_current;

	capacity_Wh;

	remaining_capacity_Wh;

	max_target_discharge_power;

	max_target_charge_power;
	
	stat_batt_power;

	temperature_min; 

	temperature_max;

	/* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
	if(!stillAliveCAN)
	{
	bms_status = FAULT;
	Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
	}
	else
	{
	stillAliveCAN--;
	}
}

void handle_can_tesla_model_3_battery()
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
			case 0xABC:
				//Handle content
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
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;

		//ESP32Can.CANWriteFrame(&message100);

	}
	//Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;

		//ESP32Can.CANWriteFrame(&message10); 
	}
}
