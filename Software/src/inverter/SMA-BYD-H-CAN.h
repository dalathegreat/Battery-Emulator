#ifndef SMA_BYD_H_CAN_H
#define SMA_BYD_H_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"
#include "src/devboard/hal/hal.h"

#ifdef SMA_BYD_H_CAN
#define SELECTED_INVERTER_CLASS SmaBydHInverter
#endif

class SmaBydHInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "BYD over SMA CAN";

  virtual bool controls_contactor() { return true; }
  virtual bool allows_contactor_closing() { return digitalRead(INVERTER_CONTACTOR_ENABLE_PIN) == 1; }

 private:
  static const int READY_STATE = 0x03;
  static const int STOP_STATE = 0x02;

  void transmit_can_init();

  unsigned long previousMillis100ms = 0;

  uint8_t pairing_events = 0;
  uint32_t inverter_time = 0;
  uint16_t inverter_voltage = 0;
  int16_t inverter_current = 0;
  uint16_t timeWithoutInverterAllowsContactorClosing = 0;
  static const int THIRTY_MINUTES = 1200;

  //Actual content messages
  CAN_frame SMA_158 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x158,
                       .data = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x6A, 0xAA, 0xAA}};
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
  CAN_frame SMA_4D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x4D8,
                       .data = {0x09, 0xFD, 0x00, 0x00, 0x00, 0xA8, 0x02, 0x08}};
  CAN_frame SMA_518 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x518,
                       .data = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}};
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

  int16_t temperature_average = 0;
  uint16_t ampere_hours_remaining = 0;
};

#endif
