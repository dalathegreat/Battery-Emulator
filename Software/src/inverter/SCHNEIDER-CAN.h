#ifndef SCHNEIDER_CAN_H
#define SCHNEIDER_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#define CAN_INVERTER_SELECTED
#define SELECTED_INVERTER_CLASS SchneiderInverter

#define STATE_OFFLINE 0
#define STATE_STANDBY 1
#define STATE_STARTING 2
#define STATE_ONLINE 3
#define STATE_FAULTED 4

// Same enumerations used for Fault and Warning
#define FAULTS_CHARGE_OVERCURRENT 0
#define FAULTS_DISCHARGE_OVERCURRENT 1
#define FAULTS_OVER_TEMPERATURE 2
#define FAULTS_UNDER_TEMPERATURE 3
#define FAULTS_OVER_VOLTAGE 4
#define FAULTS_UNDER_VOLTAGE 5
#define FAULTS_CELL_IMBALANCE 6
#define FAULTS_INTERNAL_COM_ERROR 7
#define FAULTS_SYSTEM_ERROR 8

// Commands. Bit0 forced charge request. Bit1 charge permitted. Bit2 discharge permitted. Bit3 Stop
#define COMMAND_ONLY_CHARGE_ALLOWED 0x02
#define COMMAND_ONLY_DISCHARGE_ALLOWED 0x04
#define COMMAND_CHARGE_AND_DISCHARGE_ALLOWED 0x06
#define COMMAND_STOP 0x08

class SchneiderInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);

 private:
  /* Do not change code below unless you are sure what you are doing */
  unsigned long previousMillis10s = 0;    // will store last time a 10s CAN Message was send
  unsigned long previousMillis2s = 0;     // will store last time a 2s CAN Message was send
  unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was send

  CAN_frame SE_320 = {.FD = false,  //SE BMS Protocol Version
                      .ext_ID = true,
                      .DLC = 2,
                      .ID = 0x320,
                      .data = {0x00, 0x02}};  //TODO: How do we reply with Protocol Version: 0x0002 ?
  CAN_frame SE_321 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x321,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_322 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x322,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_323 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x323,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_324 = {.FD = false, .ext_ID = true, .DLC = 4, .ID = 0x324, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_325 = {.FD = false, .ext_ID = true, .DLC = 6, .ID = 0x325, .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_326 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x326,
                      .data = {0x00, STATE_STARTING, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_327 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x327,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_328 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x328,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_330 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x330,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_331 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x331,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_332 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x332,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SE_333 = {.FD = false,
                      .ext_ID = true,
                      .DLC = 8,
                      .ID = 0x333,
                      .data = {0x53, 0x45, 0x42, 0x4D, 0x53, 0x00, 0x00, 0x00}};  //SEBMS

  int16_t temperature_average = 0;
  uint16_t remaining_capacity_ah = 0;
  uint16_t fully_charged_capacity_ah = 0;
  uint16_t commands = 0;
  uint16_t warnings = 0;
  uint16_t faults = 0;
  uint16_t state = 0;
};

#endif
