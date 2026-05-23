#ifndef SMA_SBS_BYD_CAN_H
#define SMA_SBS_BYD_CAN_H

#include "../devboard/hal/hal.h"
#include "SmaInverterBase.h"

class SmaSBSBydHvsInverter : public SmaInverterBase {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "SMA SBS compatible BYD Battery-Box HVS";

  virtual bool controls_contactor() { return true; }

 private:
  static const int READY_STATE = 0x03;
  static const int STOP_STATE = 0x02;
  static const int THIRTY_MINUTES = 1200;
  unsigned long previousMillis60s = 0;
  unsigned long previousMillis100ms = 0;
  unsigned long previousMillisBatch = 0;
  const uint8_t delay_between_batches_ms =
      7;  //TODO, tweak to as low as possible before performance issues/crashes appear
  bool transmit_can_init = false;
  bool transmit_can_batch_finished = false;

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
                       .data = {0x01, 0x4A, 0x01, 0x25, 0x10, 0x10, 0xFF, 0xFF}};

  // Pairing/Battery setup information

  CAN_frame SMA_558 = {
      .FD = false,  //Pairing first message
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x558,
      .data = {0x03, 0x24, 0x00, 0x04, 0x00, 0x66, 0x04, 0x09}};  //SW Ver. 3.24.0.R,	10,24kWh, 4 modules
  CAN_frame SMA_598 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x598,
                       .data = {0x12, 0xD6, 0x43, 0xA4, 0x00, 0x00, 0x00, 0x00}};  //B0-4 Serial 316031908
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

  int16_t discharge_current = 0;
  int16_t charge_current = 0;
  int16_t temperature_average = 0;
  uint16_t ampere_hours_remaining = 0;
};

#endif
