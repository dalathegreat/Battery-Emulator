#include "SOLAX-CAN.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100ms = 0; // will store last time a 100ms CAN Message was sent
static const int interval100ms = 100; // interval (ms) at which send CAN Messages
static int temp = 0; //Temporary variable used for bitshifting
static int max_charge_rate_amp = 0;
static int max_discharge_rate_amp = 0;

//CAN message translations from this amazing repository: https://github.com/rand12345/solax_can_bus 

CAN_frame_t SOLAX_1872 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1872,.data = {0x8A, 0xF, 0x52, 0xC, 0xCD, 0x0, 0x5E, 0x1}}; //BMS_Limits
CAN_frame_t SOLAX_1873 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1873,.data = {0x6D, 0xD, 0x0, 0x0, 0x5D, 0x0, 0xA3, 0x1}}; //BMS_PackData
CAN_frame_t SOLAX_1874 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1874,.data = {0xCE, 0x0, 0xBC, 0x0, 0x29, 0x0, 0x28, 0x0}}; //BMS_CellData
CAN_frame_t SOLAX_1875 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1875,.data = {0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x3A, 0x0}}; //BMS_Status
CAN_frame_t SOLAX_1876 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1876,.data = {0x0, 0x0, 0xD4, 0x0F, 0x0, 0x0, 0xC9, 0x0F}}; //BMS_PackTemps
CAN_frame_t SOLAX_1877 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1877,.data = {0x0, 0x0, 0x0, 0x0, 0x53, 0x0, 0x1D, 0x10}};
CAN_frame_t SOLAX_1878 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1878,.data = {0x6D, 0xD, 0x0, 0x0, 0xB0, 0x3, 0x4, 0x0}}; //BMS_PackStats
CAN_frame_t SOLAX_1879 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1879,.data = {0x1, 0x8, 0x1, 0x2, 0x1, 0x2, 0x0, 0x3}};
CAN_frame_t SOLAX_1801 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1801,.data = {0x2, 0x0, 0x1, 0x0, 0x1, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1881 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1881,.data = {0x0, 0x36, 0x53, 0x42, 0x4D, 0x53, 0x46, 0x41}}; // 0 6 S B M S F A
CAN_frame_t SOLAX_1882 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1882,.data = {0x0, 0x32, 0x33, 0x41, 0x42, 0x30, 0x35, 0x32}}; // 0 2 3 A B 0 5 2
CAN_frame_t SOLAX_100A001 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x100A001,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

void update_values_can_solax()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages
  
  //SOC (100.00%)
  temp = SOC/100; //Remove decimals, inverter takes only integer in a byte
  SOLAX_1873.data.u8[4] = temp;

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
  //Increase decimal amount
  max_charge_rate_amp = max_charge_rate_amp*10;

  //Write the calculated charge rate to the CAN message
  SOLAX_1872.data.u8[4] = (uint8_t) max_charge_rate_amp; //TODO, test that values are OK
  SOLAX_1872.data.u8[5] = (max_charge_rate_amp << 8);

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
  //Increase decimal amount
  max_discharge_rate_amp = max_discharge_rate_amp*10;

  //Write the calculated charge rate to the CAN message
  SOLAX_1872.data.u8[6] = (uint8_t) max_discharge_rate_amp; //TODO, test that values are OK
  SOLAX_1872.data.u8[7] = (max_discharge_rate_amp << 8);

  //Todo (ranked in priority)
  //Add current
  //Add voltage
  //Add remaining kWh
  //Add temperature
  //Add cell voltages
  //Add pack voltage min/max for alarms
  //Add cell voltage min/max for alarms

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