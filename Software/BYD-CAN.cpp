#include "BYD-CAN.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
unsigned long previousMillis2s = 0; // will store last time a 2s CAN Message was send
unsigned long previousMillis10s = 0; // will store last time a 10s CAN Message was send
unsigned long previousMillis60s = 0; // will store last time a 60s CAN Message was send
const int interval2s = 2000; // interval (ms) at which send CAN Messages
const int interval10s = 10000; // interval (ms) at which send CAN Messages
const int interval60s = 60000; // interval (ms) at which send CAN Messages
const int rx_queue_size = 10; // Receive Queue size

const CAN_frame_t BYD_250 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x250,.data = {0x03, 0x16, 0x00, 0x66, 0x00, 0x33, 0x02, 0x09}};
const CAN_frame_t BYD_290 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x290,.data = {0x06, 0x37, 0x10, 0xD9, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t BYD_110 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x110,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

int charge_max_current = 0;

void update_values_can_byd()
{ //This function maps all the values fetched via CAN to the correct CAN messages
  charge_max_current = max_target_charge_power;
  BYD_110.data.u8[4] = (charge_max_current && 0x00FF) >> 8;
  BYD_110.data.u8[5] = 0xB4;
}

void handle_can_byd()
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
      case 0x151:
        if(rx_frame.data.u8[0] & 0x01)
        {
          send_intial_data();
        }
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
	// Send 2s CAN Message
	if (currentMillis - previousMillis2s >= interval2s)
	{
		previousMillis2s = currentMillis;

    ESP32Can.CANWriteFrame(&BYD_110);
	}
  //Send 60s message
	if (currentMillis - previousMillis60s >= interval60s)
	{ 
		previousMillis60s = currentMillis;

    //ESP32Can.CANWriteFrame(&LEAF_1D4); 
		//Serial.println("CAN 10ms done");
	}
}

void send_intial_data()
{
  ESP32Can.CANWriteFrame(&BYD_250);
  ESP32Can.CANWriteFrame(&BYD_290);
}
