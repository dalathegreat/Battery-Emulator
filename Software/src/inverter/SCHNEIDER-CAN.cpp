#include "../include.h"
#ifdef SCHNEIDER_CAN
#include "../datalayer/datalayer.h"
#include "SCHNEIDER-CAN.h"

/* Version 2: SE BMS Communication Protocol 
Protocol: CAN 2.0 Specification
Frame: Extended CAN Bus Frame (29 bit identifier)
Bitrate: 500 kbps
Endian: Big Endian (MSB, most significant byte of a value received first)*/

/* TODOs
- Figure out how to reply with protocol version in 0x320
- Figure out what to set Battery Manufacturer ID to in 0x330
- Figure out what to set Battery Model ID in 0x330
  - We will need CAN logs from existing battery OR contact Schneider for one free number
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10s = 0;    // will store last time a 10s CAN Message was send
static unsigned long previousMillis2s = 0;     // will store last time a 2s CAN Message was send
static unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was send

CAN_frame SE_320 = {.FD = false,  //SE BMS Protocol Version
                    .ext_ID = true,
                    .DLC = 2,
                    .ID = 0x320,
                    .data = {0x00, 0x02}};  //TODO: How do we reply with Protocol Version: 0x0002 ?
CAN_frame SE_321 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x321,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_322 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x322,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_323 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x323,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_324 = {.FD = false, .ext_ID = true, .DLC = 4, .ID = 0x324, .data = {0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_325 = {.FD = false, .ext_ID = true, .DLC = 6, .ID = 0x325, .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_326 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x326,
                    .data = {0x00, STATE_STARTING, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_327 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x327,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_328 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x328,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_330 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x330,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_331 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x331,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_332 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x332,
                    .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SE_333 = {.FD = false,
                    .ext_ID = true,
                    .DLC = 8,
                    .ID = 0x333,
                    .data = {0x53, 0x45, 0x42, 0x4D, 0x53, 0x00, 0x00, 0x00}};  //SEBMS

static int16_t temperature_average = 0;
static uint16_t remaining_capacity_ah = 0;
static uint16_t fully_charged_capacity_ah = 0;
static uint16_t commands = 0;
static uint16_t warnings = 0;
static uint16_t faults = 0;
static uint16_t state = 0;

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  /* Calculate temperature */
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  /* Calculate capacity, Amp hours(Ah) = Watt hours (Wh) / Voltage (V)*/
  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    remaining_capacity_ah =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
    fully_charged_capacity_ah =
        ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
  }
  /* Set active commands/warnings/faults/state*/
  if (datalayer.battery.status.bms_status == FAULT) {
    state = STATE_FAULTED;
    //TODO: Map warnings and faults incase an event is set. Low prio, but nice to have
    commands = COMMAND_STOP;
  } else {  //Battery-Emulator running
    state = STATE_ONLINE;
    warnings = 0;
    faults = 0;
    if (datalayer.battery.status.reported_soc == 10000) {
      //Battery full. Only allow discharge
      commands = COMMAND_ONLY_DISCHARGE_ALLOWED;
    } else if (datalayer.battery.status.reported_soc == 0) {
      //Battery empty. Only allow charge
      commands = COMMAND_ONLY_CHARGE_ALLOWED;
    } else {  //SOC is somewhere between 0.1% and 99.9%. Allow both charge and discharge
      commands = COMMAND_CHARGE_AND_DISCHARGE_ALLOWED;
    }
  }

  //Map values to CAN messages
  //Max charge voltage+2 (eg 10000.00V = 1000000 , 32bits long)
  SE_321.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV * 10) >> 24);
  SE_321.data.u8[1] = (((datalayer.battery.info.max_design_voltage_dV * 10) & 0x00FF0000) >> 16);
  SE_321.data.u8[2] = (((datalayer.battery.info.max_design_voltage_dV * 10) & 0x0000FF00) >> 8);
  SE_321.data.u8[3] = ((datalayer.battery.info.max_design_voltage_dV * 10) & 0x000000FF);
  //Minimum discharge voltage+2 (eg 10000.00V = 1000000 , 32bits long)
  SE_321.data.u8[4] = ((datalayer.battery.info.min_design_voltage_dV * 10) >> 24);
  SE_321.data.u8[5] = (((datalayer.battery.info.min_design_voltage_dV * 10) & 0x00FF0000) >> 16);
  SE_321.data.u8[6] = (((datalayer.battery.info.min_design_voltage_dV * 10) & 0x0000FF00) >> 8);
  SE_321.data.u8[7] = ((datalayer.battery.info.min_design_voltage_dV * 10) & 0x000000FF);

  //Maximum charge current+2 (eg 10000.00A = 1000000) TODO: Note s32 bit, which direction?
  SE_322.data.u8[0] = ((datalayer.battery.status.max_charge_current_dA * 10) >> 24);
  SE_322.data.u8[1] = (((datalayer.battery.status.max_charge_current_dA * 10) & 0x00FF0000) >> 16);
  SE_322.data.u8[2] = (((datalayer.battery.status.max_charge_current_dA * 10) & 0x0000FF00) >> 8);
  SE_322.data.u8[3] = ((datalayer.battery.status.max_charge_current_dA * 10) & 0x000000FF);
  //Maximum discharge current+2 (eg 10000.00A = 1000000) TODO: Note s32 bit, which direction?
  SE_322.data.u8[4] = ((datalayer.battery.status.max_discharge_current_dA * 10) >> 24);
  SE_322.data.u8[5] = (((datalayer.battery.status.max_discharge_current_dA * 10) & 0x00FF0000) >> 16);
  SE_322.data.u8[6] = (((datalayer.battery.status.max_discharge_current_dA * 10) & 0x0000FF00) >> 8);
  SE_322.data.u8[7] = ((datalayer.battery.status.max_discharge_current_dA * 10) & 0x000000FF);

  //Voltage (ex 370.00 = 37000, 32bits long)
  SE_323.data.u8[0] = ((datalayer.battery.status.voltage_dV * 10) >> 24);
  SE_323.data.u8[1] = (((datalayer.battery.status.voltage_dV * 10) & 0x00FF0000) >> 16);
  SE_323.data.u8[2] = (((datalayer.battery.status.voltage_dV * 10) & 0x0000FF00) >> 8);
  SE_323.data.u8[3] = ((datalayer.battery.status.voltage_dV * 10) & 0x000000FF);
  //Current (ex 81.00A = 8100) TODO: Note s32 bit, which direction?
  SE_323.data.u8[4] = ((datalayer.battery.status.current_dA * 10) >> 24);
  SE_323.data.u8[5] = (((datalayer.battery.status.current_dA * 10) & 0x00FF0000) >> 16);
  SE_323.data.u8[6] = (((datalayer.battery.status.current_dA * 10) & 0x0000FF00) >> 8);
  SE_323.data.u8[7] = ((datalayer.battery.status.current_dA * 10) & 0x000000FF);

  //Temperature average
  SE_324.data.u8[0] = (temperature_average >> 8);
  SE_324.data.u8[1] = (temperature_average & 0x00FF);
  //SOC (100.0%)
  SE_324.data.u8[2] = ((datalayer.battery.status.reported_soc / 10) >> 8);
  SE_324.data.u8[3] = ((datalayer.battery.status.reported_soc / 10) & 0x00FF);
  //Commands (enum)
  SE_325.data.u8[0] = (commands >> 8);
  SE_325.data.u8[1] = (commands & 0x00FF);
  //Warnings (enum)
  SE_325.data.u8[2] = (warnings >> 8);
  SE_325.data.u8[3] = (warnings & 0x00FF);
  //Faults (enum)
  SE_325.data.u8[4] = (faults >> 8);
  SE_325.data.u8[5] = (faults & 0x00FF);

  //State (enum)
  SE_326.data.u8[0] = (state >> 8);
  SE_326.data.u8[1] = (state & 0x00FF);
  //Cycle count (OPTIONAL UINT16)
  //SE_326.data.u8[2] = Cycle count not tracked by emulator
  //SE_326.data.u8[3] = Cycle count not tracked by emulator
  //StateOfHealth (OPTIONAL 0-100%)
  SE_326.data.u8[4] = (datalayer.battery.status.soh_pptt / 100 >> 8);
  SE_326.data.u8[5] = (datalayer.battery.status.soh_pptt / 100 & 0x00FF);
  //Capacity (OPTIONAL, full charge) AH+1
  SE_326.data.u8[6] = (fully_charged_capacity_ah >> 8);
  SE_326.data.u8[7] = (fully_charged_capacity_ah & 0x00FF);

  //Cell temp max (OPTIONAL dC)
  SE_327.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  SE_327.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //Cell temp min (OPTIONAL dC)
  SE_327.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  SE_327.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  //Cell max volt (OPTIONAL 4.000V)
  SE_327.data.u8[4] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  SE_327.data.u8[5] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  //Cell min volt (OPTIONAL 4.000V)
  SE_327.data.u8[6] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  SE_327.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);

  //Lifetime Charge Energy (OPTIONAL, WH, UINT32)
  //SE_328.data.u8[0] = Lifetime energy not tracked by emulator
  //SE_328.data.u8[1] = Lifetime energy not tracked by emulator
  //SE_328.data.u8[2] = Lifetime energy not tracked by emulator
  //SE_328.data.u8[3] = Lifetime energy not tracked by emulator
  //Lifetime Discharge Energy (OPTIONAL, WH, UINT32)
  //SE_328.data.u8[4] = Lifetime energy not tracked by emulator
  //SE_328.data.u8[5] = Lifetime energy not tracked by emulator
  //SE_328.data.u8[6] = Lifetime energy not tracked by emulator
  //SE_328.data.u8[7] = Lifetime energy not tracked by emulator

  //Battery Manufacturer ID (UINT16)
  //Unique identifier for each battery manufacturer implementing this protocol. IDs must be requested through Schneider Electric Solar.
  SE_330.data.u8[0] = 0;  //TODO, set Battery Manufacturer ID
  SE_330.data.u8[1] = 0;  //TODO, set Battery Manufacturer ID
  //Battery Model ID (UINT16)
  //Unique identifier for each battery model that a manufacturer has implemented this protocol on. IDs must be requested through Schneider Electric Solar.
  SE_330.data.u8[2] = 0;  //TODO, set Battery Model ID
  SE_330.data.u8[3] = 0;  //TODO, set Battery Model ID
  //Serial numbers
  //(For instance ABC123 would be represented as:
  //0x41[char5], 0x42[char4], 0x43[char3], 0x31[char2], 0x32 [char1], 0x33 [char0])
  SE_330.data.u8[4] = 0x42;  //Char 19 - B
  SE_330.data.u8[5] = 0x41;  //Char 18 - A
  SE_330.data.u8[6] = 0x54;  //Char 17 - T
  SE_330.data.u8[7] = 0x54;  //Char 16 - T

  SE_331.data.u8[0] = 0x45;  //Char 15 - E
  SE_331.data.u8[1] = 0x52;  //Char 14 - R
  SE_331.data.u8[2] = 0x59;  //Char 13 - Y
  SE_331.data.u8[3] = 0x45;  //Char 12 - E
  SE_331.data.u8[4] = 0x4D;  //Char 11 - M
  SE_331.data.u8[5] = 0x55;  //Char 10 - U
  SE_331.data.u8[6] = 0x4C;  //Char 9 - L
  SE_331.data.u8[7] = 0x41;  //Char 8 - A

  SE_332.data.u8[0] = 0x54;  //Char 7 - T
  SE_332.data.u8[1] = 0x4F;  //Char 6 - O
  SE_332.data.u8[2] = 0x52;  //Char 5 - R
  SE_332.data.u8[3] = 0x30;  //Char 4 - 0
  SE_332.data.u8[4] = 0x31;  //Char 3 - 1
  SE_332.data.u8[5] = 0x32;  //Char 2 - 2
  SE_332.data.u8[6] = 0x33;  //Char 1 - 3
  SE_332.data.u8[7] = 0x34;  //Char 0 - 4

  //UNIQUE ID
  //Schneider Electric Unique string identifier. The value should be an unique string "SEBMS"
  SE_333.data.u8[0] = 0x53;  //Char 5 - S
  SE_333.data.u8[1] = 0x45;  //Char 4 - E
  SE_333.data.u8[2] = 0x42;  //Char 3 - B
  SE_333.data.u8[3] = 0x4D;  //Char 2 - M
  SE_333.data.u8[4] = 0x53;  //Char 1 - S
  SE_333.data.u8[5] = 0x00;  //Char 0 - NULL

  //Protocol version, TODO: How do we reply with protocol version 0x0002 ?
  SE_320.data.u8[0] = 0x00;
  SE_320.data.u8[1] = 0x02;
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x310:  // Still alive message from inverter, every 1s
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can(&SE_321, can_config.inverter);
    transmit_can(&SE_322, can_config.inverter);
    transmit_can(&SE_323, can_config.inverter);
    transmit_can(&SE_324, can_config.inverter);
    transmit_can(&SE_325, can_config.inverter);
  }
  // Send 2s CAN Message
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;

    transmit_can(&SE_320, can_config.inverter);
    transmit_can(&SE_326, can_config.inverter);
    transmit_can(&SE_327, can_config.inverter);
  }
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;
    transmit_can(&SE_328, can_config.inverter);
    transmit_can(&SE_330, can_config.inverter);
    transmit_can(&SE_331, can_config.inverter);
    transmit_can(&SE_332, can_config.inverter);
    transmit_can(&SE_333, can_config.inverter);
  }
}

void setup_inverter(void) {  // Performs one time setup
  strncpy(datalayer.system.info.inverter_protocol, "Schneider V2 SE BMS CAN", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}

#endif
