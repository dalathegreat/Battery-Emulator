#ifndef SMA_BYD_HVS_CAN_H
#define SMA_BYD_HVS_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"
#include "src/devboard/hal/hal.h"

#ifdef SMA_BYD_HVS_CAN
#define SELECTED_INVERTER_CLASS SmaBydHvsInverter
#endif

class SmaBydHvsInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "BYD Battery-Box HVS over SMA CAN";

  virtual bool controls_contactor() { return true; }
  virtual bool allows_contactor_closing() { return digitalRead(INVERTER_CONTACTOR_ENABLE_PIN) == 1; }

 private:
  static const int READY_STATE = 0x03;
  static const int STOP_STATE = 0x02;
  static const int THIRTY_MINUTES = 1200;

  unsigned long previousMillis100ms = 0;
  unsigned long previousMillisBatch = 0;
  uint8_t batch_send_index = 0;
  const uint8_t delay_between_batches_ms =
      7;  //TODO, tweak to as low as possible before performance issues/crashes appear
  bool transmit_can_init = false;

  uint8_t pairing_events = 0;
  uint32_t inverter_time = 0;
  uint16_t inverter_voltage = 0;
  int16_t inverter_current = 0;
  uint16_t timeWithoutInverterAllowsContactorClosing = 0;

  //Actual content messages
  CAN_frame SMA_158 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x158,  // All 0xAA, no faults active
                       .data = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
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
  CAN_frame SMA_4D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x4D8,
                       .data = {0x10, 0x62, 0x00, 0x16, 0x01, 0x68, 0x03, 0x08}};
  CAN_frame SMA_518 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x518,
                       .data = {0x01, 0x4A, 0x01, 0x25, 0xFF, 0xFF, 0xFF, 0xFF}};

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

  int16_t discharge_current = 0;
  int16_t charge_current = 0;
  int16_t temperature_average = 0;
  uint16_t ampere_hours_remaining = 0;
};

#endif
