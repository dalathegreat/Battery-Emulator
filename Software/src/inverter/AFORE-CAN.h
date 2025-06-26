#ifndef AFORE_CAN_H
#define AFORE_CAN_H
#include "../include.h"

#ifdef AFORE_CAN
#define SELECTED_INVERTER_CLASS AforeCanInverter
#endif

#include "CanInverterProtocol.h"

class AforeCanInverter : public CanInverterProtocol {
 public:
  void setup();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  void update_values();
  static constexpr char* Name = "Afore battery over CAN";

 private:
  /* The code is following the Afore 2.3 CAN standard, little-endian, 500kbps, from 2023.08.07 */
  uint8_t inverter_status =
      0;  //0 = init, 1 = standby, 2 = starting, 3 = grid connected, 4 off-grid, 5 diesel generator, 6 grid connected, but disconnected, 7off grid and disconnected, 8 = power failure processing, 9 = power off, 10 = Failure
  bool time_to_send_info = false;
  uint8_t char0 = 0;
  uint8_t char1 = 0;
  uint8_t char2 = 0;
  uint8_t char3 = 0;
  uint8_t char4 = 0;
  //Actual content messages
  CAN_frame AFORE_350 = {.FD = false,  // Operation information
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x350,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_351 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x351,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_352 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x352,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_353 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x353,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_354 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x354,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_355 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x355,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_356 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x356,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_357 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x357,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_358 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x358,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame AFORE_359 = {.FD = false,  // Serial number 0-7
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x359,
                         .data = {0x62, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79, 0x2D}};  // Battery-
  CAN_frame AFORE_35A = {.FD = false,                                                // Serial number 8-15
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x35A,
                         .data = {0x65, 0x6D, 0x75, 0x6C, 0x61, 0x74, 0x6F, 0x72}};  // Emulator
};

#endif
