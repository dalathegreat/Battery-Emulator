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

  bool battery_RelayOpenRequest = false;
  bool battery_InterlockOpen = false;
  uint8_t ContactorResetStatemachine = 0;
  uint8_t CollisionResetStatemachine = 0;
  uint8_t counter_10ms = 0;
  uint8_t counter_20ms = 0;
  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
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
  CAN_frame ECMP_0A6 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x0A6, .data = {0x02, 0x01}};
  CAN_frame ECMP_0F0 = {.FD = false,  //VCU (Common)
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0F0,
                        .data = {0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}};
  CAN_frame ECMP_0F2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0F2,
                        .data = {0x7D, 0x00, 0x4E, 0x20, 0x00, 0x00, 0x50, 0x01}};
  CAN_frame ECMP_111 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x111,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
  CAN_frame ECMP_0C5 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x0C5,
                        .data = {0x08, 0x00, 0x00, 0x0A, 0x02, 0x04, 0xC0, 0x00}};
  CAN_frame ECMP_17B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x17B,
                        .data = {0x00, 0x00, 0x00, 0x0C, 0x98, 0x00, 0x00, 0x0F}};
  CAN_frame ECMP_230 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x230,
                        .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_27A = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x27A,
                        .data = {0x0C, 0x0E, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_31E = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x31E,
                        .data = {0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};
  CAN_frame ECMP_37F = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x37F,
                        .data = {0x47, 0x49, 0x4E, 0x47, 0x47, 0x00, 0x46, 0x47}};
  CAN_frame ECMP_382 = {.FD = false,  //BSI_Info (VCU) PSA specific
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x382,
                        .data = {0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_383 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x383,
                        .data = {0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_3A2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3A2,
                        .data = {0x7C, 0xC8, 0x3F, 0xD3, 0x9A, 0x33, 0x45, 0x05}};
  CAN_frame ECMP_3A3 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3A3,
                        .data = {0x49, 0x49, 0x41, 0x00, 0xDD, 0x72, 0x00, 0x0D}};
  CAN_frame ECMP_439 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x439,
                        .data = {0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_POLL = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x6B4,
                         .data = {0x03, 0x22, 0xD8, 0x66, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_CONTACTOR_RESET_0 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_CONTACTOR_RESET_1 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x04, 0x31, 0x01, 0xDD, 0x35, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_CONTACTOR_RESET_2 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x04, 0x31, 0x03, 0xDD, 0x35, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_CONTACTOR_RESET_3 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_COLLISION_RESET_0 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_COLLISION_RESET_1 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x04, 0x31, 0x01, 0xDF, 0x60, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_COLLISION_RESET_2 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x04, 0x31, 0x03, 0xDF, 0x60, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_COLLISION_RESET_3 = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x6B4,
                                      .data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  uint8_t data_0F0_20[16] = {0xFF, 0x0E, 0x1D, 0x2C, 0x3B, 0x4A, 0x59, 0x68,
                             0x77, 0x86, 0x95, 0xA4, 0xB3, 0xC2, 0xD1, 0xE0};
  uint8_t data_0F0_00[16] = {0xF1, 0x00, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,
                             0x79, 0x88, 0x97, 0xA6, 0xB5, 0xC4, 0xD3, 0xE2};
  uint8_t data_0F2_CRC[16] = {0x01, 0x10, 0x2F, 0x3E, 0x4D, 0x5C, 0x6B, 0x7A,
                              0x89, 0x98, 0xA7, 0xB6, 0xC5, 0xD4, 0xE3, 0xF2};
  uint8_t data_17B_CRC[16] = {0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A, 0x69, 0x78,
                              0x87, 0x96, 0xA5, 0xB4, 0xC3, 0xD2, 0xE1, 0xF0};
  uint8_t data_010_CRC[8] = {0xB4, 0x96, 0x78, 0x5A, 0x3C, 0x1E, 0xF0, 0xD2};
  uint8_t data_31E_CRC[16] = {0x01, 0x10, 0x2F, 0x3E, 0x4D, 0x5C, 0x6B, 0x7A,
                              0x89, 0x98, 0xA7, 0xB6, 0xC5, 0xD4, 0xE3, 0xF2};
  uint8_t data_3A2_CRC[16] = {0x09, 0x18, 0x27, 0x36, 0x45, 0x54, 0x63, 0x72,
                              0x81, 0x90, 0xAF, 0xBE, 0xCD, 0xDC, 0xEB, 0xFA};
  uint8_t data_3A3_CRC[16] = {0x0D, 0x1C, 0x2B, 0x3A, 0x49, 0x58, 0x67, 0x76,
                              0x85, 0x94, 0xA3, 0xB2, 0xC1, 0xD0, 0xEF, 0xFE};
};
#endif
