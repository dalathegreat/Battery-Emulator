#ifndef STELLANTIS_ECMP_BATTERY_H
#define STELLANTIS_ECMP_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#define BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS EcmpBattery

class EcmpBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4546;
  static const int MIN_PACK_VOLTAGE_DV = 3210;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2700;
#define STOPPED 0
  bool battery_RelayOpenRequest = false;
  bool battery_InterlockOpen = false;
  uint8_t ContactorResetStatemachine = 0;
  uint8_t CollisionResetStatemachine = 0;
  uint8_t IsolationResetStatemachine = 0;
  uint8_t counter_10ms = 0;
  uint8_t counter_20ms = 0;
  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t counter_010 = 0;
  uint16_t ticks_552 = 60513;  //Hmm, will overflow a while after start
  uint8_t battery_MainConnectorState = 0;
  int16_t battery_current = 0;
  uint16_t battery_voltage = 370;
  uint16_t battery_soc = 0;
  uint16_t cellvoltages[108];
  uint16_t battery_AllowedMaxChargeCurrent = 0;
  uint16_t battery_AllowedMaxDischargeCurrent = 0;
  uint16_t battery_insulationResistanceKOhm = 0;
  int16_t battery_highestTemperature = 0;
  int16_t battery_lowestTemperature = 0;

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis200 = 0;   // will store last time a 200ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent

  CAN_frame ECMP_010 = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x010, .data = {0xB4}};
  CAN_frame ECMP_041 = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x041, .data = {0x00}};
  CAN_frame ECMP_0A6 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x0A6, .data = {0x02, 0x00}};
  CAN_frame ECMP_0F0 = {.FD = false,  //VCU (Common)
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0F0,
                        .data = {0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}};
  CAN_frame ECMP_0F2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0F2,
                        .data = {0x7D, 0x00, 0x4E, 0x20, 0x00, 0x00, 0x90, 0x0D}};
  CAN_frame ECMP_110 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x110,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4E, 0x20}};
  CAN_frame ECMP_111 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x111,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_112 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x112,
                        .data = {0x4E, 0x20, 0x00, 0x0F, 0xA0, 0x7D, 0x00, 0x0A}};
  CAN_frame ECMP_114 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x114,
                        .data = {0x00, 0x00, 0x00, 0x7D, 0x07, 0xD0, 0x7D, 0x00}};
  CAN_frame ECMP_0C5 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0C5,
                        .data = {0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_125 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x125,
                        .data = {0x0E, 0x50, 0x0E, 0xB9, 0x92, 0x45, 0x20, 0x00}};
  CAN_frame ECMP_127 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x127,
                        .data = {0x6D, 0x59, 0xF4, 0x9B, 0xE2, 0xCD, 0xC7, 0xD0}};
  CAN_frame ECMP_129 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x129,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_17B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x17B,
                        .data = {0x00, 0x00, 0x00, 0x7F, 0x98, 0x00, 0x00, 0x0F}};
  CAN_frame ECMP_230 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x230,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_27A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x27A,
                        .data = {0x4F, 0x58, 0x00, 0x02, 0x24, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_31E = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x31E,
                        .data = {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08}};
  CAN_frame ECMP_31D = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x31D,
                        .data = {0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42}};
  CAN_frame ECMP_345 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x345,
                        .data = {0x45, 0x51, 0x00, 0x04, 0xDD, 0x00, 0x02, 0x31}};
  CAN_frame ECMP_351 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x351,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x0E, 0xA0, 0x00, 0x0E}};
  CAN_frame ECMP_372 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x372,
                        .data = {0x9A, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_37F = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x37F,
                        .data = {0x42, 0x45, 0x4E, 0x43, 0x42, 0x00, 0x43, 0x42}};
  CAN_frame ECMP_382 = {.FD = false,  //BSI_Info (VCU) PSA specific
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x382,
                        .data = {0x02, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_383 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x383,
                        .data = {0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_3A2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3A2,
                        .data = {0x01, 0x48, 0x00, 0x00, 0x81, 0x00, 0x08, 0x02}};
  CAN_frame ECMP_3A3 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3A3,
                        .data = {0x49, 0x49, 0x40, 0x00, 0xDD, 0x08, 0x00, 0x0F}};
  CAN_frame ECMP_439 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x439,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_486 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x486,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_552 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x552,
                        .data = {0x0B, 0xC8, 0xEC, 0x4D, 0x00, 0xD4, 0x2C, 0xFE}};
  CAN_frame ECMP_591 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x591,
                        .data = {0x38, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_786 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x786,
                        .data = {0x38, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_794 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x794,
                        .data = {0x38, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_POLL = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x6B4,
                         .data = {0x03, 0x22, 0xD8, 0x66, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_DIAG_START = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x6B4,
                               .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_CONTACTOR_RESET_START = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x01, 0xDD, 0x35, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_CONTACTOR_RESET_PROGRESS = {.FD = false,
                                             .ext_ID = false,
                                             .DLC = 8,
                                             .ID = 0x6B4,
                                             .data = {0x04, 0x31, 0x03, 0xDD, 0x35, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_COLLISION_RESET_START = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x01, 0xDF, 0x60, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_COLLISION_RESET_PROGRESS = {.FD = false,
                                             .ext_ID = false,
                                             .DLC = 8,
                                             .ID = 0x6B4,
                                             .data = {0x04, 0x31, 0x03, 0xDF, 0x60, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_ISOLATION_RESET_START = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 8,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x01, 0xDF, 0x46, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_ISOLATION_RESET_PROGRESS = {.FD = false,
                                             .ext_ID = false,
                                             .DLC = 8,
                                             .ID = 0x6B4,
                                             .data = {0x04, 0x31, 0x03, 0xDF, 0x46, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_RESET_DONE = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x6B4,
                               .data = {0x02, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  uint8_t data_010_CRC[8] = {0xB4, 0x96, 0x78, 0x5A, 0x3C, 0x1E, 0xF0, 0xD2};
  uint8_t data_3A2_CRC[16] = {0x08, 0x17, 0x26, 0x35, 0x44, 0x53, 0x62, 0x71,
                              0x80, 0x9F, 0xAE, 0xBD, 0xCC, 0xDB, 0xEA, 0xF9};
  uint8_t data_345_content[16] = {0x01, 0xF2, 0xE3, 0xD4, 0xC5, 0xB6, 0xA7, 0x98,
                                  0x89, 0x7A, 0x6B, 0x5C, 0x4D, 0x3E, 0x2F, 0x10};
};
#endif
