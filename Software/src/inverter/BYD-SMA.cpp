#include "../include.h"
#ifdef BYD_SMA
#include "../datalayer/datalayer.h"
#include "BYD-SMA.h"

/* TODO: Map error bits in 0x158 */

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100ms = 0;

//Actual content messages
CAN_frame SMA_358 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x358,
                     .data = {0x11, 0xA0, 0x07, 0x00, 0x01, 0x5E, 0x00, 0xC8}};
CAN_frame SMA_3D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3D8,
                     .data = {0x13, 0x2E, 0x27, 0x10, 0x00, 0x45, 0xF9, 0x00}};
CAN_frame SMA_458 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x458,
                     .data = {0x00, 0x00, 0x11, 0xC8, 0x00, 0x00, 0x0E, 0xF4}};
CAN_frame SMA_518 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x518,
                     .data = {0x01, 0x4A, 0x01, 0x25, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame SMA_4D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x4D8,
                     .data = {0x10, 0x62, 0x00, 0x16, 0x01, 0x68, 0x03, 0x08}};
CAN_frame SMA_158 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x158,  // All 0xAA, no faults active
                     .data = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};

// Pairing/Battery setup information

CAN_frame SMA_558 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x558,
                     .data = {0x03, 0x13, 0x00, 0x03, 0x00, 0x66, 0x04, 0x07}};  //4x BYD modules, Vendor ID 7 BYD
CAN_frame SMA_598 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x598,
                     .data = {0x00, 0x01, 0x0F, 0x2C, 0x5C, 0x98, 0xB6, 0xEE}};
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
                       .data = {0x01, 0x2D, 0x42, 0x6F, 0x78, 0x20, 0x48, 0x31}};  //- B o x  H 1
CAN_frame SMA_618_3 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x02, 0x30, 0x2E, 0x32, 0x00, 0x00, 0x00, 0x00}};  // 0 . 2

static int16_t discharge_current = 0;
static int16_t charge_current = 0;
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
  //Maxvoltage (eg 400.0V = 4000 , 16bits long)
  SMA_358.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SMA_358.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  //Minvoltage (eg 300.0V = 3000 , 16bits long)
  SMA_358.data.u8[2] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  SMA_358.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  //Discharge limited current, 500 = 50A, (0.1, A)
  SMA_358.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  SMA_358.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Charge limited current, 125 =12.5A (0.1, A)
  SMA_358.data.u8[6] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SMA_358.data.u8[7] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

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

  //Error bits
  if (!datalayer.system.status.inverter_allows_contactor_closing) {
    SMA_158.data.u8[2] = 0x6A;
  } else {
    SMA_158.data.u8[2] = 0xAA;
  }

  /*
  //SMA_158.data.u8[0] = //bit12 Fault high temperature, bit34Battery cellundervoltage, bit56 Battery cell overvoltage, bit78 batterysystemdefect
  //TODO: add all error bits. Sending message with all 0xAA until that.

  0x158 can be used to send error messages or warnings.

  Each message is defined of two bits:  
  01=message triggered  
  10=no message triggered  
  0xA9=10101001, triggers first message  
  0xA6=10100110, triggers second message  
  0x9A=10011010, triggers third message  
  0x6A=01101010, triggers forth message  
  bX defines the byte

  b0 A9   Battery system defect
  b0 A6   Battery cell overvoltage fault
  b0 9A   Battery cell undervoltage fault
  b0 6A   Battery high temperature fault
  b1 A9   Battery low temperature fault
  b1 A6   Battery high temperature fault
  b1 9A   Battery low temperature fault
  b1 6A   Overload (reboot required)
  b2 A9   Overload (reboot required)
  b2 A6   Incorrect switch position for the battery disconnection point
  b2 9A   Battery system short circuit
  b2 6A   Internal battery hardware fault
  b3 A9   Battery imbalancing fault
  b3 A6   Battery service life expiry
  b3 9A   Battery system thermal management defective
  b3 6A   Internal battery hardware fault
  b4 A9   Battery system defect (warning)
  b4 A6   Battery cell overvoltage fault (warning)
  b4 9A   Battery cell undervoltage fault (warning)
  b4 6A   Battery high temperature fault (warning)
  b5 A9   Battery low temperature fault (warning)
  b5 A6   Battery high temperature fault (warning)
  b5 9A   Battery low temperature fault (warning)
  b5 6A   Self-diagnosis (warning)
  b6 A9   Self-diagnosis (warning)
  b6 A6   Incorrect switch position for the battery disconnection point (warning)
  b6 9A   Battery system short circuit (warning)
  b6 6A   Internal battery hardware fault (warning)
  b7 A9   Battery imbalancing fault (warning)
  b7 A6   Battery service life expiry (warning)
  b7 9A   Battery system thermal management defective (warning)
  b7 6A   Internal battery hardware fault (warning)

*/
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x360:  //Message originating from SMA inverter - Voltage and current
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Frame0-1 Voltage
      //Frame2-3 Current
      break;
    case 0x3E0:  //Message originating from SMA inverter - ?
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x420:  //Message originating from SMA inverter - Timestamp
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Frame0-3 Timestamp
      /*
      transmit_can(&SMA_158, can_config.inverter);
      transmit_can(&SMA_358, can_config.inverter);
      transmit_can(&SMA_3D8, can_config.inverter);
      transmit_can(&SMA_458, can_config.inverter);
      transmit_can(&SMA_518, can_config.inverter);
      transmit_can(&SMA_4D8, can_config.inverter);
      */
      break;
    case 0x5E0:  //Message originating from SMA inverter - String
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x560:  //Message originating from SMA inverter - Init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E7:  //Pairing request
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can(&SMA_558, can_config.inverter);
      transmit_can(&SMA_598, can_config.inverter);
      transmit_can(&SMA_5D8, can_config.inverter);
      transmit_can(&SMA_618_1, can_config.inverter);
      transmit_can(&SMA_618_2, can_config.inverter);
      transmit_can(&SMA_618_3, can_config.inverter);
      transmit_can(&SMA_158, can_config.inverter);
      transmit_can(&SMA_358, can_config.inverter);
      transmit_can(&SMA_3D8, can_config.inverter);
      transmit_can(&SMA_458, can_config.inverter);
      transmit_can(&SMA_518, can_config.inverter);
      transmit_can(&SMA_4D8, can_config.inverter);
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();

  // Send CAN Message every 100ms if we're enabled
  if (datalayer.system.status.inverter_allows_contactor_closing) {
    if (currentMillis - previousMillis100ms >= 100) {
      previousMillis100ms = currentMillis;
      transmit_can(&SMA_158, can_config.inverter);
      transmit_can(&SMA_358, can_config.inverter);
      transmit_can(&SMA_3D8, can_config.inverter);
      transmit_can(&SMA_458, can_config.inverter);
      transmit_can(&SMA_518, can_config.inverter);
      transmit_can(&SMA_4D8, can_config.inverter);
    }
  }
}
void setup_inverter(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, "BYD Battery-Box HVS over SMA CAN", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
  datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
  pinMode(INVERTER_CONTACTOR_ENABLE_PIN, INPUT);
}
#endif
