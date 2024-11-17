#include "../include.h"
#ifdef SMA_CAN
#include "../datalayer/datalayer.h"
#include "SMA-CAN.h"

/* SMA 48V CAN protocol:
CAN 2.0A
500kBit/sec
11-Bit Identifiers */

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100ms = 0;

//Actual content messages
CAN_frame SMA_558 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x558,
                     .data = {0x03, 0x12, 0x00, 0x04, 0x00, 0x59, 0x07, 0x07}};  //7x BYD modules, Vendor ID 7 BYD
CAN_frame SMA_598 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x598,
                     .data = {0x00, 0x00, 0x12, 0x34, 0x5A, 0xDE, 0x07, 0x4F}};  //B0-4 Serial, rest unknown
CAN_frame SMA_5D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x5D8,
                     .data = {0x00, 0x42, 0x59, 0x44, 0x00, 0x00, 0x00, 0x00}};  //B Y D
CAN_frame SMA_618_1 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x00, 0x42, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79}};  //0 B A T T E R Y
CAN_frame SMA_618_2 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x01, 0x2D, 0x42, 0x6F, 0x78, 0x20, 0x48, 0x39}};  //1 - B O X   H
CAN_frame SMA_618_3 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x02, 0x2E, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00}};  //2 - 0
CAN_frame SMA_358 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x358,
                     .data = {0x0F, 0x6C, 0x06, 0x20, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SMA_3D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3D8,
                     .data = {0x04, 0x10, 0x27, 0x10, 0x00, 0x18, 0xF9, 0x00}};
CAN_frame SMA_458 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x458,
                     .data = {0x00, 0x00, 0x06, 0x75, 0x00, 0x00, 0x05, 0xD6}};
CAN_frame SMA_518 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x518,
                     .data = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame SMA_4D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x4D8,
                     .data = {0x09, 0xFD, 0x00, 0x00, 0x00, 0xA8, 0x02, 0x08}};
CAN_frame SMA_158 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x158,
                     .data = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x6A, 0xAA, 0xAA}};

static int16_t temperature_average = 0;
static uint16_t ampere_hours_remaining = 0;

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  //Calculate values

  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    ampere_hours_remaining =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) *
         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
  }

  //Map values to CAN messages
  //Battery charge voltage (eg 400.0V = 4000 , 16bits long)
  SMA_351.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV - 20) >> 8);
  SMA_351.data.u8[1] = ((datalayer.battery.info.max_design_voltage_dV - 20) & 0x00FF);
  //Discharge limited current, 500 = 50A, (0.1, A)
  SMA_351.data.u8[2] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  SMA_351.data.u8[3] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Charge limited current, 125 =12.5A (0.1, A)
  SMA_351.data.u8[4] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SMA_351.data.u8[5] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  //Discharge voltage (eg 300.0V = 3000 , 16bits long)
  SMA_351.data.u8[6] = ((datalayer.battery.info.min_design_voltage_dV + 20) >> 8);
  SMA_351.data.u8[7] = ((datalayer.battery.info.min_design_voltage_dV + 20) & 0x00FF);

  //SOC (100.00%)
  SMA_3D8.data.u8[0] = (datalayer.battery.status.reported_soc >> 8);
  SMA_3D8.data.u8[1] = (datalayer.battery.status.reported_soc & 0x00FF);
  //StateOfHealth (100.00%)
  SMA_3D8.data.u8[2] = (datalayer.battery.status.soh_pptt >> 8);
  SMA_3D8.data.u8[3] = (datalayer.battery.status.soh_pptt & 0x00FF);
  //State of charge (AH, 0.1)
  SMA_3D8.data.u8[4] = (ampere_hours_remaining >> 8);
  SMA_3D8.data.u8[5] = (ampere_hours_remaining & 0x00FF);

  //Voltage (370.0)
  SMA_4D8.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  SMA_4D8.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Current (TODO: signed OK?)
  SMA_4D8.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  SMA_4D8.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Temperature average
  SMA_4D8.data.u8[4] = (temperature_average >> 8);
  SMA_4D8.data.u8[5] = (temperature_average & 0x00FF);
  //Battery ready
  if (datalayer.battery.status.bms_status == ACTIVE) {
    SMA_4D8.data.u8[6] = READY_STATE;
  } else {
    SMA_4D8.data.u8[6] = STOP_STATE;
  }
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Frame0-1 Battery Voltage
      //Frame2-3 Battery Current
      //Frame4-5 Battery Temperature
      //Frame6-7 SOC Battery
      break;
    case 0x306:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Frame0-1 SOH Battery
      //Frame2 Charging procedure
      //Frame3 Operating state
      //Frame4-5 Active error message
      //Frame6-7 Battery charge voltage setpoint
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();

  // Send CAN Message every 100ms if Enable line is HIGH
  if (datalayer.system.status.inverter_allows_contactor_closing) {
    if (currentMillis - previousMillis100ms >= 100) {
      previousMillis100ms = currentMillis;

      transmit_can(&SMA_351, can_config.inverter);
      transmit_can(&SMA_355, can_config.inverter);
      transmit_can(&SMA_356, can_config.inverter);
      transmit_can(&SMA_35A, can_config.inverter);
      transmit_can(&SMA_35B, can_config.inverter);
      transmit_can(&SMA_35E, can_config.inverter);
      transmit_can(&SMA_35F, can_config.inverter);

      //Remote quick stop (optional)
      if (datalayer.battery.status.bms_status == FAULT) {
        transmit_can(&SMA_00F, can_config.inverter);
        //After receiving this message, Sunny Island will immediately go into standby.
        //Please send start command, to start again. Manual start is also possible.
      }
    }
  }
}

void setup_inverter(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, "SMA 48V CAN", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
#endif
