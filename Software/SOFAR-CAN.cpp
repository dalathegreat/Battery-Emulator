#include "SOFAR-CAN.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

/* Do not change code below unless you are sure what you are doing */
//Actual content messages
//Note that these are technically extended frames. If more batteries are put in parallel,the first battery sends 0x351 the next battery sends 0x1351 etc. 16 batteries in parallel supported
CAN_frame_t SOFAR_351 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x351,.data = {0xC6, 0x08, 0xFA, 0x00, 0xFA, 0x00, 0x80, 0x07}};
CAN_frame_t SOFAR_355 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x355,.data = {0x31, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xF6, 0x00}};
CAN_frame_t SOFAR_356 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x356,.data = {0x36, 0x08, 0x10, 0x00, 0xD0, 0x00, 0x01, 0x00}};
CAN_frame_t SOFAR_30F = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x30F,.data = {0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_359 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x359,.data = {0x00, 0x00, 0x00, 0x00, 0x04, 0x10, 0x27, 0x10}};
CAN_frame_t SOFAR_35E = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x35E,.data = {0x41, 0x4D, 0x41, 0x53, 0x53, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_35F = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x35F,.data = {0x00, 0x00, 0x24, 0x4E, 0x32, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_35A = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x35A,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

CAN_frame_t SOFAR_670 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x670,.data = {0x00, 0x8A, 0x33, 0x11, 0x59, 0x1A, 0x00, 0x00}};
CAN_frame_t SOFAR_671 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x671,.data = {0x00, 0x42, 0x48, 0x55, 0x35, 0x31, 0x32, 0x30}};
CAN_frame_t SOFAR_672 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x672,.data = {0x00, 0x32, 0x35, 0x45, 0x50, 0x43, 0x32, 0x31}};
CAN_frame_t SOFAR_673 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x673,.data = {0x00, 0x34, 0x32, 0x36, 0x31, 0x36, 0x32, 0x00}};
CAN_frame_t SOFAR_680 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x680,.data = {0x00, 0xB7, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame_t SOFAR_681 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x681,.data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame_t SOFAR_682 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x682,.data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame_t SOFAR_683 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x683,.data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame_t SOFAR_684 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x684,.data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame_t SOFAR_685 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x685,.data = {0x00, 0xB3, 0x0C, 0xBB, 0x0C, 0xB3, 0x0C, 0x00}};
CAN_frame_t SOFAR_690 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x690,.data = {0x00, 0xD7, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_691 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x691,.data = {0x00, 0xD4, 0x00, 0xD1, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_6A0 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x6A0,.data = {0x00, 0xFA, 0x00, 0xDD, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_6B0 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x6B0,.data = {0x00, 0xF6, 0x00, 0x06, 0x02, 0x01, 0x00, 0x00}};
CAN_frame_t SOFAR_6C0 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x6C0,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_770 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x770,.data = {0x00, 0x56, 0x0B, 0xF0, 0x58, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_771 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x771,.data = {0x00, 0x42, 0x48, 0x55, 0x35, 0x31, 0x32, 0x30}};
CAN_frame_t SOFAR_772 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x772,.data = {0x00, 0x32, 0x35, 0x45, 0x50, 0x43, 0x32, 0x31}};
CAN_frame_t SOFAR_773 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x773,.data = {0x00, 0x34, 0x32, 0x36, 0x31, 0x36, 0x32, 0x00}};
CAN_frame_t SOFAR_780 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x780,.data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame_t SOFAR_781 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x781,.data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame_t SOFAR_782 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x782,.data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame_t SOFAR_783 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x783,.data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame_t SOFAR_784 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x784,.data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame_t SOFAR_785 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x785,.data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame_t SOFAR_790 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x790,.data = {0x00, 0xCD, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_791 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x791,.data = {0x00, 0xCD, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_7A0 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7A0,.data = {0x00, 0xFA, 0x00, 0xE1, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t SOFAR_7B0 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7B0,.data = {0x00, 0xF9, 0x00, 0x06, 0x02, 0xE9, 0x5D, 0x00}};
CAN_frame_t SOFAR_7C0 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7C0,.data = {0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x80, 0x00}};

void update_values_can_pylon()
{ //This function maps all the values fetched from battery CAN to the correct CAN messages

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

}

void receive_can_sofar(CAN_frame_t rx_frame)
{
  switch (rx_frame.MsgID)
  {
    case 0x605:
      frame1_605 = rx_frame.data.u8[1];
      frame3_605 = rx_frame.data.u8[3];
    break;
    case 0x705:
      frame1_705 = rx_frame.data.u8[1];
      frame3_705 = rx_frame.data.u8[3];
    break;
    default:
    break;
  }
}

void send_can_sofar()
{
  unsigned long currentMillis = millis();
}