#ifndef FERROAMP_CAN_H
#define FERROAMP_CAN_H

#include "CanInverterProtocol.h"

class FerroampCanInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);

  static constexpr const char* Name = "Ferroamp Pylon battery over CAN bus";

 private:
  void send_system_data();
  void send_setup_info();

  /* Some inverters need to see a specific amount of cells/modules to emulate a specific Pylon battery.
Change the following only if your inverter is generating fault codes about voltage range */
  static const int TOTAL_CELL_AMOUNT = 576;  //6 * 3 Force-H3-like modules * 32 cells/module
  static const int MODULES_IN_SERIES = 3;
  static const int CELLS_PER_MODULE = 32;
  static const int VOLTAGE_LEVEL = 308;
  static const int AH_CAPACITY = 50;

  static const int PYLON_CELL_SPREAD_mV = 3;
  static const int PYLON_IDLE_CURRENT_THRESHOLD_dA = 5;  //0.5 A
  static const int TEMPERATURE_OFFSET_dC = 1000;

  //Actual content messages
  CAN_frame FERROAMP_7311 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x7311,
                             .data = {0x02, 0x00, 0x0A, 0x04, 0x01, 0x06, 0x35, 0x06}};
  CAN_frame FERROAMP_7321 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x7321,
      .data = {(TOTAL_CELL_AMOUNT & 0xFF), (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES, CELLS_PER_MODULE,
               (uint8_t)(VOLTAGE_LEVEL & 0x00FF), (uint8_t)(VOLTAGE_LEVEL >> 8), (uint8_t)(AH_CAPACITY & 0x00FF),
               (uint8_t)(AH_CAPACITY >> 8)}};
  CAN_frame FERROAMP_7331 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x7331,
                             .data = {0x50, 0x59, 0x4C, 0x4F, 0x4E, 0x54, 0x45, 0x43}};
  CAN_frame FERROAMP_7341 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x7341,
                             .data = {0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //Startup/control command replies observed in real Pylontech/ESO15 cold-start logs.
  CAN_frame FERROAMP_8231 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x8231,
                             .data = {0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FERROAMP_8251 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 1,
                             .ID = 0x8251,
                             .data = {0xAA}};
  CAN_frame FERROAMP_8271 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 1,
                             .ID = 0x8271,
                             .data = {0xAA}};

  CAN_frame FERROAMP_4211 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4211,
                             .data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
  CAN_frame FERROAMP_4221 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4221,
                             .data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
  CAN_frame FERROAMP_4231 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4231,
                             .data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
  CAN_frame FERROAMP_4241 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4241,
                             .data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
  CAN_frame FERROAMP_4251 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4251,
                             .data = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FERROAMP_4261 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4261,
                             .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, (uint8_t)(MODULES_IN_SERIES - 1), 0x00}};
  CAN_frame FERROAMP_4271 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4271,
                             .data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
  CAN_frame FERROAMP_4281 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4281,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FERROAMP_4291 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x4291,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FERROAMP_42A1 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x42A1,
                             .data = {0xE8, 0x03, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FERROAMP_42B1 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x42B1,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x64}};
  CAN_frame FERROAMP_42C1 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x42C1,
                             .data = {0xE8, 0x03, 0x00, 0x00, 0xB8, 0x10, 0xC4, 0x09}};
  CAN_frame FERROAMP_42D1 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x42D1,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}};
  CAN_frame FERROAMP_42E1 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x42E1,
                             .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (uint8_t)(MODULES_IN_SERIES - 1), 0x00}};

  uint16_t cell_tweaked_max_voltage_mV = 3300;
  uint16_t cell_tweaked_min_voltage_mV = 3300;
  uint8_t pylon_heartbeat = 0xE8;
};

#endif
