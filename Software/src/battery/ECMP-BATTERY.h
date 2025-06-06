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
#define NOT_SAMPLED_YET 255
#define COMPLETED_STATE 0
  bool battery_RelayOpenRequest = false;
  bool battery_InterlockOpen = false;
  uint8_t ContactorResetStatemachine = 0;
  uint8_t CollisionResetStatemachine = 0;
  uint8_t IsolationResetStatemachine = 0;
  uint8_t DisableIsoMonitoringStatemachine = 0;
  uint8_t timeSpentDisableIsoMonitoring = 0;
  uint8_t timeSpentContactorReset = 0;
  uint8_t timeSpentCollisionReset = 0;
  uint8_t timeSpentIsolationReset = 0;
  uint8_t countIsolationReset = 0;
  uint8_t counter_10ms = 0;
  uint8_t counter_20ms = 0;
  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t counter_010 = 0;
  uint32_t ticks_552 = 0x0BC8CFC6;
  uint8_t battery_MainConnectorState = 0;
  int16_t battery_current = 0;
  uint16_t battery_voltage = 370;
  uint16_t battery_soc = 0;
  uint16_t cellvoltages[108];
  uint16_t battery_AllowedMaxChargeCurrent = 0;
  uint16_t battery_AllowedMaxDischargeCurrent = 0;
  uint16_t battery_insulationResistanceKOhm = 0;
  uint8_t battery_insulation_failure_diag = 0;
  int16_t battery_highestTemperature = 0;
  int16_t battery_lowestTemperature = 0;
  uint8_t pid_welding_detection = NOT_SAMPLED_YET;
  uint8_t pid_reason_open = NOT_SAMPLED_YET;
  uint8_t pid_contactor_status = NOT_SAMPLED_YET;
  uint8_t pid_negative_contactor_control = NOT_SAMPLED_YET;
  uint8_t pid_negative_contactor_status = NOT_SAMPLED_YET;
  uint8_t pid_positive_contactor_control = NOT_SAMPLED_YET;
  uint8_t pid_positive_contactor_status = NOT_SAMPLED_YET;
  uint8_t pid_contactor_negative = NOT_SAMPLED_YET;
  uint8_t pid_contactor_positive = NOT_SAMPLED_YET;
  uint8_t pid_precharge_relay_control = NOT_SAMPLED_YET;
  uint8_t pid_precharge_relay_status = NOT_SAMPLED_YET;
  uint8_t pid_recharge_status = NOT_SAMPLED_YET;
  uint8_t pid_delta_temperature = NOT_SAMPLED_YET;
  uint8_t pid_coldest_module = NOT_SAMPLED_YET;
  uint8_t pid_lowest_temperature = NOT_SAMPLED_YET;
  uint8_t pid_average_temperature = NOT_SAMPLED_YET;
  uint8_t pid_highest_temperature = NOT_SAMPLED_YET;
  uint8_t pid_hottest_module = NOT_SAMPLED_YET;
  uint16_t pid_avg_cell_voltage = NOT_SAMPLED_YET;
  int32_t pid_current = NOT_SAMPLED_YET;
  uint32_t pid_insulation_res_neg = NOT_SAMPLED_YET;
  uint32_t pid_insulation_res_pos = NOT_SAMPLED_YET;
  uint32_t pid_22 = NOT_SAMPLED_YET;
  uint32_t pid_max_discharge_10s = NOT_SAMPLED_YET;
  uint32_t pid_max_discharge_30s = NOT_SAMPLED_YET;
  uint32_t pid_max_charge_10s = NOT_SAMPLED_YET;
  uint32_t pid_max_charge_30s = NOT_SAMPLED_YET;
  uint32_t pid_energy_capacity = NOT_SAMPLED_YET;
  uint8_t pid_highest_cell_voltage_num = NOT_SAMPLED_YET;
  uint8_t pid_lowest_cell_voltage_num = NOT_SAMPLED_YET;
  uint16_t pid_sum_of_cells = NOT_SAMPLED_YET;
  uint16_t pid_cell_min_capacity = NOT_SAMPLED_YET;
  uint8_t pid_cell_voltage_measurement_status = NOT_SAMPLED_YET;
  uint32_t pid_insulation_res = NOT_SAMPLED_YET;
  uint16_t pid_pack_voltage = NOT_SAMPLED_YET;
  uint16_t pid_high_cell_voltage = NOT_SAMPLED_YET;
  uint16_t pid_low_cell_voltage = NOT_SAMPLED_YET;
  uint8_t pid_battery_energy = NOT_SAMPLED_YET;
  uint32_t pid_40 = NOT_SAMPLED_YET;
  uint8_t pid_41 = NOT_SAMPLED_YET;
  uint8_t pid_42 = NOT_SAMPLED_YET;

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis250 = 0;   // will store last time a 250ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent

#define PID_WELD_CHECK 0xD814
#define PID_CONT_REASON_OPEN 0xD812
#define PID_CONTACTOR_STATUS 0xD813
#define PID_NEG_CONT_CONTROL 0xD44F
#define PID_NEG_CONT_STATUS 0xD453
#define PID_POS_CONT_CONTROL 0xD44E
#define PID_POS_CONT_STATUS 0xD452
#define PID_CONTACTOR_NEGATIVE 0xD44C
#define PID_CONTACTOR_POSITIVE 0xD44D
#define PID_PRECHARGE_RELAY_CONTROL 0xD44B
#define PID_PRECHARGE_RELAY_STATUS 0xD451
#define PID_RECHARGE_STATUS 0xD864
#define PID_DELTA_TEMPERATURE 0xD878
#define PID_COLDEST_MODULE 0xD446
#define PID_LOWEST_TEMPERATURE 0xD87D
#define PID_AVERAGE_TEMPERATURE 0xD877
#define PID_HIGHEST_TEMPERATURE 0xD817
#define PID_HOTTEST_MODULE 0xD445
#define PID_AVG_CELL_VOLTAGE 0xD43D
#define PID_CURRENT 0xD816
#define PID_INSULATION_NEG 0xD87C
#define PID_INSULATION_POS 0xD87B
#define PID_22 0xD876
#define PID_MAX_DISCHARGE_10S 0xD873
#define PID_MAX_DISCHARGE_30S 0xD874
#define PID_MAX_CHARGE_10S 0xD871
#define PID_MAX_CHARGE_30S 0xD872
#define PID_ENERGY_CAPACITY 0xD860
#define PID_HIGH_CELL_NUM 0xD43B
#define PID_LOW_CELL_NUM 0xD43C
#define PID_SUM_OF_CELLS 0xD438
#define PID_CELL_MIN_CAPACITY 0xD413
#define PID_32 0xD48A
#define PID_INSULATION_RES 0xD47A
#define PID_PACK_VOLTAGE 0xD815
#define PID_HIGH_CELL_VOLTAGE 0xD870
#define PID_36 0xD440  //Multi-frame
#define PID_LOW_CELL_VOLTAGE 0xD86F
#define PID_BATTERY_ENERGY 0xD865
#define PID_39 0xD470  //Multi-frame (State of Cell1 Of Module 11 of the Traction Battery?)
#define PID_40 0xD42F  //?Collision information Counter recieved by CAN (Order unsure on these)
#define PID_41 0xD87F  //?Collision Counter recieved by Wire
#define PID_42 0xD48D  //?Detection of a Vehicle Impact
#define PID_43 0xD465  //Unknown Multi Frame
#define PID_44 0xD492  //Unknown

  uint16_t poll_state = PID_WELD_CHECK;
  uint16_t incoming_poll = 0;

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
                        .data = {0x7D, 0x00, 0x4E, 0x20, 0x00, 0x00, 0x60, 0x0D}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_110 = {.FD = false,
                        .ext_ID = false,  //EVSE
                        .DLC = 8,
                        .ID = 0x110,
                        .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x87, 0x05}};  // NOTE. Changes on BMS state
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
  CAN_frame ECMP_17B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x17B,
                        .data = {0x00, 0x00, 0x00, 0x7E, 0x78, 0x00, 0x00, 0x0F}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_230 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x230,  //TODO; removed during testing
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_27A = {.FD = false,  //VCU message
                        .ext_ID = false,
                        .DLC = 8,     //Contains SEV main state, position of the BSI shunt park, ACC status
                        .ID = 0x27A,  // electricnetwork state, powetrain status, Wakeuips, diagmux, APC activation
                        .data = {0x4F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_31E = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x31E,
                        .data = {0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_31D = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x31D,
                        .data = {0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42}};
  CAN_frame ECMP_3D0 = {
      .FD = false,      //Not in logs, but makes speed go to 0km/h
      .ext_ID = false,  //TODO, removed this 3D0 during testing
      .DLC = 8,
      .ID = 0x3D0,
      .data = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};  // Allows for DTCs to clear ? Removed now
  CAN_frame ECMP_345 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x345,
                        .data = {0x45, 0x57, 0x00, 0x04, 0x00, 0x00, 0x06, 0x31}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_351 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x351,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0E}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_372 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x372,
                        .data = {0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_37F = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x37F,
                        .data = {0x45, 0x49, 0x51, 0x45, 0x45, 0x00, 0x45, 0x45}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_382 = {
      .FD = false,  //BSI_Info (VCU) PSA specific
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x382,  //Frame1 has rollerbenchmode request, frame2 has generic powertrain cycle sync status
      .data = {0x02, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_383 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x383,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_3A2 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3A2,
                        .data = {0x03, 0xE8, 0x00, 0x00, 0x81, 0x00, 0x08, 0x02}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_3A3 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x3A3,
                        .data = {0x4A, 0x4A, 0x40, 0x00, 0x00, 0x08, 0x00, 0x0F}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_439 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x439,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ECMP_486 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x486,
                        .data = {0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_552 = {.FD = false,  //VCU 1s periodic (Perfectly handled in Battery-Emulator)
                        .ext_ID = false,
                        .DLC = 8,     //552 seems to be tracking time in byte 0-3
                        .ID = 0x552,  // distance in km in byte 4-6, temporal reset counter in byte 7
                        .data = {0x00, 0x02, 0x95, 0x6D, 0x00, 0xD7, 0xB5, 0xFE}};
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
  CAN_frame ECMP_794 = {.FD = false,  //Unsure who sends this. Could it be BMU?
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x794,
                        .data = {0xB8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};  // NOTE. Changes on BMS state
  CAN_frame ECMP_POLL = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x6B4, .data = {0x03, 0x22, 0xD8, 0x66}};
  CAN_frame ECMP_ACK = {.FD = false,  //Ack frame
                        .ext_ID = false,
                        .DLC = 3,
                        .ID = 0x6B4,
                        .data = {0x30, 0x00, 0x00}};
  CAN_frame ECMP_DIAG_START = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x6B4, .data = {0x02, 0x10, 0x03}};
  //Start diagnostic session (extended diagnostic session, mode 0x10 with sub-mode 0x03)
  CAN_frame ECMP_CONTACTOR_RESET_START = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 5,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x01, 0xDD, 0x35}};
  CAN_frame ECMP_CONTACTOR_RESET_PROGRESS = {.FD = false,
                                             .ext_ID = false,
                                             .DLC = 5,
                                             .ID = 0x6B4,
                                             .data = {0x04, 0x31, 0x03, 0xDD, 0x35}};
  CAN_frame ECMP_COLLISION_RESET_START = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 5,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x01, 0xDF, 0x60}};
  CAN_frame ECMP_COLLISION_RESET_PROGRESS = {.FD = false,
                                             .ext_ID = false,
                                             .DLC = 5,
                                             .ID = 0x6B4,
                                             .data = {0x04, 0x31, 0x03, 0xDF, 0x60}};
  CAN_frame ECMP_ISOLATION_RESET_START = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 5,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x01, 0xDF, 0x46}};
  CAN_frame ECMP_ISOLATION_RESET_PROGRESS = {.FD = false,
                                             .ext_ID = false,
                                             .DLC = 8,
                                             .ID = 0x6B4,
                                             .data = {0x04, 0x31, 0x03, 0xDF, 0x46}};
  CAN_frame ECMP_RESET_DONE = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x6B4, .data = {0x02, 0x3E, 0x00}};
  CAN_frame ECMP_FACTORY_MODE_ACTIVATION = {.FD = false,
                                            .ext_ID = false,
                                            .DLC = 5,
                                            .ID = 0x6B4,
                                            .data = {0x04, 0x2E, 0xD9, 0x00, 0x01}};
  CAN_frame ECMP_FACTORY_MODE_ACTIVATION_NEW = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 4,
                                                .ID = 0x6B4,
                                                .data = {0x04, 0x2E, 0x19, 0x01}};
  CAN_frame ECMP_DISABLE_ISOLATION_REQ = {.FD = false,
                                          .ext_ID = false,
                                          .DLC = 5,
                                          .ID = 0x6B4,
                                          .data = {0x04, 0x31, 0x02, 0xDF, 0xE1}};
  uint8_t data_010_CRC[8] = {0xB4, 0x96, 0x78, 0x5A, 0x3C, 0x1E, 0xF0, 0xD2};
  uint8_t data_3A2_CRC[16] = {0x0C, 0x1B, 0x2A, 0x39, 0x48, 0x57,
                              0x66, 0x75, 0x84, 0x93, 0xA2, 0xB1};                 // NOTE. Changes on BMS state
  uint8_t data_345_content[16] = {0x04, 0xF5, 0xE6, 0xD7, 0xC8, 0xB9, 0xAA, 0x9B,  // NOTE. Changes on BMS state
                                  0x8C, 0x7D, 0x6E, 0x5F, 0x40, 0x31, 0x22, 0x13};
};
#endif
