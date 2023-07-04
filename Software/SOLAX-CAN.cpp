#include "SOLAX-CAN.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis2s = 0; // will store last time a 2s CAN Message was send
static unsigned long previousMillis10s = 0; // will store last time a 10s CAN Message was send
static unsigned long previousMillis60s = 0; // will store last time a 60s CAN Message was send
static const int interval2s = 2000; // interval (ms) at which send CAN Messages
static const int interval10s = 10000; // interval (ms) at which send CAN Messages
static const int interval60s = 60000; // interval (ms) at which send CAN Messages

CAN_frame_t SOLAX_1872 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1872,.data = {0x03, 0x16, 0x00, 0x66, 0x00, 0x33, 0x02, 0x09}};

void update_values_can_solax()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages

}

void send_can_solax()
{
  unsigned long currentMillis = millis();
	// Send 2s CAN Message
	if (currentMillis - previousMillis2s >= interval2s)
	{
		previousMillis2s = currentMillis;

	}
  // Send 10s CAN Message
	if (currentMillis - previousMillis10s >= interval10s)
	{
		previousMillis10s = currentMillis;

    //Serial.println("CAN 10s done");
	}
  //Send 60s message
	if (currentMillis - previousMillis60s >= interval60s)
	{ 
		previousMillis60s = currentMillis;

    //ESP32Can.CANWriteFrame(&BYD_190); 
		//Serial.println("CAN 60s done");
	}
}

void receive_can_solax(CAN_frame_t rx_frame)
{
  Serial.println("Inverter sending CAN message");
}