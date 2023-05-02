#include "TESLA-MODEL-3-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
static unsigned long previousMillis30 = 0; // will store last time a 30ms CAN Message was send
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
static const int interval10 = 10; // interval (ms) at which send CAN Messages
static const int interval30 = 30; // interval (ms) at which send CAN Messages
static const int interval100 = 100; // interval (ms) at which send CAN Messages
static uint8_t stillAliveCAN = 6; //counter for checking if CAN is still alive

CAN_frame_t TESLA_221 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x221,.data = {0x40, 0x41, 0x05, 0x15, 0x00, 0x50, 0x71, 0x7f}};

uint16_t volts = 0;
uint16_t amps = 0;
int16_t max_temp = 0;
int16_t min_temp = 0;

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
  static int mux = 0;
  static int temp = 0;

  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
	if (rx_frame.FIR.B.FF == CAN_frame_std)
	{
    stillAliveCAN = 6; //We got CAN-messages flowing in!
		//printf("New standard frame");
		switch (rx_frame.MsgID)
			{
			case 0x352:
        //SOC
				break;
      case 0x20A:
        //Contactor state
        break;
      case 0x132:
        //battery amps/volts 
        volts = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) * 0.01;
        amps = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
        if (amps > 32768)
        {
          amps = - (65535 - amps);
        }
        amps = amps * 0.1;

        break;
      case 0x3D2:
        // total charge/discharge kwh 
        break;
      case 0x332:
        //min/max hist values
        mux = rx_frame.data.u8[0];
        mux = mux & 0x03;

        if(mux == 1) //Cell voltages
        {
          //todo handle cell voltages
        }
        if(mux == 0)//Temperature sensors
        {
          temp = rx_frame.data.u8[2];
          max_temp = (temp * 0.5) - 40; //in celcius

          temp = rx_frame.data.u8[3];
          min_temp = (volts * 0.5) - 40; //in celcius
        }
        break;
      case 0x401:
        //cell voltages 
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
  //Send 30ms message
	if (currentMillis - previousMillis30 >= interval30)
	{ 
		previousMillis10 = currentMillis;

    //Command contactor to close
		ESP32Can.CANWriteFrame(&TESLA_221);
	}
	//Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;

		//ESP32Can.CANWriteFrame(&message10); 
	}
}
