#include "PYLON-CAN.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

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

void update_values_can_pylon()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages
  //TODO, add mappings
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
  ESP32Can.CANWriteFrame(&PYLON_7310);
  ESP32Can.CANWriteFrame(&PYLON_7320);
}

void send_system_data()
{ //System equipment information
  ESP32Can.CANWriteFrame(&PYLON_4210);
  ESP32Can.CANWriteFrame(&PYLON_4220);
  ESP32Can.CANWriteFrame(&PYLON_4230);
  ESP32Can.CANWriteFrame(&PYLON_4240);
  ESP32Can.CANWriteFrame(&PYLON_4250);
  ESP32Can.CANWriteFrame(&PYLON_4260);
  ESP32Can.CANWriteFrame(&PYLON_4270);
  ESP32Can.CANWriteFrame(&PYLON_4280);
  ESP32Can.CANWriteFrame(&PYLON_4290);
}