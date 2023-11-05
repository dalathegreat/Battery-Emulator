#include "PYLON-CAN.h"
#include "../../ESP32CAN.h"
#include "../../CAN_config.h"

#define SEND_0 //If defined, the messages will have ID ending with 0 (useful for some inverters)
//#define SEND_1 //If defined, the messages will have ID ending with 1 (useful for some inverters)

/* Do not change code below unless you are sure what you are doing */
//Actual content messages
CAN_frame_t PYLON_7310 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x7310,.data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
CAN_frame_t PYLON_7320 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x7320,.data = {0x4B, 0x00, 0x05, 0x0F, 0x2D, 0x00, 0x56, 0x00}};

CAN_frame_t PYLON_4210 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4210,.data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
CAN_frame_t PYLON_4220 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4220,.data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
CAN_frame_t PYLON_4230 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4230,.data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
CAN_frame_t PYLON_4240 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4240,.data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
CAN_frame_t PYLON_4250 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4250,.data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_4260 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4260,.data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
CAN_frame_t PYLON_4270 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4270,.data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
CAN_frame_t PYLON_4280 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4280,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_4290 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4290,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

CAN_frame_t PYLON_7311 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x7311,.data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
CAN_frame_t PYLON_7321 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x7321,.data = {0x4B, 0x00, 0x05, 0x0F, 0x2D, 0x00, 0x56, 0x00}};

CAN_frame_t PYLON_4211 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4211,.data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
CAN_frame_t PYLON_4221 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4221,.data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
CAN_frame_t PYLON_4231 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4231,.data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
CAN_frame_t PYLON_4241 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4241,.data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
CAN_frame_t PYLON_4251 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4251,.data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_4261 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4261,.data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
CAN_frame_t PYLON_4271 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4271,.data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
CAN_frame_t PYLON_4281 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4281,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_4291 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_ext,}},.MsgID = 0x4291,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


void update_values_can_pylon()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages
  //There are more mappings that could be added, but this should be enough to use as a starting point
  // Note we map both 0 and 1 messages

  //Charge / Discharge allowed
  PYLON_4280.data.u8[0] = 0;
  PYLON_4280.data.u8[1] = 0;
  PYLON_4280.data.u8[2] = 0;
  PYLON_4280.data.u8[3] = 0;
  PYLON_4281.data.u8[0] = 0;
  PYLON_4281.data.u8[1] = 0;
  PYLON_4281.data.u8[2] = 0;
  PYLON_4281.data.u8[3] = 0;

  //Voltage (370.0)
  PYLON_4210.data.u8[0] = (battery_voltage >> 8);
  PYLON_4210.data.u8[1] = (battery_voltage & 0x00FF);
  PYLON_4211.data.u8[0] = (battery_voltage >> 8);
  PYLON_4211.data.u8[1] = (battery_voltage & 0x00FF);

  //Current (TODO, SIGNED? Or looks like it could be just offset, in that case the below line wont work)
  PYLON_4210.data.u8[2] = (battery_current >> 8);
  PYLON_4210.data.u8[3] = (battery_current & 0x00FF);
  PYLON_4211.data.u8[2] = (battery_current >> 8);
  PYLON_4211.data.u8[3] = (battery_current & 0x00FF);

  //SOC (100.00%)
  PYLON_4210.data.u8[6] = (SOC*0.01); //Remove decimals
  PYLON_4211.data.u8[6] = (SOC*0.01); //Remove decimals

  //StateOfHealth (100.00%)
  PYLON_4210.data.u8[7] = (StateOfHealth*0.01);
  PYLON_4211.data.u8[7] = (StateOfHealth*0.01);

  //Minvoltage (eg 300.0V = 3000 , 16bits long) Charge Cutoff Voltage
  PYLON_4220.data.u8[0] = (min_volt_pylon_can >> 8);
  PYLON_4220.data.u8[1] = (min_volt_pylon_can & 0x00FF);
  PYLON_4221.data.u8[0] = (min_volt_pylon_can >> 8);
  PYLON_4221.data.u8[1] = (min_volt_pylon_can & 0x00FF);

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Discharge Cutoff Voltage
  PYLON_4220.data.u8[2] = (max_volt_pylon_can >> 8);
  PYLON_4220.data.u8[3] = (max_volt_pylon_can & 0x00FF);
  PYLON_4221.data.u8[2] = (max_volt_pylon_can >> 8);
  PYLON_4221.data.u8[3] = (max_volt_pylon_can & 0x00FF);

  //In case we run into any errors/faults, we can set charge / discharge forbidden
  if(bms_status == FAULT)
  {
    PYLON_4280.data.u8[0] = 0xAA;
    PYLON_4280.data.u8[1] = 0xAA;
    PYLON_4280.data.u8[2] = 0xAA;
    PYLON_4280.data.u8[3] = 0xAA;
    PYLON_4281.data.u8[0] = 0xAA;
    PYLON_4281.data.u8[1] = 0xAA;
    PYLON_4281.data.u8[2] = 0xAA;
    PYLON_4281.data.u8[3] = 0xAA;
  }

}

void receive_can_pylon(CAN_frame_t rx_frame)
{
  switch (rx_frame.MsgID)
  {
    case 0x4200: //Message originating from inverter. Depending on which data is required, act accordingly
    if(rx_frame.data.u8[0] == 0x02)
    {
      send_setup_info();
    }
    if(rx_frame.data.u8[0] == 0x00)
    {
      send_system_data();
    }
    break;
    default:
    break;
  }
}

void send_setup_info()
{ //Ensemble information
  #ifdef SEND_0
  ESP32Can.CANWriteFrame(&PYLON_7310);
  ESP32Can.CANWriteFrame(&PYLON_7320);
  #endif
  #ifdef SEND_1
  ESP32Can.CANWriteFrame(&PYLON_7311);
  ESP32Can.CANWriteFrame(&PYLON_7321);
  #endif
}

void send_system_data()
{ //System equipment information
  #ifdef SEND_0
  ESP32Can.CANWriteFrame(&PYLON_4210);
  ESP32Can.CANWriteFrame(&PYLON_4220);
  ESP32Can.CANWriteFrame(&PYLON_4230);
  ESP32Can.CANWriteFrame(&PYLON_4240);
  ESP32Can.CANWriteFrame(&PYLON_4250);
  ESP32Can.CANWriteFrame(&PYLON_4260);
  ESP32Can.CANWriteFrame(&PYLON_4270);
  ESP32Can.CANWriteFrame(&PYLON_4280);
  ESP32Can.CANWriteFrame(&PYLON_4290);
  #endif
  #ifdef SEND_1
  ESP32Can.CANWriteFrame(&PYLON_4211);
  ESP32Can.CANWriteFrame(&PYLON_4221);
  ESP32Can.CANWriteFrame(&PYLON_4231);
  ESP32Can.CANWriteFrame(&PYLON_4241);
  ESP32Can.CANWriteFrame(&PYLON_4251);
  ESP32Can.CANWriteFrame(&PYLON_4261);
  ESP32Can.CANWriteFrame(&PYLON_4271);
  ESP32Can.CANWriteFrame(&PYLON_4281);
  ESP32Can.CANWriteFrame(&PYLON_4291);
  #endif
}