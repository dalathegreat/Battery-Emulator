#include "SOLAX-CAN.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100ms = 0; // will store last time a 100ms CAN Message was sent
static const int interval100ms = 100; // interval (ms) at which send CAN Messages
static int max_charge_rate_amp = 0;
static int max_discharge_rate_amp = 0;
static int temperature_average = 0;

//CAN message translations from this amazing repository: https://github.com/rand12345/solax_can_bus 

CAN_frame_t SOLAX_1872 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1872,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_Limits
CAN_frame_t SOLAX_1873 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1873,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_PackData
CAN_frame_t SOLAX_1874 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1874,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_CellData
CAN_frame_t SOLAX_1875 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1875,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_Status
CAN_frame_t SOLAX_1876 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1876,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_PackTemps
CAN_frame_t SOLAX_1877 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1877,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1878 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1878,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_PackStats
CAN_frame_t SOLAX_1879 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1879,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1801 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1801,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1881 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1881,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; // E.g.: 0 6 S B M S F A
CAN_frame_t SOLAX_1882 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1882,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; // E.g.: 0 2 3 A B 0 5 2
CAN_frame_t SOLAX_100A001 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x100A001,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

void update_values_can_solax()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages
  
  //Calculate the required values
  temperature_average = ((temperature_max + temperature_min)/2);

  //max_target_charge_power (30000W max)
  if(SOC > 9999) //99.99%
  { //Additional safety incase SOC% is 100, then do not charge battery further
    max_charge_rate_amp = 0;
  }
  else
  { //We can pass on the battery charge rate (in W) to the inverter (that takes A)
    if(max_target_charge_power >= 30000)
    {
      max_charge_rate_amp = 75; //Incase battery can take over 30kW, cap value to 75A
    }
    else
    { //Calculate the W value into A
      max_charge_rate_amp = (max_target_charge_power/(battery_voltage*0.1)); // P/U = I
    }
  }

  //max_target_discharge_power (30000W max)
  if(SOC < 100) //1.00%
  { //Additional safety incase SOC% is below 1, then do not charge battery further
    max_discharge_rate_amp = 0;
  }
  else
  { //We can pass on the battery discharge rate to the inverter
    if(max_target_discharge_power >= 30000)
    {
      max_discharge_rate_amp = 75; //Incase battery can be charged with over 30kW, cap value to 75A
    }
    else
    { //Calculate the W value into A
      max_discharge_rate_amp = (max_target_discharge_power/(battery_voltage*0.1)); // P/U = I
    }
  }
  
  //Put the values into the CAN messages

  //BMS_Limits
  SOLAX_1872.data.u8[0] = (uint8_t) max_volt_solax_can; //Todo, scaling OK?
  SOLAX_1872.data.u8[1] = (max_volt_solax_can >> 8);
  SOLAX_1872.data.u8[2] = (uint8_t) min_volt_solax_can; //Todo, scaling OK?
  SOLAX_1872.data.u8[3] = (min_volt_solax_can >> 8);
  SOLAX_1872.data.u8[4] = (uint8_t) (max_charge_rate_amp*10); //Todo, scaling OK?
  SOLAX_1872.data.u8[5] = ((max_charge_rate_amp*10) >> 8);
  SOLAX_1872.data.u8[6] = (uint8_t) (max_discharge_rate_amp*10); //Todo, scaling OK?
  SOLAX_1872.data.u8[7] = ((max_discharge_rate_amp*10) >> 8);

  //BMS_PackData
  SOLAX_1873.data.u8[0] = (uint8_t) battery_voltage; //Todo, scaling OK?
  SOLAX_1873.data.u8[1] = (battery_voltage >> 8);
  SOLAX_1873.data.u8[2] = (int8_t) stat_batt_power; //Todo, scaling OK? Signed?
  SOLAX_1873.data.u8[3] = (stat_batt_power >> 8);
  SOLAX_1873.data.u8[4] = (uint8_t) (SOC/100); //SOC (100.00%)
  //SOLAX_1873.data.u8[5] = //Seems like this is not required? Or shall we put SOC decimals here?
  SOLAX_1873.data.u8[6] = (uint8_t) (remaining_capacity_Wh/100); //Todo, scaling OK?
  SOLAX_1873.data.u8[7] = ((remaining_capacity_Wh/100) >> 8);

  //BMS_CellData
  SOLAX_1874.data.u8[0] = (uint8_t) temperature_max; 
  SOLAX_1874.data.u8[1] = (temperature_max >> 8);
  SOLAX_1874.data.u8[2] = (uint8_t) temperature_min; 
  SOLAX_1874.data.u8[3] = (temperature_min >> 8);
  SOLAX_1874.data.u8[4] = (uint8_t) (cell_max_voltage/100); //Todo, scaling OK? Supposed to be alarm trigger absolute cell max?
  SOLAX_1874.data.u8[5] = ((cell_max_voltage/100) >> 8); 
  SOLAX_1874.data.u8[6] = (uint8_t) (cell_min_voltage/100); //Todo, scaling OK? Supposed to be alarm trigger absolute cell min?
  SOLAX_1874.data.u8[7] = ((cell_min_voltage/100) >> 8);

  //BMS_Status
  SOLAX_1875.data.u8[0] = (uint8_t) temperature_average; 
  SOLAX_1875.data.u8[1] = (temperature_average >> 8);
  SOLAX_1875.data.u8[2] = (2); // Contactor, 0=off, 2=on //Todo, base this on what variable?

  //BMS_PackTemps (strange name, since it has voltages?)
  SOLAX_1876.data.u8[2] = (uint8_t) cell_max_voltage; //Todo, scaling OK?
  SOLAX_1876.data.u8[3] = (cell_min_voltage >> 8);

  SOLAX_1876.data.u8[6] = (uint8_t) cell_min_voltage; //Todo, scaling OK?
  SOLAX_1876.data.u8[7] = (cell_min_voltage >> 8);

  //Unknown
  SOLAX_1877.data.u8[4] = (uint8_t) 1; // ID?
  SOLAX_1877.data.u8[6] = (uint8_t) 1; // byte1?
  SOLAX_1877.data.u8[7] = (uint8_t) 1; // byte2?

  //BMS_PackStats
  SOLAX_1878.data.u8[0] = (uint8_t) battery_voltage; //TODO, should this be max or current voltage?
  SOLAX_1878.data.u8[1] = (battery_voltage >> 8);

  SOLAX_1878.data.u8[4] = (uint8_t) capacity_Wh; //TODO, scaling OK?
  SOLAX_1878.data.u8[5] = (capacity_Wh >> 8);
}

void send_can_solax()
{
  unsigned long currentMillis = millis();
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100ms >= interval100ms)
	{
		previousMillis100ms = currentMillis;

    ESP32Can.CANWriteFrame(&SOLAX_1872);
    ESP32Can.CANWriteFrame(&SOLAX_1873);
    ESP32Can.CANWriteFrame(&SOLAX_1874);
    ESP32Can.CANWriteFrame(&SOLAX_1875);
    ESP32Can.CANWriteFrame(&SOLAX_1876);
    ESP32Can.CANWriteFrame(&SOLAX_1877);
    ESP32Can.CANWriteFrame(&SOLAX_1878);

    //Todo, how often should the messages be sent? And the other messages, only on bootup?
	}
}

void receive_can_solax(CAN_frame_t rx_frame)
{
  //Serial.println("Inverter sending CAN message");
  //0x1871 [0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00]
  //Todo, should we respond with something once the inverter sends a message?
}
