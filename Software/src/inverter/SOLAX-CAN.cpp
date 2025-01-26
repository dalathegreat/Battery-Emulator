#include "../include.h"
#ifdef SOLAX_CAN
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "SOLAX-CAN.h"

#define NUMBER_OF_MODULES 0
#define BATTERY_TYPE 0x50
// If you are having BattVoltFault issues, configure the above values according to wiki page
// https://github.com/dalathegreat/Battery-Emulator/wiki/Solax-inverters

/* Do not change code below unless you are sure what you are doing */
static int16_t temperature_average = 0;
static uint8_t STATE = BATTERY_ANNOUNCE;
static unsigned long LastFrameTime = 0;
static uint8_t number_of_batteries = 1;
static uint16_t capped_capacity_Wh;
static uint16_t capped_remaining_capacity_Wh;

//CAN message translations from this amazing repository: https://github.com/rand12345/solax_can_bus

CAN_frame SOLAX_1801 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1801,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_1872 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1872,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_Limits
CAN_frame SOLAX_1873 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1873,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_PackData
CAN_frame SOLAX_1874 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1874,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_CellData
CAN_frame SOLAX_1875 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1875,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_Status
CAN_frame SOLAX_1876 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1876,
                        .data = {0x0, 0x0, 0xE2, 0x0C, 0x0, 0x0, 0xD7, 0x0C}};  //BMS_PackTemps
CAN_frame SOLAX_1877 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1877,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_1878 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1878,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  //BMS_PackStats
CAN_frame SOLAX_1879 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1879,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_187E = {.FD = false,  //Needed for Ultra
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x187E,
                        .data = {0x60, 0xEA, 0x0, 0x0, 0x64, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_187D = {.FD = false,  //Needed for Ultra
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x187D,
                        .data = {0x8B, 0x01, 0x0, 0x0, 0x8B, 0x1, 0x0, 0x0}};
CAN_frame SOLAX_187C = {.FD = false,  //Needed for Ultra
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x187C,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_187B = {.FD = false,  //Needed for Ultra
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x187B,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_187A = {.FD = false,  //Needed for Ultra
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x187A,
                        .data = {0x01, 0x50, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame SOLAX_1881 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1881,
                        .data = {0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // E.g.: 0 6 S B M S F A
CAN_frame SOLAX_1882 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1882,
                        .data = {0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // E.g.: 0 2 3 A B 0 5 2
CAN_frame SOLAX_100A001 = {.FD = false, .ext_ID = true, .DLC = 0, .ID = 0x100A001, .data = {}};

// __builtin_bswap64 needed to convert to ESP32 little endian format
// Byte[4] defines the requested contactor state: 1 = Closed , 0 = Open
#define Contactor_Open_Payload __builtin_bswap64(0x0200010000000000)
#define Contactor_Close_Payload __builtin_bswap64(0x0200010001000000)

void update_values_can_inverter() {  //This function maps all the values fetched from battery CAN to the correct CAN messages
  // If not receiveing any communication from the inverter, open contactors and return to battery announce state
  if (millis() - LastFrameTime >= SolaxTimeout) {
    datalayer.system.status.inverter_allows_contactor_closing = false;
    STATE = BATTERY_ANNOUNCE;
  }
  //Calculate the required values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  // Batteries might be larger than uint16_t value can take
  if (datalayer.battery.info.total_capacity_Wh > 65000) {
    capped_capacity_Wh = 65000;
  } else {
    capped_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
  }
  // Batteries might be larger than uint16_t value can take
  if (datalayer.battery.status.reported_remaining_capacity_Wh > 65000) {
    capped_remaining_capacity_Wh = 65000;
  } else {
    capped_remaining_capacity_Wh = datalayer.battery.status.reported_remaining_capacity_Wh;
  }

  //Put the values into the CAN messages
  //BMS_Limits
  SOLAX_1872.data.u8[0] = (uint8_t)datalayer.battery.info.max_design_voltage_dV;
  SOLAX_1872.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SOLAX_1872.data.u8[2] = (uint8_t)datalayer.battery.info.min_design_voltage_dV;
  SOLAX_1872.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  SOLAX_1872.data.u8[4] = (uint8_t)datalayer.battery.status.max_charge_current_dA;
  SOLAX_1872.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SOLAX_1872.data.u8[6] = (uint8_t)datalayer.battery.status.max_discharge_current_dA;
  SOLAX_1872.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);

  //BMS_PackData
  SOLAX_1873.data.u8[0] = (uint8_t)datalayer.battery.status.voltage_dV;  // OK
  SOLAX_1873.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  SOLAX_1873.data.u8[2] = (int8_t)datalayer.battery.status.current_dA;  // OK, Signed (Active current in Amps x 10)
  SOLAX_1873.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  SOLAX_1873.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);  //SOC (100.00%)
  //SOLAX_1873.data.u8[5] = //Seems like this is not required? Or shall we put SOC decimals here?
  SOLAX_1873.data.u8[6] = (uint8_t)(capped_remaining_capacity_Wh / 10);
  SOLAX_1873.data.u8[7] = ((capped_remaining_capacity_Wh / 10) >> 8);

  //BMS_CellData
  SOLAX_1874.data.u8[0] = (int8_t)datalayer.battery.status.temperature_max_dC;
  SOLAX_1874.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  SOLAX_1874.data.u8[2] = (int8_t)datalayer.battery.status.temperature_min_dC;
  SOLAX_1874.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);
  SOLAX_1874.data.u8[4] = (uint8_t)(datalayer.battery.info.max_cell_voltage_mV);
  SOLAX_1874.data.u8[5] = (datalayer.battery.info.max_cell_voltage_mV >> 8);
  SOLAX_1874.data.u8[6] = (uint8_t)(datalayer.battery.status.cell_min_voltage_mV);
  SOLAX_1874.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //BMS_Status
  SOLAX_1875.data.u8[0] = (uint8_t)temperature_average;
  SOLAX_1875.data.u8[1] = (temperature_average >> 8);
  SOLAX_1875.data.u8[2] = (uint8_t)NUMBER_OF_MODULES;  // Number of slave batteries
  SOLAX_1875.data.u8[4] = (uint8_t)0;                  // Contactor Status 0=off, 1=on.

  //BMS_PackTemps (strange name, since it has voltages?)
  SOLAX_1876.data.u8[0] = (int8_t)datalayer.battery.status.temperature_max_dC;
  SOLAX_1876.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  SOLAX_1876.data.u8[2] = (uint8_t)datalayer.battery.status.cell_max_voltage_mV;
  SOLAX_1876.data.u8[3] = (datalayer.battery.status.cell_max_voltage_mV >> 8);

  SOLAX_1876.data.u8[4] = (int8_t)datalayer.battery.status.temperature_min_dC;
  SOLAX_1876.data.u8[5] = (datalayer.battery.status.temperature_min_dC >> 8);
  SOLAX_1876.data.u8[6] = (uint8_t)datalayer.battery.status.cell_min_voltage_mV;
  SOLAX_1876.data.u8[7] = (datalayer.battery.status.cell_min_voltage_mV >> 8);

  //Unknown
  SOLAX_1877.data.u8[4] = (uint8_t)BATTERY_TYPE;  // Battery type (Default 0x50)
  SOLAX_1877.data.u8[6] = (uint8_t)0x22;          // Firmware version?
  SOLAX_1877.data.u8[7] =
      (uint8_t)0x02;  // The above firmware version applies to:02 = Master BMS, 10 = S1, 20 = S2, 30 = S3, 40 = S4

  //BMS_PackStats
  SOLAX_1878.data.u8[0] = (uint8_t)(datalayer.battery.status.voltage_dV);
  SOLAX_1878.data.u8[1] = ((datalayer.battery.status.voltage_dV) >> 8);

  SOLAX_1878.data.u8[4] = (uint8_t)datalayer.battery.info.total_capacity_Wh;
  SOLAX_1878.data.u8[5] = (datalayer.battery.info.total_capacity_Wh >> 8);
  SOLAX_1878.data.u8[6] = (datalayer.battery.info.total_capacity_Wh >> 16);
  SOLAX_1878.data.u8[7] = (datalayer.battery.info.total_capacity_Wh >> 24);

  // BMS_Answer
  SOLAX_1801.data.u8[0] = 2;
  SOLAX_1801.data.u8[2] = 1;
  SOLAX_1801.data.u8[4] = 1;

  //Ultra messages
  SOLAX_187E.data.u8[0] = (uint8_t)datalayer.battery.status.reported_remaining_capacity_Wh;
  SOLAX_187E.data.u8[1] = (datalayer.battery.status.reported_remaining_capacity_Wh >> 8);
  SOLAX_187E.data.u8[2] = (datalayer.battery.status.reported_remaining_capacity_Wh >> 16);
  SOLAX_187E.data.u8[3] = (datalayer.battery.status.reported_remaining_capacity_Wh >> 24);
  SOLAX_187E.data.u8[5] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
}

void transmit_can_inverter() {
  // No periodic sending used on this protocol, we react only on incoming CAN messages!
}

void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {

  if (rx_frame.ID == 0x1871) {
    datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
  }

  if (rx_frame.ID == 0x1871 && rx_frame.data.u8[0] == (0x01) ||
      rx_frame.ID == 0x1871 && rx_frame.data.u8[0] == (0x02)) {
    LastFrameTime = millis();
    switch (STATE) {
      case (BATTERY_ANNOUNCE):
#ifdef DEBUG_LOG
        logging.println("Solax Battery State: Announce");
#endif
        datalayer.system.status.inverter_allows_contactor_closing = false;
        SOLAX_1875.data.u8[4] = (0x00);  // Inform Inverter: Contactor 0=off, 1=on.
        for (uint8_t i = 0; i <= number_of_batteries; i++) {
          transmit_can_frame(&SOLAX_187E, can_config.inverter);
          transmit_can_frame(&SOLAX_187A, can_config.inverter);
          transmit_can_frame(&SOLAX_1872, can_config.inverter);
          transmit_can_frame(&SOLAX_1873, can_config.inverter);
          transmit_can_frame(&SOLAX_1874, can_config.inverter);
          transmit_can_frame(&SOLAX_1875, can_config.inverter);
          transmit_can_frame(&SOLAX_1876, can_config.inverter);
          transmit_can_frame(&SOLAX_1877, can_config.inverter);
          transmit_can_frame(&SOLAX_1878, can_config.inverter);
        }
        transmit_can_frame(&SOLAX_100A001, can_config.inverter);  //BMS Announce
        // Message from the inverter to proceed to contactor closing
        // Byte 4 changes from 0 to 1
        if (rx_frame.data.u64 == Contactor_Close_Payload)
          STATE = WAITING_FOR_CONTACTOR;
        break;

      case (WAITING_FOR_CONTACTOR):
        SOLAX_1875.data.u8[4] = (0x00);  // Inform Inverter: Contactor 0=off, 1=on.
        transmit_can_frame(&SOLAX_187E, can_config.inverter);
        transmit_can_frame(&SOLAX_187A, can_config.inverter);
        transmit_can_frame(&SOLAX_1872, can_config.inverter);
        transmit_can_frame(&SOLAX_1873, can_config.inverter);
        transmit_can_frame(&SOLAX_1874, can_config.inverter);
        transmit_can_frame(&SOLAX_1875, can_config.inverter);
        transmit_can_frame(&SOLAX_1876, can_config.inverter);
        transmit_can_frame(&SOLAX_1877, can_config.inverter);
        transmit_can_frame(&SOLAX_1878, can_config.inverter);
        transmit_can_frame(&SOLAX_1801, can_config.inverter);  // Announce that the battery will be connected
        STATE = CONTACTOR_CLOSED;                              // Jump to Contactor Closed State
#ifdef DEBUG_LOG
        logging.println("Solax Battery State: Contactor Closed");
#endif
        break;

      case (CONTACTOR_CLOSED):
        datalayer.system.status.inverter_allows_contactor_closing = true;
        SOLAX_1875.data.u8[4] = (0x01);  // Inform Inverter: Contactor 0=off, 1=on.
        transmit_can_frame(&SOLAX_187E, can_config.inverter);
        transmit_can_frame(&SOLAX_187A, can_config.inverter);
        transmit_can_frame(&SOLAX_1872, can_config.inverter);
        transmit_can_frame(&SOLAX_1873, can_config.inverter);
        transmit_can_frame(&SOLAX_1874, can_config.inverter);
        transmit_can_frame(&SOLAX_1875, can_config.inverter);
        transmit_can_frame(&SOLAX_1876, can_config.inverter);
        transmit_can_frame(&SOLAX_1877, can_config.inverter);
        transmit_can_frame(&SOLAX_1878, can_config.inverter);
        // Message from the inverter to open contactor
        // Byte 4 changes from 1 to 0
        if (rx_frame.data.u64 == Contactor_Open_Payload) {
          set_event(EVENT_INVERTER_OPEN_CONTACTOR, 0);
          STATE = BATTERY_ANNOUNCE;
        }
        break;
    }
  }

  if (rx_frame.ID == 0x1871 && rx_frame.data.u64 == __builtin_bswap64(0x0500010000000000)) {
    transmit_can_frame(&SOLAX_1881, can_config.inverter);
    transmit_can_frame(&SOLAX_1882, can_config.inverter);
#ifdef DEBUG_LOG
    logging.println("1871 05-frame received from inverter");
#endif
  }
  if (rx_frame.ID == 0x1871 && rx_frame.data.u8[0] == (0x03)) {
#ifdef DEBUG_LOG
    logging.println("1871 03-frame received from inverter");
#endif
  }
}
void setup_inverter(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.inverter_protocol, "SolaX Triple Power LFP over CAN bus", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
  datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
}
#endif
