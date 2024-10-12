#include "../include.h"
#ifdef SOFAR_CAN
#include "../datalayer/datalayer.h"
#include "SOFAR-CAN.h"

/* This implementation of the SOFAR can protocol is halfway done. What's missing is implementing the inverter replies, all the CAN messages are listed, but the can sending is missing. */

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send

//Actual content messages
//Note that these are technically extended frames. If more batteries are put in parallel,the first battery sends 0x351 the next battery sends 0x1351 etc. 16 batteries in parallel supported
CAN_frame SOFAR_351 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x351,
                       .data = {0xC6, 0x08, 0xFA, 0x00, 0xFA, 0x00, 0x80, 0x07}};
CAN_frame SOFAR_355 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x355,
                       .data = {0x31, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xF6, 0x00}};
CAN_frame SOFAR_356 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x356,
                       .data = {0x36, 0x08, 0x10, 0x00, 0xD0, 0x00, 0x01, 0x00}};
CAN_frame SOFAR_30F = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x30F,
                       .data = {0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_359 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x359,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x04, 0x10, 0x27, 0x10}};
CAN_frame SOFAR_35E = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x35E,
                       .data = {0x41, 0x4D, 0x41, 0x53, 0x53, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_35F = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x35F,
                       .data = {0x00, 0x00, 0x24, 0x4E, 0x32, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_35A = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x35A,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

CAN_frame SOFAR_670 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x670,
                       .data = {0x00, 0x8A, 0x33, 0x11, 0x59, 0x1A, 0x00, 0x00}};
CAN_frame SOFAR_671 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x671,
                       .data = {0x00, 0x42, 0x48, 0x55, 0x35, 0x31, 0x32, 0x30}};
CAN_frame SOFAR_672 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x672,
                       .data = {0x00, 0x32, 0x35, 0x45, 0x50, 0x43, 0x32, 0x31}};
CAN_frame SOFAR_673 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x673,
                       .data = {0x00, 0x34, 0x32, 0x36, 0x31, 0x36, 0x32, 0x00}};
CAN_frame SOFAR_680 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x680,
                       .data = {0x00, 0xB7, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame SOFAR_681 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x681,
                       .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame SOFAR_682 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x682,
                       .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame SOFAR_683 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x683,
                       .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame SOFAR_684 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x684,
                       .data = {0x00, 0xB6, 0x0C, 0xB3, 0x0C, 0xB4, 0x0C, 0x00}};
CAN_frame SOFAR_685 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x685,
                       .data = {0x00, 0xB3, 0x0C, 0xBB, 0x0C, 0xB3, 0x0C, 0x00}};
CAN_frame SOFAR_690 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x690,
                       .data = {0x00, 0xD7, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_691 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x691,
                       .data = {0x00, 0xD4, 0x00, 0xD1, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_6A0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x6A0,
                       .data = {0x00, 0xFA, 0x00, 0xDD, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_6B0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x6B0,
                       .data = {0x00, 0xF6, 0x00, 0x06, 0x02, 0x01, 0x00, 0x00}};
CAN_frame SOFAR_6C0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x6C0,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_770 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x770,
                       .data = {0x00, 0x56, 0x0B, 0xF0, 0x58, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_771 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x771,
                       .data = {0x00, 0x42, 0x48, 0x55, 0x35, 0x31, 0x32, 0x30}};
CAN_frame SOFAR_772 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x772,
                       .data = {0x00, 0x32, 0x35, 0x45, 0x50, 0x43, 0x32, 0x31}};
CAN_frame SOFAR_773 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x773,
                       .data = {0x00, 0x34, 0x32, 0x36, 0x31, 0x36, 0x32, 0x00}};
CAN_frame SOFAR_780 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x780,
                       .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame SOFAR_781 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x781,
                       .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame SOFAR_782 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x782,
                       .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame SOFAR_783 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x783,
                       .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame SOFAR_784 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x784,
                       .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame SOFAR_785 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x785,
                       .data = {0x00, 0xEB, 0x0C, 0xED, 0x0C, 0xED, 0x0C, 0x00}};
CAN_frame SOFAR_790 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x790,
                       .data = {0x00, 0xCD, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_791 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x791,
                       .data = {0x00, 0xCD, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_7A0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x7A0,
                       .data = {0x00, 0xFA, 0x00, 0xE1, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SOFAR_7B0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x7B0,
                       .data = {0x00, 0xF9, 0x00, 0x06, 0x02, 0xE9, 0x5D, 0x00}};
CAN_frame SOFAR_7C0 = {.FD = false,
                       .ext_ID = true,
                       .DLC = 8,
                       .ID = 0x7C0,
                       .data = {0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x80, 0x00}};

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
  SOFAR_351.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SOFAR_351.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  //SOFAR_351.data.u8[2] = DC charge current limitation (Default 25.0A)
  //SOFAR_351.data.u8[3] = DC charge current limitation
  //SOFAR_351.data.u8[4] = DC discharge current limitation (Default 25.0A)
  //SOFAR_351.data.u8[5] = DC discharge current limitation
  //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
  SOFAR_351.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  SOFAR_351.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);

  //SOC
  SOFAR_355.data.u8[0] = (datalayer.battery.status.reported_soc / 100);
  SOFAR_355.data.u8[2] = (datalayer.battery.status.soh_pptt / 100);
  //SOFAR_355.data.u8[6] = (AH_remaining >> 8);
  //SOFAR_355.data.u8[7] = (AH_remaining & 0x00FF);

  //Voltage (370.0)
  SOFAR_356.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  SOFAR_356.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SOFAR_356.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  SOFAR_356.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  SOFAR_356.data.u8[2] = (datalayer.battery.status.temperature_max_dC >> 8);
  SOFAR_356.data.u8[3] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //In here we need to respond to the inverter. TODO: make logic
    case 0x605:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //frame1_605 = rx_frame.data.u8[1];
      //frame3_605 = rx_frame.data.u8[3];
      break;
    case 0x705:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //frame1_705 = rx_frame.data.u8[1];
      //frame3_705 = rx_frame.data.u8[3];
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //Frames actively reported by BMS
    transmit_can(&SOFAR_351, can_config.inverter);
    transmit_can(&SOFAR_355, can_config.inverter);
    transmit_can(&SOFAR_356, can_config.inverter);
    transmit_can(&SOFAR_30F, can_config.inverter);
    transmit_can(&SOFAR_359, can_config.inverter);
    transmit_can(&SOFAR_35E, can_config.inverter);
    transmit_can(&SOFAR_35F, can_config.inverter);
    transmit_can(&SOFAR_35A, can_config.inverter);
  }
}
#endif
