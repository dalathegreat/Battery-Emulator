#include "../include.h"
#ifdef SMA_TRIPOWER_CAN
#include "../datalayer/datalayer.h"
#include "SMA-TRIPOWER-CAN.h"

/* TODO:
- Figure out the manufacturer info needed in send_tripower_init() CAN messages
  - CAN logs from real system might be needed
- Figure out how cellvoltages need to be displayed
- Figure out if sending send_tripower_init() like we do now is OK
- Figure out how to send the non-cyclic messages when needed
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis500ms = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis2s = 0;     // will store last time a 2s CAN Message was send
static unsigned long previousMillis10s = 0;    // will store last time a 10s CAN Message was send

static uint32_t inverter_time = 0;
static uint16_t inverter_voltage = 0;
static int16_t inverter_current = 0;
static bool pairing_completed = false;
static int16_t temperature_average = 0;
static uint16_t ampere_hours_remaining = 0;

//Actual content messages
CAN_frame SMA_558 = {.FD = false,  //Pairing first message
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x558,                                                // BYD HVS 10.2 kWh (0x66 might be kWh)
                     .data = {0x03, 0x24, 0x00, 0x04, 0x00, 0x66, 0x04, 0x09}};  //Amount of modules? Vendor ID?
CAN_frame SMA_598 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x598,
                     .data = {0x12, 0x30, 0x8A, 0x5B, 0x00, 0x00, 0x00, 0x00}};  //B0-4 Serial?
CAN_frame SMA_5D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x5D8,
                     .data = {0x00, 0x42, 0x59, 0x44, 0x00, 0x00, 0x00, 0x00}};  //B Y D
CAN_frame SMA_618_0 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x00, 0x42, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79}};  //BATTERY
CAN_frame SMA_618_1 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x01, 0x2D, 0x42, 0x6F, 0x78, 0x20, 0x50, 0x72}};  //-Box Pr
CAN_frame SMA_618_2 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x02, 0x65, 0x6D, 0x69, 0x75, 0x6D, 0x20, 0x48}};  //emium H
CAN_frame SMA_618_3 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x618,
                       .data = {0x03, 0x56, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00}};  //VS
CAN_frame SMA_358 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x358,
                     .data = {0x12, 0x40, 0x0C, 0x80, 0x01, 0x00, 0x01, 0x00}};
CAN_frame SMA_3D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x3D8,
                     .data = {0x04, 0x06, 0x27, 0x10, 0x00, 0x19, 0x00, 0xFA}};
CAN_frame SMA_458 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x458,
                     .data = {0x00, 0x00, 0x73, 0xAE, 0x00, 0x00, 0x64, 0x64}};
CAN_frame SMA_4D8 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x4D8,
                     .data = {0x10, 0x62, 0x00, 0x00, 0x00, 0x78, 0x02, 0x08}};
CAN_frame SMA_518 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x518,
                     .data = {0x00, 0x96, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00}};

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the inverter CAN
  // Update values
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
  if (datalayer.battery.status.bms_status == FAULT) {
    SMA_4D8.data.u8[6] = STOP_STATE;
  } else {
    SMA_4D8.data.u8[6] = READY_STATE;
  }
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x360:  //Message originating from SMA inverter - Voltage and current
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_voltage = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      inverter_current = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;
    case 0x3E0:  //Message originating from SMA inverter - ?
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x420:  //Message originating from SMA inverter - Timestamp
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_time =
          (rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;
    case 0x560:  //Message originating from SMA inverter - Init
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E0:  //Message originating from SMA inverter - String
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      //Inverter brand (frame1-3 = 0x53 0x4D 0x41) = SMA
      break;
    case 0x660:  //Message originating from SMA inverter - Pairing request
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      send_tripower_init();
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();

  // Send CAN Message only if we're enabled by inverter
  if (!datalayer.system.status.inverter_allows_contactor_closing) {
    return;
  }

  // Send CAN Message every 2s
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;
    transmit_can(&SMA_358, can_config.inverter);
  }
  // Send CAN Message every 10s
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;
    transmit_can(&SMA_518, can_config.inverter);
    transmit_can(&SMA_4D8, can_config.inverter);
    transmit_can(&SMA_3D8, can_config.inverter);
    if (pairing_completed) {
      transmit_can(
          &SMA_458,
          can_config
              .inverter);  //TODO; not confirmed if battery sends. Transmission starts only after battery is paired
    }
  }
}
void send_tripower_init() {
  transmit_can(&SMA_558, can_config.inverter);    //Pairing start - Vendor
  transmit_can(&SMA_598, can_config.inverter);    //Serial
  transmit_can(&SMA_5D8, can_config.inverter);    //BYD
  transmit_can(&SMA_618_0, can_config.inverter);  //BATTERY
  transmit_can(&SMA_618_1, can_config.inverter);  //-Box Pr
  transmit_can(&SMA_618_2, can_config.inverter);  //emium H
  transmit_can(&SMA_618_3, can_config.inverter);  //VS
  transmit_can(&SMA_358, can_config.inverter);
  transmit_can(&SMA_3D8, can_config.inverter);
  transmit_can(&SMA_458, can_config.inverter);
  transmit_can(&SMA_4D8, can_config.inverter);
  transmit_can(&SMA_518, can_config.inverter);
  pairing_completed = true;
}

void setup_inverter(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, "SMA Tripower CAN", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
#endif
