#include "SOLAX-CAN.h"

/* Do not change code below unless you are sure what you are doing */
static uint16_t max_charge_rate_amp = 0;
static uint16_t max_discharge_rate_amp = 0;
static uint16_t temperature_average = 0;
static int STATE = BATTERY_ANNOUNCE;
static unsigned long LastFrameTime = 0;
static const uint8_t BatteryModuleFirmware = 2;

//CAN message translations from this amazing repository: https://github.com/rand12345/solax_can_bus 

CAN_frame_t SOLAX_1801 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1801,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1872 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1872,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_Limits
CAN_frame_t SOLAX_1873 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1873,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_PackData
CAN_frame_t SOLAX_1874 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1874,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_CellData
CAN_frame_t SOLAX_1875 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1875,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_Status
CAN_frame_t SOLAX_1876 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1876,.data = {0x0, 0x0, 0xE2, 0x0C, 0x0, 0x0, 0xD7, 0x0C}}; //BMS_PackTemps
CAN_frame_t SOLAX_1877 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1877,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1878 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1878,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}; //BMS_PackStats
CAN_frame_t SOLAX_1879 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1879,.data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t SOLAX_1881 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1881,.data = {0x00, 0x36, 0x53, 0x42, 0x4D, 0x53, 0x46, 0x41}}; // E.g.: 0 6 S B M S F A
CAN_frame_t SOLAX_1882 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x1882,.data = {0x00, 0x32, 0x33, 0x41, 0x42, 0x30, 0x35, 0x32}}; // E.g.: 0 2 3 A B 0 5 2
CAN_frame_t SOLAX_100A001 = {.FIR = {.B = {.DLC = 0,.FF = CAN_frame_ext,}},.MsgID = 0x100A001,.data = {}};

// __builtin_bswap64 needed to convert to ESP32 little endian format
// Byte[4] defines the requested contactor state: 1 = Closed , 0 = Open
#define Contactor_Open_Payload  __builtin_bswap64(0x0200010000000000)
#define Contactor_Close_Payload  __builtin_bswap64(0x0200010001000000) 

void CAN_WriteFrame(CAN_frame_t* tx_frame)
{
if(dual_can){
  CANMessage MCP2515Frame; //Struct with ACAN2515 library format, needed to use the MCP2515 library
  MCP2515Frame.id = tx_frame->MsgID;
  MCP2515Frame.ext = tx_frame->FIR.B.FF;
  MCP2515Frame.len = tx_frame->FIR.B.DLC;
  for (uint8_t i=0 ; i<MCP2515Frame.len ; i++) {
        MCP2515Frame.data[i] = tx_frame->data.u8[i];
      }
  can.tryToSend(MCP2515Frame);
  }
  else{
  ESP32Can.CANWriteFrame(tx_frame);
  }
}

void update_values_can_solax()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages
  // If not receiveing any communication from the inverter, open contactors and return to battery announce state
  if (millis() - LastFrameTime >= SolaxTimeout)
  {
    inverterAllowsContactorClosing = 0;
    STATE = BATTERY_ANNOUNCE;
  }
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
  SOLAX_1874.data.u8[4] = (uint8_t) (cell_max_voltage); //Todo, scaling OK? Supposed to be alarm trigger absolute cell max?
  SOLAX_1874.data.u8[5] = (cell_max_voltage >> 8); 
  SOLAX_1874.data.u8[6] = (uint8_t) (cell_min_voltage); //Todo, scaling OK? Supposed to be alarm trigger absolute cell min?
  SOLAX_1874.data.u8[7] = (cell_min_voltage >> 8);

  //BMS_Status
  SOLAX_1875.data.u8[0] = (uint8_t) temperature_average; 
  SOLAX_1875.data.u8[1] = (temperature_average >> 8);
  SOLAX_1875.data.u8[2] = (uint8_t) 0; // Number of slave batteries
  SOLAX_1875.data.u8[4] = (uint8_t) 0; // Contactor Status 0=off, 1=on.

  //BMS_PackTemps (strange name, since it has voltages?)
  SOLAX_1876.data.u8[2] = (uint8_t) cell_max_voltage; //Todo, scaling OK?
  SOLAX_1876.data.u8[3] = (cell_min_voltage >> 8);

  SOLAX_1876.data.u8[6] = (uint8_t) cell_min_voltage; //Todo, scaling OK?
  SOLAX_1876.data.u8[7] = (cell_min_voltage >> 8);

  //Unknown
  SOLAX_1877.data.u8[4] = (uint8_t) 0x53;
  SOLAX_1877.data.u8[6] = (BatteryModuleFirmware >> 8);
  SOLAX_1877.data.u8[7] = (uint8_t) BatteryModuleFirmware;

  //BMS_PackStats
  SOLAX_1878.data.u8[0] = (uint8_t) (battery_voltage/10); //TODO, should this be max or current voltage?
  SOLAX_1878.data.u8[1] = ((battery_voltage/10) >> 8);

  SOLAX_1878.data.u8[4] = (uint8_t) capacity_Wh; //TODO, scaling OK?
  SOLAX_1878.data.u8[5] = (capacity_Wh >> 8);

  // BMS_Answer
  SOLAX_1801.data.u8[0] = 2;
  SOLAX_1801.data.u8[2] = 1;
  SOLAX_1801.data.u8[4] = 1;

}

void receive_can_solax(CAN_frame_t rx_frame)
{
  if (rx_frame.MsgID == 0x1871) {
    LastFrameTime = millis();
    switch(STATE)
    {
      case(BATTERY_ANNOUNCE):
        Serial.println("Solax Battery State: Announce");
        inverterAllowsContactorClosing = 0;
        SOLAX_1875.data.u8[4] = (0x00); // Inform Inverter: Contactor 0=off, 1=on.
        CAN_WriteFrame(&SOLAX_100A001); //BMS Announce
        CAN_WriteFrame(&SOLAX_1872);
        CAN_WriteFrame(&SOLAX_1873);
        CAN_WriteFrame(&SOLAX_1874);
        CAN_WriteFrame(&SOLAX_1875);
        CAN_WriteFrame(&SOLAX_1876);
        CAN_WriteFrame(&SOLAX_1877);
        CAN_WriteFrame(&SOLAX_1878);
        // Message from the inverter to proceed to contactor closing
        // Byte 4 changes from 0 to 1
        if (rx_frame.data.u64 == Contactor_Close_Payload)
        STATE = WAITING_FOR_CONTACTOR;
      break;

      case(WAITING_FOR_CONTACTOR):
        SOLAX_1875.data.u8[4] = (0x00); // Inform Inverter: Contactor 0=off, 1=on.
        CAN_WriteFrame(&SOLAX_1872);
        CAN_WriteFrame(&SOLAX_1873);
        CAN_WriteFrame(&SOLAX_1874);
        CAN_WriteFrame(&SOLAX_1875);
        CAN_WriteFrame(&SOLAX_1876);
        CAN_WriteFrame(&SOLAX_1877);
        CAN_WriteFrame(&SOLAX_1878);
        CAN_WriteFrame(&SOLAX_1801); // Announce that the battery will be connected
        STATE = CONTACTOR_CLOSED; // Jump to Contactor Closed State
        Serial.println("Solax Battery State: Contactor Closed");
      break;
      
      case(CONTACTOR_CLOSED):
        inverterAllowsContactorClosing = 1;
        SOLAX_1875.data.u8[4] = (0x01); // Inform Inverter: Contactor 0=off, 1=on.
        CAN_WriteFrame(&SOLAX_1872);
        CAN_WriteFrame(&SOLAX_1873);
        CAN_WriteFrame(&SOLAX_1874);
        CAN_WriteFrame(&SOLAX_1875);
        CAN_WriteFrame(&SOLAX_1876);
        CAN_WriteFrame(&SOLAX_1877);
        CAN_WriteFrame(&SOLAX_1878);
        // Message from the inverter to open contactor
        // Byte 4 changes from 1 to 0
        if (rx_frame.data.u64 == Contactor_Open_Payload)
        STATE = BATTERY_ANNOUNCE;
      break;
    }
  }
}
