#ifndef PYLON_CAN_H
#define PYLON_CAN_H

#include "CanInverterProtocol.h"

class PylonInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  bool setup() override;
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Pylontech HV battery over CAN bus";

 private:
  void send_system_data();
  void send_setup_info();

  /* Some inverters need to see a specific amount of cells/modules to emulate a specific Pylon battery.
     Change the following only if your inverter is generating fault codes about voltage range, in the Settings */
  static const int TOTAL_CELL_AMOUNT = 120;
  static const int MODULES_IN_SERIES = 4;
  static const int CELLS_PER_MODULE = 30;
  static const int VOLTAGE_LEVEL = 384;
  static const int AH_CAPACITY = 37;

  /* Do not change code below unless you are sure what you are doing */
  //Actual content messages
  CAN_frame PYLON_731X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7310,  //ID changes based on user config
                          .data = {0x01, 0x00, 0x02, 0x01, 0x01, 0x02, 0x00, 0x00}};
  CAN_frame PYLON_732X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x7320,  //ID changes based on user config
                          .data = {TOTAL_CELL_AMOUNT, (uint8_t)(TOTAL_CELL_AMOUNT >> 8), MODULES_IN_SERIES,
                                   CELLS_PER_MODULE, (uint8_t)(VOLTAGE_LEVEL & 0x00FF), (uint8_t)(VOLTAGE_LEVEL >> 8),
                                   (uint8_t)(AH_CAPACITY & 0x00FF), (uint8_t)(AH_CAPACITY >> 8)}};
  CAN_frame PYLON_421X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4210,  //ID changes based on user config
                          .data = {0xA5, 0x09, 0x30, 0x75, 0x9D, 0x04, 0x2E, 0x64}};
  CAN_frame PYLON_422X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4220,  //ID changes based on user config
                          .data = {0x8C, 0x0A, 0xE9, 0x07, 0x4A, 0x79, 0x4A, 0x79}};
  CAN_frame PYLON_423X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4230,  //ID changes based on user config
                          .data = {0xDF, 0x0C, 0xDA, 0x0C, 0x03, 0x00, 0x06, 0x00}};
  CAN_frame PYLON_424X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4240,  //ID changes based on user config
                          .data = {0x7E, 0x04, 0x62, 0x04, 0x11, 0x00, 0x03, 0x00}};
  CAN_frame PYLON_425X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4250,  //ID changes based on user config
                          .data = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_426X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4260,  //ID changes based on user config
                          .data = {0xAC, 0xC7, 0x74, 0x27, 0x03, 0x00, 0x02, 0x00}};
  CAN_frame PYLON_427X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4270,  //ID changes based on user config
                          .data = {0x7E, 0x04, 0x62, 0x04, 0x05, 0x00, 0x01, 0x00}};
  CAN_frame PYLON_428X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4280,  //ID changes based on user config
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_429X = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4290,  //ID changes based on user config
                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  uint16_t discharge_cutoff_voltage_dV = 0;
  uint16_t charge_cutoff_voltage_dV = 0;

  static const int VOLTAGE_OFFSET_DV = 20;  // Small offset voltage to avoid generating voltage events
};

#endif
