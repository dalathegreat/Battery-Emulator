#ifndef FORD_MACH_E_BATTERY_H
#define FORD_MACH_E_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"
#include "FORD-MACH-E-BATTERY-HTML.h"

class FordMachEBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Ford Mustang Mach-E battery";
  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

  bool supports_reset_DTC() { return true; }
  void reset_DTC() { UserRequestDTCreset = true; }

 private:
  FordMachEHtmlRenderer renderer;
  bool UserRequestDTCreset = false;
  //90S NMC
  static const int MAX_PACK_VOLTAGE_90S_DV = 3902;
  static const int MIN_PACK_VOLTAGE_90S_DV = 2490;
  static const int MAX_CAPACITY_90S_WH = 98800;

  //94S 98.8 kWh NMC LGES
  static const int MAX_PACK_VOLTAGE_94S_DV = 4072;
  static const int MIN_PACK_VOLTAGE_94S_DV = 2612;
  static const int MAX_CAPACITY_94S_WH = 98800;

  //75.7 kWh NMC LGES
  static const int MAX_PACK_VOLTAGE_96S_DV = 4140;
  static const int MIN_PACK_VOLTAGE_96S_DV = 2680;
  static const int MAX_CAPACITY_96S_WH = 75700;

  //78.2 kWh LFP CATL
  static const int MAX_PACK_VOLTAGE_108S_DV = 3870;
  static const int MIN_PACK_VOLTAGE_108S_DV = 3200;
  static const int MAX_CAPACITY_108S_WH = 78200;
  static const int MAX_CELL_VOLTAGE_LFP_MV = 3670;

  //Common
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2900;

  static const int MAX_CHARGE_POWER_WHEN_TOPBALANCING_W = 200;  // W, what power to allow for top balancing battery
  static const int FLOAT_START_MV = 20;  // mV, how many mV under overvoltage to start float charging

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

  static const uint8_t NOT_SAMPLED_YET = 255;
  // BECM Module PID Definitions
  static const uint16_t PID_HVB_TEMP = 0x4800;
  static const uint16_t PID_HVB_SOC = 0x4801;
  static const uint16_t PID_HVB_CONTACTOR_STATUS = 0x4802;
  static const uint16_t PID_HVB_CONTACTOR_POSITIVE_LEAK_VOLTAGE = 0x4803;
  static const uint16_t PID_HVB_CONTACTOR_NEGATIVE_LEAK_VOLTAGE = 0x4804;
  static const uint16_t PID_HVB_CONTACTOR_POSITIVE_VOLTAGE = 0x4805;
  static const uint16_t PID_HVB_CONTACTOR_NEGATIVE_VOLTAGE = 0x4806;
  static const uint16_t PID_HVB_CONTACTOR_POSITIVE_BUS_LEAK_RESISTANCE = 0x4811;
  static const uint16_t PID_HVB_CONTACTOR_NEGATIVE_BUS_LEAK_RESISTANCE = 0x4812;
  static const uint16_t PID_HVB_CONTACTOR_OVERALL_LEAK_RESISTANCE = 0x4813;
  static const uint16_t PID_HVB_CONTACTOR_OPEN_LEAK_RESISTANCE = 0x4814;
  static const uint16_t PID_HVB_ETE = 0x4848;
  static const uint16_t PID_HVB_CURRENT = 0x48F9;
  static const uint16_t PID_CHARGER_POWER_LIMIT = 0x48FB;
  static const uint16_t PID_HVB_SOH = 0x490C;
  static const uint16_t PID_HVB_CALENDAR_AGE_MONTHS = 0x4810;
  static const uint16_t PID_BATTERY_CAPACITY = 0x485C;
  static const uint16_t PID_MAINTENANCE_REBALANCE_STATUS = 0x4818;
  //Shared PIDs with other modules, but also seen in BECM
  static const uint16_t PID_HVB_VOLTAGE = 0x480D;                   // OBCC, SOBDM, SOBDMC, SOBDMB
  static const uint16_t PID_HVB_CHARGE_VOLTAGE_REQUESTED = 0x4844;  // OBCC, SOBDM
  static const uint16_t PID_HVB_SOC_D = 0x4845;                     // OBCC, SOBDM
  static const uint16_t PID_HVB_MAX_CHARGE_CURRENT = 0x48BC;        // OBCC, SOBDM
  static const uint16_t PID_HVB_CHARGE_CURRENT_REQUESTED = 0x4842;  // OBCC, SOBDM
  static const uint16_t PID_GEAR_COMMANDED = 0x1E12;                // GSM, SOBDM, SOBDMC
  static const uint16_t PID_KEY_STATE = 0x411F;               // GWM, WACM, ACM, HVAC, DSP, APIM, SOBDM, SOBDMC, SOBDMB
  static const uint16_t PID_CHARGE_PLUG = 0x4843;             // OBCC, SOBDM
  static const uint16_t PID_CHARGER_OUTPUT_VOLTAGE = 0x484A;  // OBCC, SOBDM
  static const uint16_t PID_CHARGER_STATUS = 0x484F;          // OBCC, SOBDM
  static const uint16_t PID_CHARGER_OUTPUT_CURRENT_MEASURED = 0x4850;  // OBCC, SOBDM
  static const uint16_t PID_EVSE_TYPE = 0x4851;                        // OBCC, SOBDM
  static const uint16_t PID_CHARGER_MAX_POWER = 0x48C4;                // OBCC, SOBDM
  static const uint16_t PID_CHARGING_STATUS = 0x484D;                  // SOBDM
  static const uint16_t PID_CHARGER_INPUT_POWER_AVAILABLE = 0x484E;    // SOBDM
  static const uint16_t PID_TIME = 0xDD00;                             // many modules
  static const uint16_t PID_LORES_ODOMETER = 0xDD01;                   // many modules
  static const uint16_t PID_ENGINE_RUNTIME = 0xF41F;                   // SOBDMB, SOBDMC, PCM
  static const uint16_t PID_TIME_SINCE_MODULE_POWER_UP_OR_RESET = 0xD002;
  // Unknown PIDs seen in CAN log — not yet decoded
  /*
static const uint16_t PID_UNKNOWN_1 = 0x0117;
static const uint16_t PID_UNKNOWN_2 = 0x0119;
static const uint16_t PID_UNKNOWN_3 = 0x011A;
static const uint16_t PID_UNKNOWN_4 = 0x0599;
static const uint16_t PID_UNKNOWN_5 = 0x05A1;
static const uint16_t PID_UNKNOWN_6 = 0x05CB;
static const uint16_t PID_UNKNOWN_7 = 0x4808;
static const uint16_t PID_UNKNOWN_8 = 0x4809;
static const uint16_t PID_UNKNOWN_9 = 0x480A;
static const uint16_t PID_UNKNOWN_12 = 0x481D;
static const uint16_t PID_UNKNOWN_13 = 0x483E;
static const uint16_t PID_UNKNOWN_14 = 0x483F;
static const uint16_t PID_UNKNOWN_15 = 0x4840;
static const uint16_t PID_UNKNOWN_16 = 0x4841;
static const uint16_t PID_UNKNOWN_17 = 0x4846;
static const uint16_t PID_UNKNOWN_18 = 0x485D;
static const uint16_t PID_UNKNOWN_19 = 0x4878;
static const uint16_t PID_UNKNOWN_20 = 0x4898;
static const uint16_t PID_UNKNOWN_21 = 0x489E;
static const uint16_t PID_UNKNOWN_22 = 0x48A1;
static const uint16_t PID_UNKNOWN_23 = 0x48AF;
static const uint16_t PID_UNKNOWN_24 = 0x48CF;
static const uint16_t PID_UNKNOWN_25 = 0x48E2;
static const uint16_t PID_UNKNOWN_26 = 0x48E4;
static const uint16_t PID_UNKNOWN_27 = 0x4901;
static const uint16_t PID_UNKNOWN_28 = 0x4903;
static const uint16_t PID_UNKNOWN_29 = 0x4904;
static const uint16_t PID_UNKNOWN_30 = 0x5B56;
static const uint16_t PID_UNKNOWN_30 = 0x48B9;  //Could be Max voltage battery module ID ?
static const uint16_t PID_UNKNOWN_32 = 0xDD80;
static const uint16_t PID_UNKNOWN_33 = 0xF404;
static const uint16_t PID_UNKNOWN_34 = 0xF405;
static const uint16_t PID_UNKNOWN_35 = 0xF40C;
static const uint16_t PID_UNKNOWN_36 = 0xF40D;
static const uint16_t PID_UNKNOWN_37 = 0xF449;
*/

  uint16_t currentpoll = PID_HVB_TEMP;
  uint8_t poll_index = 0;
  const uint16_t poll_commands[36] = {PID_HVB_TEMP,
                                      PID_HVB_SOC,
                                      PID_HVB_CONTACTOR_STATUS,
                                      PID_HVB_CONTACTOR_POSITIVE_LEAK_VOLTAGE,
                                      PID_HVB_CONTACTOR_NEGATIVE_LEAK_VOLTAGE,
                                      PID_HVB_CONTACTOR_POSITIVE_VOLTAGE,
                                      PID_HVB_CONTACTOR_NEGATIVE_VOLTAGE,
                                      PID_HVB_CONTACTOR_POSITIVE_BUS_LEAK_RESISTANCE,
                                      PID_HVB_CONTACTOR_NEGATIVE_BUS_LEAK_RESISTANCE,
                                      PID_HVB_CONTACTOR_OVERALL_LEAK_RESISTANCE,
                                      PID_HVB_CONTACTOR_OPEN_LEAK_RESISTANCE,
                                      PID_HVB_ETE,
                                      PID_HVB_CURRENT,
                                      PID_CHARGER_POWER_LIMIT,
                                      PID_HVB_SOH,
                                      PID_HVB_VOLTAGE,
                                      PID_HVB_CHARGE_VOLTAGE_REQUESTED,
                                      PID_HVB_SOC_D,
                                      PID_HVB_MAX_CHARGE_CURRENT,
                                      PID_HVB_CHARGE_CURRENT_REQUESTED,
                                      PID_GEAR_COMMANDED,
                                      PID_KEY_STATE,
                                      PID_CHARGE_PLUG,
                                      PID_CHARGER_OUTPUT_VOLTAGE,
                                      PID_CHARGER_STATUS,
                                      PID_CHARGER_OUTPUT_CURRENT_MEASURED,
                                      PID_EVSE_TYPE,
                                      PID_CHARGER_MAX_POWER,
                                      PID_CHARGING_STATUS,
                                      PID_CHARGER_INPUT_POWER_AVAILABLE,
                                      PID_TIME,
                                      PID_LORES_ODOMETER,
                                      PID_ENGINE_RUNTIME,
                                      PID_HVB_CALENDAR_AGE_MONTHS,
                                      PID_BATTERY_CAPACITY,
                                      PID_MAINTENANCE_REBALANCE_STATUS};

  int16_t pid_hvb_temp = NOT_SAMPLED_YET;
  uint32_t pid_hvb_soc = NOT_SAMPLED_YET;
  uint32_t pid_hvb_contactor_status = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_positive_leak_voltage = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_negative_leak_voltage = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_positive_voltage = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_negative_voltage = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_positive_bus_leak_resistance = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_negative_bus_leak_resistance = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_overall_leak_resistance = NOT_SAMPLED_YET;
  uint16_t pid_hvb_contactor_open_leak_resistance = NOT_SAMPLED_YET;
  uint16_t pid_hvb_ete = NOT_SAMPLED_YET;
  uint16_t pid_hvb_current = NOT_SAMPLED_YET;
  uint16_t pid_charger_power_limit = NOT_SAMPLED_YET;
  uint8_t pid_hvb_soh = NOT_SAMPLED_YET;
  uint16_t pid_hvb_voltage = NOT_SAMPLED_YET;
  uint16_t pid_hvb_charge_voltage_requested = NOT_SAMPLED_YET;
  uint16_t pid_hvb_soc_d = NOT_SAMPLED_YET;
  uint16_t pid_hvb_max_charge_current = NOT_SAMPLED_YET;
  uint16_t pid_hvb_charge_current_requested = NOT_SAMPLED_YET;
  uint8_t pid_gear_commanded = NOT_SAMPLED_YET;
  uint8_t pid_key_state = NOT_SAMPLED_YET;
  uint8_t pid_charge_plug = NOT_SAMPLED_YET;
  uint8_t pid_charger_output_voltage = NOT_SAMPLED_YET;
  uint8_t pid_charger_status = NOT_SAMPLED_YET;
  uint8_t pid_charger_output_current_measured = NOT_SAMPLED_YET;
  uint8_t pid_evse_type = NOT_SAMPLED_YET;
  uint8_t pid_charger_max_power = NOT_SAMPLED_YET;
  uint8_t pid_charging_status = NOT_SAMPLED_YET;
  uint8_t pid_charger_input_power_available = NOT_SAMPLED_YET;
  uint8_t pid_time = NOT_SAMPLED_YET;
  uint8_t pid_lores_odometer = NOT_SAMPLED_YET;
  uint8_t pid_engine_runtime = NOT_SAMPLED_YET;
  uint16_t pid_hvb_calendar_age_months = NOT_SAMPLED_YET;
  uint16_t pid_battery_capacity_ah = NOT_SAMPLED_YET;
  uint8_t pid_maintenance_rebalance_status = NOT_SAMPLED_YET;

  uint16_t poll_state = PID_HVB_TEMP;
  uint16_t incoming_poll = 0;

  CAN_frame FORD_PID_REQUEST_7DF = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x7DF,
                                    .data = {0x02, 0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_PID_REQUEST_7E4 = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x7E4,
                                    .data = {0x03, 0x22, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FORD_DTC_RESET = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7E4,
                              .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00}};

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
