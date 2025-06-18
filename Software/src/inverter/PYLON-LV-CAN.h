#ifndef PYLON_LV_CAN_H
#define PYLON_LV_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef PYLON_LV_CAN
#define SELECTED_INVERTER_CLASS PylonLvInverter
#endif

class PylonLvInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "Pylontech LV battery over CAN bus";

 private:
  void send_system_data();
  void send_setup_info();
  int16_t warning_threshold_of_min(int16_t min_val, int16_t max_val);

  static const int PACK_NUMBER = 0x01;

  // 80 means after reaching 80% of a nominal value a warning is produced (e.g. 80% of max current)
  static const int WARNINGS_PERCENT = 80;

  static constexpr const char* MANUFACTURER_NAME = "BatEmuLV";

  unsigned long previousMillis1000ms = 0;

  CAN_frame PYLON_351 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 6,
                         .ID = 0x351,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_355 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x355, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_356 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 6,
                         .ID = 0x356,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_359 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 7,
                         .ID = 0x359,
                         .data = {0x00, 0x00, 0x00, 0x00, PACK_NUMBER, 'P', 'N'}};
  CAN_frame PYLON_35C = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x35C, .data = {0x00, 0x00}};
  CAN_frame PYLON_35E = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x35E,
                         .data = {
                             MANUFACTURER_NAME[0],
                             MANUFACTURER_NAME[1],
                             MANUFACTURER_NAME[2],
                             MANUFACTURER_NAME[3],
                             MANUFACTURER_NAME[4],
                             MANUFACTURER_NAME[5],
                             MANUFACTURER_NAME[6],
                             MANUFACTURER_NAME[7],
                         }};
};

#endif
