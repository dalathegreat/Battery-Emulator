#ifndef FORD_MACH_E_BATTERY_H
#define FORD_MACH_E_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class FordMachEBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Ford Mustang Mach-E battery";

 private:
  static const int MAX_PACK_VOLTAGE_96S_DV = 4140;
  static const int MIN_PACK_VOLTAGE_96S_DV = 2680;
  static const int MAX_PACK_VOLTAGE_94S_DV = 4072;
  static const int MIN_PACK_VOLTAGE_94S_DV = 2612;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2900;

  static const int RAMPDOWN_SOC = 900;  // 90.0 SOC% to start ramping down from max charge power towards 0 at 100.00%
  static const int RAMPDOWNPOWERALLOWED = 8000;  // What power we ramp down from towards top balancing
  static const int FLOAT_MAX_POWER_W = 200;      // W, what power to allow for top balancing battery
  static const int FLOAT_START_MV = 20;          // mV, how many mV under overvoltage to start float charging

  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was send
  unsigned long previousMillis30 = 0;    // will store last time a 10ms CAN Message was send
  unsigned long previousMillis50 = 0;    // will store last time a 100ms CAN Message was send
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
  unsigned long previousMillis250 = 0;   // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was send
  unsigned long previousMillis10s = 0;   // will store last time a 10s CAN Message was send

  int16_t cell_temperature[6] = {0};
  int16_t maximum_temperature = 0;
  int16_t minimum_temperature = 0;
  uint16_t battery_soc = 5000;
  uint16_t battery_soh = 99;
  uint16_t battery_voltage = 370;
  int16_t battery_current = 0;
  uint16_t maximum_cellvoltage_mV = 3700;
  uint16_t minimum_cellvoltage_mV = 3700;

  uint8_t counter_30ms = 0;
  uint8_t counter_8_30ms = 0;
  uint16_t pid_reply = 0;

  uint16_t polled_12V = 12000;

  CAN_frame FORD_PID_REQUEST_7DF = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x7DF,
                                    .data = {0x02, 0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_PID_ACK = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x7DF,
                            .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  //Message needed for contactor closing
  CAN_frame FORD_25B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x25B,
                        .data = {0x01, 0xF4, 0x09, 0xF4, 0xE0, 0x00, 0x80, 0x00}};
  CAN_frame FORD_185 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x185,
                        .data = {0x03, 0x4E, 0x75, 0x32, 0x00, 0x80, 0x00, 0x00}};
  //Messages to emulate full vehicle
  /*
  CAN_frame FORD_47 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x047,
                       .data = {0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_48 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x048,
                       .data = {0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_4C = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x04C,
                       .data = {0x70, 0xAA, 0xBF, 0xDE, 0xCC, 0xEC, 0x00, 0x00}};
  CAN_frame FORD_5A = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x05A,
                       .data = {0x00, 0x00, 0x00, 0x0B, 0xF2, 0x90, 0x10, 0x00}};
  CAN_frame FORD_77 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x077,
                       .data = {0x00, 0x00, 0x0F, 0xFE, 0xFF, 0xFF, 0xFB, 0xFE}};
  CAN_frame FORD_7D = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x07D,
                       .data = {0x00, 0x00, 0xF0, 0xF0, 0x00, 0x3F, 0xEF, 0xFE}};
  CAN_frame FORD_7E = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x07E,
                       .data = {0x00, 0x00, 0x3E, 0x80, 0x00, 0x04, 0x00, 0x00}};
  CAN_frame FORD_7F = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x07F,
                       .data = {0x00, 0x00, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_156 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x156,
                        .data = {0x4B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}};
  CAN_frame FORD_165 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x165,
                        .data = {0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_166 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x166,
                        .data = {0x00, 0x00, 0x00, 0x01, 0x5C, 0x89, 0x00, 0x00}};
  CAN_frame FORD_167 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x167,
                        .data = {0x00, 0x80, 0x00, 0x11, 0xFF, 0xE0, 0x00, 0x00}};
  CAN_frame FORD_175 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x175,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_176 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x176,
                        .data = {0x00, 0x0E, 0xF0, 0x10, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_178 = {.FD = false,  //Static content
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x175,
                        .data = {0x01, 0xB6, 0x02, 0x00, 0x4E, 0x46, 0xC6, 0x17}};
  CAN_frame FORD_12F = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x12F,
                        .data = {0x0A, 0xF8, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

  CAN_frame FORD_200 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x200,
                        .data = {0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x70}};
  CAN_frame FORD_203 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x203,
                        .data = {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_204 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x204,
                        .data = {0xD4, 0x00, 0x7D, 0x00, 0x00, 0xF7, 0x00, 0x00}};
  CAN_frame FORD_217 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x217,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_230 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x230,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};

  CAN_frame FORD_2EC = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x2EC,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}};
  CAN_frame FORD_332 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x332,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_333 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x333,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_3C3 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3C3,
                        .data = {0x5C, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00}};
  CAN_frame FORD_415 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x415,
                        .data = {0x00, 0x00, 0xC0, 0xFC, 0x0F, 0xFE, 0xEF, 0xFE}};
  CAN_frame FORD_42B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x42B,
                        .data = {0xCB, 0xBE, 0x00, 0x02, 0x00, 0x00, 0xCE, 0x00}};
  CAN_frame FORD_42C = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x42C,
                        .data = {0x80, 0x02, 0x00, 0x00, 0x19, 0xA0, 0x00, 0x00}};
  CAN_frame FORD_42F = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x42F,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_43D = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x43D,
                        .data = {0x00, 0x00, 0xDC, 0x00, 0x00, 0x77, 0x00, 0x00}};
  CAN_frame FORD_442 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x442,
                        .data = {0x4E, 0x20, 0x78, 0x7E, 0x7C, 0x00, 0x00, 0x40}};
  CAN_frame FORD_48F = {.FD = false,  //Only sent in active charging logs (OBC?)
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x48F,
                        .data = {0x30, 0x4E, 0x20, 0x80, 0x00, 0x00, 0x80, 0x00}};
  CAN_frame FORD_4B0 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x4B0,
                        .data = {0x01, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0xF0}};
  CAN_frame FORD_581 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x4B0,
                        .data = {0x81, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
                        */
};

#endif
