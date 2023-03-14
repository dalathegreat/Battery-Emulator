#include "NISSAN-LEAF-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* User definable settings for the LEAF battery */
#define MAXPERCENTAGE 800   //800 = 80.0% , Max percentage the battery will charge to (App will show 100% once this value is reached)
#define MINPERCENTAGE 200   //200 = 20.0% , Min percentage the battery will discharge to (App will show 0% once this value is reached)
//#define INTERLOCK_REQUIRED //Uncomment this line to skip requiring both high voltage connectors to be seated on the LEAF battery
byte printValues = 1; //Should diagnostic values be printed to serial output? 

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
static unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
static const int interval10 = 10; // interval (ms) at which send CAN Messages
static const int interval100 = 100; // interval (ms) at which send CAN Messages
static uint8_t stillAlive = 6; //counter for checking if CAN is still alive 
static uint8_t mprun10r = 0; //counter 0-20 for 0x1F2 message
static byte mprun10 = 0; //counter 0-3
static byte mprun100 = 0; //counter 0-3

CAN_frame_t LEAF_1F2 = {.MsgID = 0x1F2, LEAF_1F2.FIR.B.DLC = 8, LEAF_1F2.FIR.B.FF = CAN_frame_std, LEAF_1F2.data.u8[0] = 0x10, LEAF_1F2.data.u8[1] = 0x64,LEAF_1F2.data.u8[2] = 0x00, LEAF_1F2.data.u8[3] = 0xB0,LEAF_1F2.data.u8[4] = 0x00,LEAF_1F2.data.u8[5] = 0x1E};
CAN_frame_t LEAF_50B = {.MsgID = 0x50B, LEAF_50B.FIR.B.DLC = 7, LEAF_50B.FIR.B.FF = CAN_frame_std, LEAF_50B.data.u8[0] = 0x00, LEAF_50B.data.u8[1] = 0x00,LEAF_50B.data.u8[2] = 0x06, LEAF_50B.data.u8[3] = 0xC0,LEAF_50B.data.u8[4] = 0x00,LEAF_50B.data.u8[5] = 0x00};
CAN_frame_t LEAF_50C = {.MsgID = 0x50C, LEAF_50C.FIR.B.DLC = 8, LEAF_50C.FIR.B.FF = CAN_frame_std, LEAF_50C.data.u8[0] = 0x00, LEAF_50C.data.u8[1] = 0x00,LEAF_50C.data.u8[2] = 0x00, LEAF_50C.data.u8[3] = 0x00,LEAF_50C.data.u8[4] = 0x00,LEAF_50C.data.u8[5] = 0x00};
CAN_frame_t LEAF_1D4 = {.MsgID = 0x1D4, LEAF_1D4.FIR.B.DLC = 8, LEAF_1D4.FIR.B.FF = CAN_frame_std, LEAF_1D4.data.u8[0] = 0x6E, LEAF_1D4.data.u8[1] = 0x6E,LEAF_1D4.data.u8[2] = 0x00, LEAF_1D4.data.u8[3] = 0x00,LEAF_1D4.data.u8[4] = 0x00,LEAF_1D4.data.u8[5] = 0x00};

//Nissan LEAF battery parameters from CAN
#define WH_PER_GID 77   //One GID is this amount of Watt hours
#define LB_MAX_SOC 1000  //LEAF BMS never goes over this value. We use this info to rescale SOC% sent to Fronius
#define LB_MIN_SOC 0   //LEAF BMS never goes below this value. We use this info to rescale SOC% sent to Fronius
static uint16_t LB_Discharge_Power_Limit = 0; //Limit in kW
static uint16_t LB_Charge_Power_Limit = 0; //Limit in kW
static int16_t LB_MAX_POWER_FOR_CHARGER = 0; //Limit in kW
static int16_t LB_SOC = 500; //0 - 100.0 % (0-1000)
static uint16_t LB_Wh_Remaining = 0; //Amount of energy in battery, in Wh
static uint16_t LB_GIDS = 0;
static uint16_t LB_MAX = 0;
static uint16_t LB_Max_GIDS = 273; //Startup in 24kWh mode
static uint16_t LB_SOH = 99; //State of health %
static uint16_t LB_Total_Voltage = 370; //Battery voltage (0-450V)
static int16_t LB_Current = 0; //Current in A going in/out of battery
static int16_t LB_Power = 0; //Watts going in/out of battery
static int16_t LB_HistData_Temperature_MAX = 60; //-40 to 86*C
static int16_t LB_HistData_Temperature_MIN = 50; //-40 to 86*C
static uint8_t LB_Relay_Cut_Request = 0; //LB_FAIL
static uint8_t LB_Failsafe_Status = 0; //LB_STATUS = 000b = normal start Request
                                            //001b = Main Relay OFF Request
                                            //010b = Charging Mode Stop Request
                                            //011b =  Main Relay OFF Request
                                            //100b = Caution Lamp Request
                                            //101b = Caution Lamp Request & Main Relay OFF Request
                                            //110b = Caution Lamp Request & Charging Mode Stop Request
                                            //111b = Caution Lamp Request & Main Relay OFF Request
static byte LB_Interlock = 1; //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
static byte LB_Full_CHARGE_flag = 0; //LB_FCHGEND , Goes to 1 if battery is fully charged
static byte LB_MainRelayOn_flag = 0; //No-Permission=0, Main Relay On Permission=1
static byte LB_Capacity_Empty = 0; //LB_EMPTY, , Goes to 1 if battery is empty

void update_values_leaf_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
	SOH = (LB_SOH * 100); //Increase range from 99% -> 99.00%

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

	battery_voltage = (LB_Total_Voltage*10); //One more decimal needed

	capacity_Wh = (LB_Max_GIDS * WH_PER_GID);

	remaining_capacity_Wh = LB_Wh_Remaining;

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

	/*Extra safeguards*/
	if(LB_GIDS < 10) //800Wh left in battery
	{ //Battery is running abnormally low, some discharge logic might have failed. Zero it all out.
		SOC = 0;
		max_target_discharge_power = 0;
	}

	if(LB_Full_CHARGE_flag)
	{ //Battery reports that it is fully charged stop all further charging incase it hasn't already
		max_target_charge_power = 0;
	}
  
	if(LB_Relay_Cut_Request)
	{ //LB_FAIL, BMS requesting shutdown and contactors to be opened
		Serial.println("Battery requesting immediate shutdown and contactors to be opened!");
		bms_status = FAULT;
		SOC = 0;
		max_target_discharge_power = 0;
		max_target_charge_power = 0;
	}

	if(LB_Failsafe_Status > 0) // 0 is normal, start charging/discharging
	{
		switch(LB_Failsafe_Status)
		{
			case(1):
				//Normal Stop Request
				//This means that battery is fully discharged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
				break;
			case(2):
				//Charging Mode Stop Request
				//This means that battery is fully charged and it's OK to stop the session. For stationary storage we don't disconnect contactors, so we do nothing here.
				break;
			case(3):
				//Charging Mode Stop Request & Normal Stop Request
				//Normal stop request. For stationary storage we don't disconnect contactors, so we ignore this.
				break;
			case(4):
				//Caution Lamp Request
				Serial.println("Battery raised caution indicator. Inspect battery status!");
				break;
			case(5):
				//Caution Lamp Request & Normal Stop Request
				bms_status = FAULT;
				Serial.println("Battery raised caution indicator AND requested discharge stop. Inspect battery status!");
				break;
			case(6):
				//Caution Lamp Request & Charging Mode Stop Request
				bms_status = FAULT;
				Serial.println("Battery raised caution indicator AND requested charge stop. Inspect battery status!");
				break;
			case(7):
				//Caution Lamp Request & Charging Mode Stop Request & Normal Stop Request
				bms_status = FAULT;
				Serial.println("Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!");
				break;
			default:
				break;
		}      
	}

	#ifdef INTERLOCK_REQUIRED
	if(!LB_Interlock)
	{
	Serial.println("Battery interlock loop broken. Check that high voltage connectors are seated. Battery will be disabled!");
	bms_status = FAULT;
	SOC = 0;
	max_target_discharge_power = 0;
	max_target_charge_power = 0;
	}
	#endif

	/* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
	if(!stillAlive)
	{
	bms_status = FAULT;
	Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
	}
	else
	{
	stillAlive--;
	}
	
	LB_Power = LB_Total_Voltage * LB_Current;//P = U * I
	stat_batt_power = convert2unsignedint16(LB_Power); //add sign if needed

	LB_HistData_Temperature_MIN = (LB_HistData_Temperature_MIN * 10); //increase range to fit the Fronius
	temperature_min = convert2unsignedint16(LB_HistData_Temperature_MIN); //add sign if needed

	LB_HistData_Temperature_MAX = (LB_HistData_Temperature_MAX * 10); //increase range to fit the Fronius
	temperature_max = convert2unsignedint16(LB_HistData_Temperature_MAX);

  if(printValues)
  {  //values heading towards the modbus registers
	Serial.println("SOH%: ");
	Serial.println(SOH);
	Serial.println("SOC% to Fronius: ");
	Serial.println(SOC);
	Serial.println("Max discharge power: ");
	Serial.println(max_target_discharge_power);
	Serial.println("Max charge power: ");
	Serial.println(max_target_charge_power);
	Serial.println("Temperature Min: ");
	Serial.println(temperature_min);
	Serial.println("Temperature Max: ");
	Serial.println(temperature_max);
	Serial.println("GIDS: ");
	Serial.println(LB_GIDS);
  }
}

void handle_can_leaf_battery()
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
			case 0x1DB:
				LB_Current = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
				if (LB_Current & 0x0400)
				{
					// negative so extend the sign bit
					LB_Current |= 0xf800;
				}
        	if (LB_Current > 0) { //Positive value = Charging on LEAF 
            bms_char_dis_status = 2; //Charging
          } else if (LB_Current < 0) { //Negative value = Discharging on LEAF
            bms_char_dis_status = 1; //Discharging
          } else { //LB_Current == 0
            bms_char_dis_status = 0; //idle
	        }

				LB_Total_Voltage = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6) / 2;

				//Collect various data from the BMS
				LB_Relay_Cut_Request = ((rx_frame.data.u8[1] & 0x18) >> 3);
				LB_Failsafe_Status = (rx_frame.data.u8[1] & 0x07);
				LB_MainRelayOn_flag = (byte) ((rx_frame.data.u8[3] & 0x20) >> 5);
				LB_Full_CHARGE_flag = (byte) ((rx_frame.data.u8[3] & 0x10) >> 4);
				LB_Interlock = (byte) ((rx_frame.data.u8[3] & 0x08) >> 3);
				break;
			case 0x1DC:
				LB_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
				LB_Charge_Power_Limit = (((rx_frame.data.u8[1] & 0x3F) << 2 | rx_frame.data.u8[2] >> 4) / 4.0);
				LB_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
				break;
			case 0x55B:
				LB_SOC = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
				break;
			case 0x5BC:
				stillAlive = 6; //Indicate that we are still getting CAN messages from the BMS

				LB_MAX = ((rx_frame.data.u8[5] & 0x10) >> 4);
				if (LB_MAX)
				{
					LB_Max_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
					//Max gids active, do nothing
					//Only the 30/40/62kWh packs have this mux
				}
				else
				{
					//Normal current GIDS value is transmitted
					LB_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
					LB_Wh_Remaining = (LB_GIDS * WH_PER_GID);
				}
				LB_SOH = (rx_frame.data.u8[4] >> 1); //Collect state of health from battery
				break;
			case 0x5C0: //This method only works for 2011-2017 LEAF packs, the mux is different on 2018+ 40/62kWH packs
				if ((rx_frame.data.u8[0]>>6) == 1)
				{ // Battery Temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
					LB_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
					//Serial.println(LB_HistData_Temperature_MAX);
				}
				if ((rx_frame.data.u8[0]>>6) == 3)
				{ // Battery Temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
					LB_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
					//Serial.println(LB_HistData_Temperature_MIN);
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
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;

		ESP32Can.CANWriteFrame(&LEAF_50B); //Always send 50B as a static message (Contains HCM_WakeUpSleepCommand == 11b == WakeUp, and CANMASK = 1)

		mprun100++;
		if (mprun100 > 3)
		{
			mprun100 = 0;
		}

		if (mprun100 == 0)
		{
			LEAF_50C.data.u8[5] = 0x00;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0xC8;
		}
		else if(mprun100 == 1)
		{
			LEAF_50C.data.u8[5] = 0x01;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0x5F;
		}
		else if(mprun100 == 2)
		{
			LEAF_50C.data.u8[5] = 0x02;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0x63;
		}
		else if(mprun100 == 3)
		{
			LEAF_50C.data.u8[5] = 0x03;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0xF4;
		}
		ESP32Can.CANWriteFrame(&LEAF_50C);
	}
	//Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;

		if(mprun10 == 0)
		{
		  LEAF_1D4.data.u8[4] = 0x07;
		  LEAF_1D4.data.u8[5] = 0x46;
		  LEAF_1D4.data.u8[6] = 0x01;
		  LEAF_1D4.data.u8[7] = 0x44;
		}
		else if(mprun10 == 1)
		{
		  LEAF_1D4.data.u8[4] = 0x47;
		  LEAF_1D4.data.u8[5] = 0x46;
		  LEAF_1D4.data.u8[6] = 0x01;
		  LEAF_1D4.data.u8[7] = 0x44;
		}
		else if(mprun10 == 2)
		{
		  LEAF_1D4.data.u8[4] = 0x87;
		  LEAF_1D4.data.u8[5] = 0x46;
		  LEAF_1D4.data.u8[6] = 0x01;
		  LEAF_1D4.data.u8[7] = 0x88;
		}
		else if(mprun10 == 3)
		{
		  LEAF_1D4.data.u8[4] = 0xC7;
		  LEAF_1D4.data.u8[5] = 0x46;
		  LEAF_1D4.data.u8[6] = 0x01;
		  LEAF_1D4.data.u8[7] = 0x4F;
		}
		ESP32Can.CANWriteFrame(&LEAF_1D4); 

			mprun10++;
			if (mprun10 > 3)
			{
				mprun10 = 0;
			}

		switch(mprun10r)
		{
		case(0):	
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x00;
			LEAF_1F2.data.u8[7] = 0x8F;
			break;
		case(1):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x01;
			LEAF_1F2.data.u8[7] = 0x80;
			break;
		case(2):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x02;
			LEAF_1F2.data.u8[7] = 0x81;
			break;
		case(3):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x03;
			LEAF_1F2.data.u8[7] = 0x82;
			break;
		case(4):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x00;
			LEAF_1F2.data.u8[7] = 0x8F;
			break;
		case(5):	// Set 2
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x01;
			LEAF_1F2.data.u8[7] = 0x84;
			break;
		case(6):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x02;
			LEAF_1F2.data.u8[7] = 0x85;
			break;
		case(7):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x03;
			LEAF_1F2.data.u8[7] = 0x86;
			break;
		case(8):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x00;
			LEAF_1F2.data.u8[7] = 0x83;
			break;
		case(9):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x01;
			LEAF_1F2.data.u8[7] = 0x84;
			break;
		case(10):	// Set 3
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x02;
			LEAF_1F2.data.u8[7] = 0x81;
			break;
		case(11):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x03;
			LEAF_1F2.data.u8[7] = 0x82;
			break;
		case(12):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x00;
			LEAF_1F2.data.u8[7] = 0x8F;
			break;
		case(13):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x01;
			LEAF_1F2.data.u8[7] = 0x80;
			break;
		case(14):
			LEAF_1F2.data.u8[3] = 0xB0;
			LEAF_1F2.data.u8[6] = 0x02;
			LEAF_1F2.data.u8[7] = 0x81;
			break;
		case(15):	// Set 4
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x03;
			LEAF_1F2.data.u8[7] = 0x86;
			break;
		case(16):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x00;
			LEAF_1F2.data.u8[7] = 0x83;
			break;
		case(17):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x01;
			LEAF_1F2.data.u8[7] = 0x84;
			break;
		case(18):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x02;
			LEAF_1F2.data.u8[7] = 0x85;
			break;
		case(19):
			LEAF_1F2.data.u8[3] = 0xB4;
			LEAF_1F2.data.u8[6] = 0x03;
			LEAF_1F2.data.u8[7] = 0x86;
			break;
		default:
			break;
		}

		ESP32Can.CANWriteFrame(&LEAF_1F2); //Contains (CHG_STA_RQ == 1 == Normal Charge)

		mprun10r++;
		if(mprun10r > 19)	// 0x1F2 patter repeats after 20 messages,
		{
		mprun10r = 0;
		}
		//Serial.println("CAN 10ms done");
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
