#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H

#include "CanInverterProtocol.h"

class SolaxInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  bool setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "SolaX Triple Power LFP over CAN bus";

 private:
  static const int NUMBER_OF_MODULES = 0;
  static const int BATTERY_TYPE = 0x50;
  // If you are having BattVoltFault issues, configure the above values according to wiki page
  // https://github.com/dalathegreat/Battery-Emulator/wiki/Solax-inverters

  // Timeout in milliseconds
  static const int SolaxTimeout = 2000;

  //SOLAX BMS States Definition
  static const int BATTERY_ANNOUNCE = 0;
  static const int WAITING_FOR_CONTACTOR = 1;
  static const int CONTACTOR_CLOSED = 2;
  static const int FAULT_SOLAX = 3;
  static const int UPDATING_FW = 4;

  int16_t temperature_average = 0;
  uint8_t STATE = BATTERY_ANNOUNCE;
  unsigned long LastFrameTime = 0;
  uint8_t number_of_batteries = 1;
  uint16_t capped_capacity_Wh;
  uint16_t capped_remaining_capacity_Wh;

  int configured_number_of_modules = 0;
  int configured_battery_type = 0;
  // If true, the integration will ignore the inverter's requests to open the
  // battery contactors. Useful for batteries that can't open contactors on
  // request.
  bool configured_ignore_contactors = false;

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
};

#endif
