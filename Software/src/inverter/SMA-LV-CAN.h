#ifndef SMA_LV_CAN_H
#define SMA_LV_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef SMA_LV_CAN
#define SELECTED_INVERTER_CLASS SmaLvInverter
#endif

class SmaLvInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "SMA Low Voltage (48V) protocol via CAN";

 private:
  static const int READY_STATE = 0x03;
  static const int STOP_STATE = 0x02;

  unsigned long previousMillis100ms = 0;

  static const int VOLTAGE_OFFSET_DV = 40;  //Offset in deciVolt from max charge voltage and min discharge voltage
  static const int MAX_VOLTAGE_DV = 630;
  static const int MIN_VOLTAGE_DV = 41;

  //Actual content messages
  CAN_frame SMA_00F = {.FD = false,  // Emergency stop message
                       .ext_ID = false,
                       .DLC = 8,  //Documentation unclear, should message even have any content?
                       .ID = 0x00F,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_351 = {.FD = false,  // Battery charge voltage, charge/discharge limit, min discharge voltage
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x351,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_355 = {.FD = false,  // SOC, SOH, HiResSOC
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x355,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_356 = {.FD = false,  // Battery voltage, current, temperature
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x356,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_35A = {.FD = false,  // Alarms & Warnings
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x35A,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_35B = {.FD = false,  // Events
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x35B,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_35E = {.FD = false,  // Manufacturer ASCII
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x35E,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SMA_35F = {.FD = false,  // Battery Type, version, capacity, ID
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x35F,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  int16_t temperature_average = 0;
};

#endif
