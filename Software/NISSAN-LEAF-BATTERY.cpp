#include "NISSAN-LEAF-BATTERY.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
const int interval10 = 10; // interval (ms) at which send CAN Messages
const int interval100 = 100; // interval (ms) at which send CAN Messages
const int rx_queue_size = 10; // Receive Queue size
uint16_t CANerror = 0; //counter on how many CAN errors encountered
uint8_t CANstillAlive = 12; //counter for checking if CAN is still alive 
uint8_t errorCode = 0; //stores if we have an error code active from battery control logic
uint8_t mprun10r = 0; //counter 0-20 for 0x1F2 message
byte mprun10 = 0; //counter 0-3
byte mprun100 = 0; //counter 0-3

CAN_frame_t LEAF_1F2 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x1F2,.data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
CAN_frame_t LEAF_50B = {.FIR = {.B = {.DLC = 7,.FF = CAN_frame_std,}},.MsgID = 0x50B,.data = {0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x00}};
CAN_frame_t LEAF_50C = {.FIR = {.B = {.DLC = 6,.FF = CAN_frame_std,}},.MsgID = 0x50C,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t LEAF_1D4 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x1D4,.data = {0x6E, 0x6E, 0x00, 0x04, 0x07, 0x46, 0xE0, 0x44}};

uint8_t	crctable[256] = {0,133,143,10,155,30,20,145,179,54,60,185,40,173,167,34,227,102,108,233,120,253,247,114,80,213,223,90,203,78,68,193,67,
                        198,204,73,216,93,87,210,240,117,127,250,107,238,228,97,160,37,47,170,59,190,180,49,19,150,156,25,136,13,7,130,134,3,9,
                        140,29,152,146,23,53,176,186,63,174,43,33,164,101,224,234,111,254,123,113,244,214,83,89,220,77,200,194,71,197,64,74,207,
                        94,219,209,84,118,243,249,124,237,104,98,231,38,163,169,44,189,56,50,183,149,16,26,159,14,139,129,4,137,12,6,131,18,151,
                        157,24,58,191,181,48,161,36,46,171,106,239,229,96,241,116,126,251,217,92,86,211,66,199,205,72,202,79,69,192,81,212,222,
                        91,121,252,246,115,226,103,109,232,41,172,166,35,178,55,61,184,154,31,21,144,1,132,142,11,15,138,128,5,148,17,27,158,188,
                        57,51,182,39,162,168,45,236,105,99,230,119,242,248,125,95,218,208,85,196,65,75,206,76,201,195,70,215,82,88,221,255,122,
                        112,245,100,225,235,110,175,42,32,165,52,177,187,62,28,153,147,22,135,2,8,141};

//Nissan LEAF battery parameters from CAN
#define ZE0_BATTERY 0
#define AZE0_BATTERY 1
#define ZE1_BATTERY 2
uint8_t LEAF_Battery_Type = ZE0_BATTERY;
#define WH_PER_GID 77   //One GID is this amount of Watt hours
#define LB_MAX_SOC 1000  //LEAF BMS never goes over this value. We use this info to rescale SOC% sent to Fronius
#define LB_MIN_SOC 0   //LEAF BMS never goes below this value. We use this info to rescale SOC% sent to Fronius
uint16_t LB_Discharge_Power_Limit = 0; //Limit in kW
uint16_t LB_Charge_Power_Limit = 0; //Limit in kW
int16_t LB_MAX_POWER_FOR_CHARGER = 0; //Limit in kW
int16_t LB_SOC = 500; //0 - 100.0 % (0-1000)
uint16_t LB_TEMP = 0; //Temporary value used in status checks
uint16_t LB_Wh_Remaining = 0; //Amount of energy in battery, in Wh
uint16_t LB_GIDS = 0;
uint16_t LB_MAX = 0;
uint16_t LB_Max_GIDS = 273; //Startup in 24kWh mode
uint16_t LB_StateOfHealth = 99; //State of health %
uint16_t LB_Total_Voltage = 370; //Battery voltage (0-450V)
int16_t LB_Current = 0; //Current in A going in/out of battery
int16_t LB_Power = 0; //Watts going in/out of battery
int16_t LB_HistData_Temperature_MAX = 6; //-40 to 86*C
int16_t LB_HistData_Temperature_MIN = 5; //-40 to 86*C
uint8_t LB_Relay_Cut_Request = 0; //LB_FAIL
uint8_t LB_Failsafe_Status = 0; //LB_STATUS = 000b = normal start Request
                                            //001b = Main Relay OFF Request
                                            //010b = Charging Mode Stop Request
                                            //011b =  Main Relay OFF Request
                                            //100b = Caution Lamp Request
                                            //101b = Caution Lamp Request & Main Relay OFF Request
                                            //110b = Caution Lamp Request & Charging Mode Stop Request
                                            //111b = Caution Lamp Request & Main Relay OFF Request                                     
byte LB_Interlock = 1; //Contains info on if HV leads are seated (Note, to use this both HV connectors need to be inserted)
byte LB_Full_CHARGE_flag = 0; //LB_FCHGEND , Goes to 1 if battery is fully charged
byte LB_MainRelayOn_flag = 0; //No-Permission=0, Main Relay On Permission=1
byte LB_Capacity_Empty = 0; //LB_EMPTY, , Goes to 1 if battery is empty

void update_values_leaf_battery()
{ //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE; //Startout in active mode
  
  StateOfHealth = (LB_StateOfHealth * 100); //Increase range from 99% -> 99.00%

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

  battery_current = LB_Current;

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
    //Note, this is sometimes triggered during the night while idle, and the BMS recovers after a while. Removed latching from this scenario
    errorCode = 1;
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
    errorCode = 2;
    Serial.println("Battery raised caution indicator AND requested discharge stop. Inspect battery status!");
    break;
    case(6):
    //Caution Lamp Request & Charging Mode Stop Request
    bms_status = FAULT;
    errorCode = 3;
    Serial.println("Battery raised caution indicator AND requested charge stop. Inspect battery status!");
    break;
    case(7):
    //Caution Lamp Request & Charging Mode Stop Request & Normal Stop Request
    bms_status = FAULT;
    errorCode = 4;
    Serial.println("Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!");
    break;
    default:
    break;
    }      
  }

  if(LB_StateOfHealth < 25)
  { //Battery is extremely degraded, not fit for secondlifestorage. Zero it all out.
    if(LB_StateOfHealth != 0)
    { //Extra check to see that we actually have a SOH Value available
      Serial.println("State of health critically low. Battery internal resistance too high to continue. Recycle battery.");
      bms_status = FAULT;
      errorCode = 5;
      max_target_discharge_power = 0;
      max_target_charge_power = 0;
    }
  }

  #ifdef INTERLOCK_REQUIRED
  if(!LB_Interlock)
  {
    Serial.println("Battery interlock loop broken. Check that high voltage connectors are seated. Battery will be disabled!");
    bms_status = FAULT;
    errorCode = 6;
    SOC = 0;
    max_target_discharge_power = 0;
    max_target_charge_power = 0;
  }
  #endif

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if(!CANstillAlive)
  {
    bms_status = FAULT;
    errorCode = 7;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  }
  else
  {
    CANstillAlive--;
  }
	
  LB_Power = LB_Total_Voltage * LB_Current;//P = U * I
  stat_batt_power = convert2unsignedint16(LB_Power); //add sign if needed

	temperature_min = convert2unsignedint16((LB_HistData_Temperature_MIN * 10)); //add sign if needed and increase range
	temperature_max = convert2unsignedint16((LB_HistData_Temperature_MAX * 10));

  if(printValues)
  {  //values heading towards the modbus registers
    if(errorCode > 0)
      {
        Serial.print("ERROR CODE ACTIVE IN SYSTEM. NUMBER: ");
        Serial.println(errorCode);
      }
    Serial.print("BMS Status (3=OK): ");
    Serial.println(bms_status);
    switch (bms_char_dis_status)
			{
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
    Serial.print("Power: ");
    Serial.println(LB_Power);
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
    Serial.print("GIDS: ");
    Serial.println(LB_GIDS);
    Serial.print("LEAF battery gen: ");
    Serial.println(LEAF_Battery_Type);
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
        if(is_message_corrupt(rx_frame))
        {
          CANerror++;
          break; //Message content malformed, abort reading data from it
        }
				LB_Current = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
        if (LB_Current & 0x0400)
        {
          // negative so extend the sign bit
          LB_Current |= 0xf800;
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
        if(is_message_corrupt(rx_frame))
        {
          CANerror++;
          break; //Message content malformed, abort reading data from it
        }
				LB_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
        LB_Charge_Power_Limit = (((rx_frame.data.u8[1] & 0x3F) << 2 | rx_frame.data.u8[2] >> 4) / 4.0);
				LB_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
				break;
			case 0x55B:
        if(is_message_corrupt(rx_frame))
        {
          CANerror++;
          break; //Message content malformed, abort reading data from it
        }
        LB_TEMP = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
        if (LB_TEMP != 0x3ff) //3FF is unavailable value
        {
          LB_SOC = LB_TEMP;
        }
				break;
			case 0x5BC:
        CANstillAlive = 12; //Indicate that we are still getting CAN messages from the BMS

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

        LB_TEMP = (rx_frame.data.u8[4] >> 1);
        if (LB_TEMP != 0)
        {
          LB_StateOfHealth = LB_TEMP; //Collect state of health from battery
        }
				break;
      case 0x5C0: //This method only works for 2013-2017 AZE0 LEAF packs, the mux is different on other generations
        if(LEAF_Battery_Type == AZE0_BATTERY)
        {
          if ((rx_frame.data.u8[0]>>6) == 1)
          { // Battery MAX temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
            LB_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
          }
          if ((rx_frame.data.u8[0]>>6) == 3)
          { // Battery MIN temperature. Effectively has only 7-bit precision, as the bottom bit is always 0.
            LB_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
          }
        }
        if(LEAF_Battery_Type == ZE1_BATTERY)
        { //note different mux location in first frame
          if ((rx_frame.data.u8[0] & 0x0F) == 1) 
          {
            LB_HistData_Temperature_MAX = ((rx_frame.data.u8[2] / 2) - 40);
          }
          if ((rx_frame.data.u8[0] & 0x0F) == 3) 
          {
            LB_HistData_Temperature_MIN = ((rx_frame.data.u8[2] / 2) - 40);
          }
        }
        break;
      case 0x59E:
        //AZE0 2013-2017 or ZE1 2018-2023 battery detected
        //Only detect as AZE0 if not already set as ZE1
        if(LEAF_Battery_Type != ZE1_BATTERY)
        {
          LEAF_Battery_Type = AZE0_BATTERY;
        }
        break;
      case 0x1ED:
      case 0x1C2:
        //ZE1 2018-2023 battery detected!
        LEAF_Battery_Type = ZE1_BATTERY;
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
			LEAF_50C.data.u8[3] = 0x00;
			LEAF_50C.data.u8[4] = 0x5D;
			LEAF_50C.data.u8[5] = 0xC8;
		}
		else if(mprun100 == 1)
		{
			LEAF_50C.data.u8[3] = 0x01;
			LEAF_50C.data.u8[4] = 0xB2;
			LEAF_50C.data.u8[5] = 0x31;
		}
		else if(mprun100 == 2)
		{
			LEAF_50C.data.u8[3] = 0x02;
			LEAF_50C.data.u8[4] = 0x5D;
			LEAF_50C.data.u8[5] = 0x63;
		}
		else if(mprun100 == 3)
		{
			LEAF_50C.data.u8[3] = 0x03;
			LEAF_50C.data.u8[4] = 0xB2;
			LEAF_50C.data.u8[5] = 0x9A;
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
      LEAF_1D4.data.u8[7] = 0x12;
    }
    else if(mprun10 == 1)
    {
      LEAF_1D4.data.u8[4] = 0x47;
      LEAF_1D4.data.u8[7] = 0xD5;
    }
    else if(mprun10 == 2)
    {
      LEAF_1D4.data.u8[4] = 0x87;
      LEAF_1D4.data.u8[7] = 0x19;
    }
    else if(mprun10 == 3)
    {
      LEAF_1D4.data.u8[4] = 0xC7;
      LEAF_1D4.data.u8[7] = 0xDE;
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

bool is_message_corrupt(CAN_frame_t rx_frame)
{
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++)
  {
      crc = crctable[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc != rx_frame.data.u8[7];
}
