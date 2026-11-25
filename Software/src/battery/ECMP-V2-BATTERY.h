#ifndef STELLANTIS_ECMP_V2_BATTERY_H
#define STELLANTIS_ECMP_V2_BATTERY_H
#include "UdsCanBattery.h"

// Stellantis eCMP v2 (54 kWh 102S: Peugeot e-208/e-2008, Fiat 600e, Avenger, Corsa/Mokka-e)
// & STLA Medium (73/82 kWh 96S: Peugeot e-3008 / e-5008, Opel Grandland Electric)
#define SIMULATE_FULL_CAR_ECMPV2

class EcmpV2Battery : public UdsCanBattery {
 public:
  EcmpV2Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan) : UdsCanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    dtc = &datalayer_battery->dtc;
  }

  EcmpV2Battery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis ECMPv2 102S & STLA 96S";

  bool supports_clear_isolation() { return true; }
  bool supports_insulation_resistance() { return true; }
  void clear_isolation() { UserRequestIsolationReset = true; }

  bool supports_reset_crash() { return true; }
  void reset_crash() { UserRequestCollisionReset = true; }

  bool supports_contactor_reset() { return true; }
  void reset_contactor() { UserRequestContactorReset = true; }

  bool supports_reset_BMS() { return true; }
  void reset_BMS() { UserRequestBmsReset = true; }

  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "stellantis_ecmp_dtc.json"; }

 protected:
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int DEFAULT_NUMBER_OF_CELLS = 102;
  static const int MAX_PACK_VOLTAGE_DV = 4335;  // 102 * 4.25V (auto-updates from 0x583 for 96S/102S/108S)
  static const int MIN_PACK_VOLTAGE_DV = 3060;  // 102 * 3.00V
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 3000;

  enum ContactorCmdState : uint8_t { CMD_IDLE = 0, CMD_WAKE_EDGE = 1, CMD_PRECHARGE_REQ = 2, CMD_CLOSED_HOLD = 3 };
  ContactorCmdState cmd_state = CMD_IDLE;
  unsigned long state_entered_ms = 0;
  unsigned long closed_confirmed_ms = 0;

  // Periodic TX timers
  unsigned long previousMillis10 = 0;
  unsigned long previousMillis20 = 0;
  unsigned long previousMillis30 = 0;
  unsigned long previousMillis40 = 0;
  unsigned long previousMillis50 = 0;
  unsigned long previousMillis100 = 0;
  unsigned long previousMillis250 = 0;
  unsigned long previousMillis500 = 0;
  unsigned long previousMillis1000 = 0;

  // 50ms Car-TX quiet window whenever 0x694 starts a multi-frame ISO-TP reply (e.g., Read DTC 59 02!)
  unsigned long isotp_quiet_until_ms = 0;

  // Verified eCMP v2 / STLA Real SOH signals (broadcast 0x5B5 lo12 * 0.03% + UDS 0xDF49/0xDF44)
  static const uint16_t PID_SOH_PACK = 0xDF49;
  static const uint16_t PID_SOH_MIN_CELL = 0xDF44;
  uint16_t soh_5b5_pptt = 0;      // 0..10000 (0.01% units)
  uint16_t pid_soh_pack = 0;      // 0..1000  (0.1% units from 0xDF49)
  uint16_t pid_soh_min_cell = 0;  // 0..1000  (0.1% units from 0xDF44)

  // Additional eCMP v2 UDS reset & JDD frames
  static constexpr CAN_frame ECMP_BREAKER_RESET_START = {.FD = false,
                                                         .ext_ID = false,
                                                         .DLC = 5,
                                                         .ID = 0x6B4,
                                                         .data = {0x04, 0x31, 0x01, 0xDD, 0x0B}};
  static constexpr CAN_frame ECMP_UV_RESET_START = {.FD = false,
                                                    .ext_ID = false,
                                                    .DLC = 5,
                                                    .ID = 0x6B4,
                                                    .data = {0x04, 0x31, 0x01, 0xDF, 0xFC}};
  static constexpr CAN_frame ECMP_UV_RESET_PROGRESS = {.FD = false,
                                                       .ext_ID = false,
                                                       .DLC = 5,
                                                       .ID = 0x6B4,
                                                       .data = {0x04, 0x31, 0x03, 0xDF, 0xFC}};
  static constexpr CAN_frame ECMP_CLEAR_DTCS = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 5,
                                                .ID = 0x6B4,
                                                .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF}};
  static constexpr CAN_frame ECMP_DIAG_STOP = {.FD = false,
                                               .ext_ID = false,
                                               .DLC = 3,
                                               .ID = 0x6B4,
                                               .data = {0x02, 0x10, 0x01}};
  static constexpr CAN_frame ECMP_ECU_RESET_SOFT = {.FD = false,
                                                    .ext_ID = false,
                                                    .DLC = 3,
                                                    .ID = 0x6B4,
                                                    .data = {0x02, 0x11, 0x03}};
  static constexpr CAN_frame TX_55F = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x55F, .data = {0x82}};

  // Rolling 4-bit counters
  uint8_t counter_10ms = 0;
  uint8_t counter_20ms = 0;
  uint8_t counter_30ms = 0;
  uint8_t counter_40ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t counter_500ms = 0;
  uint8_t counter_1000ms = 0;
  uint32_t ticks_552 = 0x01B113CC;

  // ============================================================================
  // eCMP v2 / STLA Car-TX CAN frames (verified against Fiat 600e & e-3008 logs)
  // ============================================================================
  // 10 ms frames
  CAN_frame TX_046 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x046, .data = {0x00, 0x00, 0x00, 0x08}};
  CAN_frame TX_0ED = {.FD = false,
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x0ED,
                      .data = {0x64, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x00}};
  CAN_frame TX_146 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x146,
                      .data = {0x00, 0x00, 0x3B, 0xE0, 0x03, 0xBE, 0xBC, 0x80}};
  CAN_frame TX_17B = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x17B,
                      .data = {0x00, 0x00, 0x40, 0x00, 0x18, 0x00, 0x00, 0x00}};
  CAN_frame TX_186 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x186,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_1B8 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x1B8,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x01}};
  CAN_frame TX_1D8 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x1D8,
                      .data = {0x00, 0x00, 0x27, 0x10, 0x9C, 0x40, 0x27, 0x10}};
  CAN_frame TX_1F8 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x1F8,
                      .data = {0x00, 0xA2, 0x94, 0x50, 0x7F, 0xFC, 0x01, 0x00}};
  CAN_frame TX_238 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x238,
                      .data = {0x00, 0x00, 0x00, 0x4E, 0x20, 0xC8, 0x7D, 0x20}};
  CAN_frame TX_2C6 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x2C6,
                      .data = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x54}};

  // 20 ms frames
  CAN_frame TX_12D = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x12D,
                      .data = {0x00, 0x00, 0xD0, 0x00, 0x00, 0x20, 0x00, 0x00}};
  CAN_frame TX_2E2 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x2E2, .data = {0x00, 0x00, 0x00, 0x00}};

  // 30 ms frames
  CAN_frame TX_421 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x421,
                      .data = {0x00, 0xFA, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00}};

  // 40 ms frames
  CAN_frame TX_342 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x342,
                      .data = {0x00, 0x00, 0x01, 0x28, 0x00, 0x00, 0x00, 0x80}};
  CAN_frame TX_428 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x428,
                      .data = {0x00, 0x01, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 50 ms frames
  CAN_frame TX_27A = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x27A,
                      .data = {0x4E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 100 ms frames
  CAN_frame TX_0A6 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x0A6, .data = {0x02, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_281 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x281, .data = {0x00, 0x51, 0x00, 0x00, 0x00}};
  CAN_frame TX_31D = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x31D,
                      .data = {0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_31E = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x31E,
                      .data = {0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_382 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x382,
                      .data = {0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_398 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x398,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}};
  CAN_frame TX_3E2 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x3E2, .data = {0x00, 0x00, 0x00, 0x00, 0x01}};
  CAN_frame TX_422 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x422,
                      .data = {0x00, 0x00, 0x10, 0x7B, 0xFA, 0x00, 0x00, 0x00}};
  CAN_frame TX_441 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x441,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x9C, 0x40, 0x00}};
  CAN_frame TX_448 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x448,
                      .data = {0x00, 0x00, 0x4E, 0x20, 0x00, 0x00, 0x00, 0x80}};
  CAN_frame TX_458 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x458,
                      .data = {0x00, 0x9C, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_482 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x482,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_4D1 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4D1,
                      .data = {0x00, 0x00, 0x00, 0x9C, 0x40, 0x28, 0xFA, 0x00}};
  CAN_frame TX_4F1 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x4F1,
                      .data = {0x00, 0xD4, 0x98, 0x48, 0x4D, 0x00, 0x00, 0x00}};
  CAN_frame TX_501 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x501,
                      .data = {0x00, 0x00, 0x10, 0x00, 0xFA, 0x00, 0x00, 0x41}};
  CAN_frame TX_526 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x526, .data = {0x20, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_541 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x541,
                      .data = {0x00, 0x00, 0x54, 0x00, 0x10, 0x9C, 0x40, 0x00}};
  CAN_frame TX_586 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 6,
                      .ID = 0x586,
                      .data = {0x00, 0x00, 0x03, 0x72, 0x00, 0x00}};

  // 500 ms frames
  CAN_frame TX_0AE = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x0AE, .data = {0x04, 0x77, 0x7A, 0x5E, 0xDF}};
  CAN_frame TX_531 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 6,
                      .ID = 0x531,
                      .data = {0x00, 0x4D, 0x00, 0x00, 0x00, 0x00}};

  // 1000 ms frames
  CAN_frame TX_402 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x402,
                      .data = {0x00, 0x00, 0x00, 0xFF, 0xE0, 0x00, 0x00, 0xA0}};
  CAN_frame TX_551 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x551, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_552 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x552,
                      .data = {0x01, 0xB1, 0x13, 0xCC, 0x00, 0x35, 0xD1, 0xFE}};
  CAN_frame TX_591 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x591,
                      .data = {0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_5A6 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x5A6, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame TX_5C6 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x5C6, .data = {0x3C, 0x3C, 0x00, 0x00}};
  CAN_frame TX_786 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x786,
                      .data = {0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // UDS diagnostic reset frames (0x6B4 -> 0x694)
  static constexpr CAN_frame ECMP_DIAG_START = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 3,
                                                .ID = 0x6B4,
                                                .data = {0x02, 0x10, 0x03}};
  static constexpr CAN_frame ECMP_CONTACTOR_RESET_START = {.FD = false,
                                                           .ext_ID = false,
                                                           .DLC = 5,
                                                           .ID = 0x6B4,
                                                           .data = {0x04, 0x31, 0x01, 0xDD, 0x35}};
  static constexpr CAN_frame ECMP_CONTACTOR_RESET_PROGRESS = {.FD = false,
                                                              .ext_ID = false,
                                                              .DLC = 5,
                                                              .ID = 0x6B4,
                                                              .data = {0x04, 0x31, 0x03, 0xDD, 0x35}};
  static constexpr CAN_frame ECMP_COLLISION_RESET_START = {.FD = false,
                                                           .ext_ID = false,
                                                           .DLC = 5,
                                                           .ID = 0x6B4,
                                                           .data = {0x04, 0x31, 0x01, 0xDF, 0x60}};
  static constexpr CAN_frame ECMP_COLLISION_RESET_PROGRESS = {.FD = false,
                                                              .ext_ID = false,
                                                              .DLC = 5,
                                                              .ID = 0x6B4,
                                                              .data = {0x04, 0x31, 0x03, 0xDF, 0x60}};
  static constexpr CAN_frame ECMP_ISOLATION_RESET_START = {.FD = false,
                                                           .ext_ID = false,
                                                           .DLC = 5,
                                                           .ID = 0x6B4,
                                                           .data = {0x04, 0x31, 0x01, 0xDF, 0x46}};
  static constexpr CAN_frame ECMP_ISOLATION_RESET_PROGRESS = {.FD = false,
                                                              .ext_ID = false,
                                                              .DLC = 5,
                                                              .ID = 0x6B4,
                                                              .data = {0x04, 0x31, 0x03, 0xDF, 0x46}};

  // UDS PIDs (0x6B4/0x694)
  static const uint8_t NOT_SAMPLED_YET = 255;
  static const uint8_t COMPLETED_STATE = 0;
  static const uint16_t PID_WELD_CHECK = 0xD814;
  static const uint16_t PID_CONT_REASON_OPEN = 0xD812;
  static const uint16_t PID_CONTACTOR_STATUS = 0xD813;
  static const uint16_t PID_INSULATION_NEG = 0xD87C;
  static const uint16_t PID_INSULATION_POS = 0xD87B;
  static const uint16_t PID_INSULATION_RES = 0xD47A;
  static const uint16_t PID_CRASH_COUNTER = 0xD42F;
  static const uint16_t PID_WIRE_CRASH = 0xD87F;
  static const uint16_t PID_12V = 0xD822;
  static const uint16_t PID_HVIL_STATE = 0xD869;
  static const uint16_t PID_BMS_STATE = 0xD45A;
  static const uint16_t PID_MOST_CRITICAL_FAULT = 0xD481;
  static const uint16_t PID_CONTACTOR_CLOSING_COUNTER = 0xD416;
  static const uint16_t PID_BATTERY_SERIAL = 0xF18C;
  static const uint16_t PID_DATE_OF_MANUFACTURE = 0xF18B;

  // Decoded eCMP v2 / STLA BMS telemetry
  uint16_t cellvoltages[108] = {0};
  int8_t celltemps[56] = {0};
  bool celltemp_valid[56] = {false};

  uint16_t HV_BATT_SOC = 1022;  // 0..1000 valid; 1022=boot/invalid
  uint16_t HV_BATT_SOH = 1000;
  uint16_t HV_BATT_REAL_VOLT_HD = 0;
  int16_t HV_BATT_REAL_CURR_HD = 0;
  uint16_t HV_BATT_COP_VOLTAGE = 0;
  int16_t HV_BATT_COP_CURRENT = 0;
  int16_t HV_BATT_REAL_POWER_HD = 0;

  uint16_t HV_BATT_SOE_HD = 0;
  uint16_t HV_BATT_SOE_MAX = 0;
  uint16_t HV_BATT_PEAK_DISCH_POWER_HD = 0;
  uint16_t HV_BATT_PEAK_CH_POWER_HD = 10000;
  uint16_t HV_BATT_NOM_CH_POWER_HD = 10000;
  uint16_t HV_BATT_STABLE_CH_POWER_HD = 10000;
  uint16_t HV_BATT_STABLE_DISCH_POWER_HD = 0;
  uint16_t HV_BATT_NOMINAL_DISCH_POWER_HD = 0;
  uint16_t MIN_ALLOW_DISCHRG_VOLTAGE = 0;

  uint16_t CELL_MAX_MV_5A3 = 0;
  uint16_t CELL_MIN_MV_5A3 = 0;
  uint8_t CELL_MAX_IDX_5A3 = 0;
  uint8_t CELL_MIN_IDX_5A3 = 0;

  uint16_t HV_BATT_FC_INSU_MINUS_RES = 0;
  uint16_t HV_BATT_FC_INSU_PLUS_RES = 0;
  uint16_t HV_BATT_FC_VHL_INSU_PLUS_RES = 0;
  uint16_t HV_BATT_ONLY_INSU_MINUS_RES = 0;

  uint8_t CONTACTORS_STATE = 0;
  uint8_t TBMU_FAULT_TYPE = 0;
  uint8_t CONTACTOR_OPENING_REASON = 0;
  uint8_t NUMBER_OF_TEMPERATURE_SENSORS_IN_BATTERY = 34;
  uint8_t NUMBER_OF_CELL_MEASUREMENTS_IN_BATTERY = DEFAULT_NUMBER_OF_CELLS;
  int8_t TEMPERATURE_MINIMUM_C = 20;

  bool HV_BATT_CRASH_MEMORIZED = false;
  bool ALERT_BATT = false;
  bool ALERT_LOW_SOC = false;
  bool ALERT_HIGH_SOC = false;
  bool ALERT_SOC_JUMP = false;
  bool ALERT_OVERCHARGE = false;
  bool ALERT_TEMP_DIFF = false;
  bool ALERT_HIGH_TEMP = false;
  bool ALERT_OVERVOLTAGE = false;
  bool ALERT_CELL_OVERVOLTAGE = false;
  bool ALERT_CELL_UNDERVOLTAGE = false;
  bool ALERT_CELL_POOR_CONSIST = false;

  uint8_t ContactorResetStatemachine = 0;
  uint8_t CollisionResetStatemachine = 0;
  uint8_t IsolationResetStatemachine = 0;
  uint8_t BmsResetStatemachine = 0;
  uint8_t timeSpentContactorReset = 0;
  uint8_t timeSpentCollisionReset = 0;
  uint8_t timeSpentIsolationReset = 0;
  uint8_t timeSpentBmsReset = 0;
  uint8_t countIsolationReset = 0;
  bool UserRequestContactorReset = false;
  bool UserRequestCollisionReset = false;
  bool UserRequestIsolationReset = false;
  bool UserRequestBmsReset = false;

  uint32_t pid_insulation_res_neg = NOT_SAMPLED_YET;
  uint32_t pid_insulation_res_pos = NOT_SAMPLED_YET;
  uint32_t pid_insulation_res = NOT_SAMPLED_YET;
  uint32_t pid_crash_counter = NOT_SAMPLED_YET;
  uint32_t pid_contactor_closing_counter = NOT_SAMPLED_YET;
  uint32_t pid_date_of_manufacture = NOT_SAMPLED_YET;
  uint16_t pid_most_critical_fault = NOT_SAMPLED_YET;
  uint16_t pid_12v = 12345;
  uint8_t pid_welding_detection = NOT_SAMPLED_YET;
  uint8_t pid_reason_open = NOT_SAMPLED_YET;
  uint8_t pid_contactor_status = NOT_SAMPLED_YET;
  uint8_t pid_wire_crash = NOT_SAMPLED_YET;
  uint8_t pid_hvil_state = NOT_SAMPLED_YET;
  uint8_t pid_bms_state = NOT_SAMPLED_YET;
  uint8_t pid_battery_serial[16] = {0};
};
#endif
