#ifndef BOLT_AMPERA_BATTERY_H
#define BOLT_AMPERA_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "BOLT-AMPERA-HTML.h"
#include "CanBattery.h"

#ifdef BOLT_AMPERA_BATTERY
#define SELECTED_BATTERY_CLASS BoltAmperaBattery
#endif

class BoltAmperaBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr char* Name = "Chevrolet Bolt EV/Opel Ampera-e";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  BoltAmperaHtmlRenderer renderer;
  static const int MAX_PACK_VOLTAGE_DV = 4150;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2500;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int POLL_7E4_CAPACITY_EST_GEN1 = 0x41A3;
  static const int POLL_7E4_CAPACITY_EST_GEN2 = 0x45F9;
  static const int POLL_7E4_SOC_DISPLAY = 0x8334;
  static const int POLL_7E4_SOC_RAW_HIGHPREC = 0x43AF;
  static const int POLL_7E4_MAX_TEMPERATURE = 0x4349;
  static const int POLL_7E4_MIN_TEMPERATURE = 0x434A;
  static const int POLL_7E4_MIN_CELL_V = 0x4329;
  static const int POLL_7E4_MAX_CELL_V = 0x432B;
  static const int POLL_7E4_INTERNAL_RES = 0x40E9;
  static const int POLL_7E4_LOWEST_CELL_NUMBER = 0x433B;
  static const int POLL_7E4_HIGHEST_CELL_NUMBER = 0x433C;
  static const int POLL_7E4_VOLTAGE = 0x432D;
  static const int POLL_7E4_VEHICLE_ISOLATION = 0x41EC;
  static const int POLL_7E4_ISOLATION_TEST_KOHM = 0x43A6;
  static const int POLL_7E4_HV_LOCKED_OUT = 0x44F8;
  static const int POLL_7E4_CRASH_EVENT = 0x4522;
  static const int POLL_7E4_HVIL = 0x4310;
  static const int POLL_7E4_HVIL_STATUS = 0x4311;
  static const int POLL_7E4_CURRENT = 0x4356;
  static const int POLL_7E7_CURRENT = 0x40D4;
  static const int POLL_7E7_5V_REF = 0x40D3;
  static const int POLL_7E7_MODULE_TEMP_1 = 0x40D7;
  static const int POLL_7E7_MODULE_TEMP_2 = 0x40D9;
  static const int POLL_7E7_MODULE_TEMP_3 = 0x40DB;
  static const int POLL_7E7_MODULE_TEMP_4 = 0x40DD;
  static const int POLL_7E7_MODULE_TEMP_5 = 0x40DF;
  static const int POLL_7E7_MODULE_TEMP_6 = 0x40E1;
  static const int POLL_7E7_CELL_AVG_VOLTAGE = 0xC218;
  static const int POLL_7E7_CELL_AVG_VOLTAGE_2 = 0x44B9;
  static const int POLL_7E7_TERMINAL_VOLTAGE = 0x82A3;
  static const int POLL_7E7_IGNITION_POWER_MODE = 0x8002;
  static const int POLL_7E7_GMLAN_HIGH_SPEED_STATUS = 0x8043;
  static const int POLL_7E7_HV_ISOLATION_RESISTANCE = 0x43A6;
  static const int POLL_7E7_HV_BUS_VOLTAGE = 0x438B;
  static const int POLL_7E7_HYBRID_CELL_BALANCING_ID_1 = 0x4323;
  static const int POLL_7E7_HYBRID_CELL_BALANCING_ID_2 = 0x4324;
  static const int POLL_7E7_HYBRID_CELL_BALANCING_ID_3 = 0x4325;
  static const int POLL_7E7_HYBRID_CELL_BALANCING_ID_4 = 0x4326;
  static const int POLL_7E7_HYBRID_CELL_BALANCING_ID_5 = 0x4327;
  static const int POLL_7E7_HYBRID_CELL_BALANCING_ID_6 = 0x4340;
  static const int POLL_7E7_HYBRID_BATTERY_CELL_BALANCE_STATUS = 0x431C;
  static const int POLL_7E7_5V_REF_VOLTAGE_1 = 0x428F;
  static const int POLL_7E7_5V_REF_VOLTAGE_2 = 0x4290;
  static const int POLL_7E7_CELL_01 = 0x4181;
  static const int POLL_7E7_CELL_02 = 0x4182;
  static const int POLL_7E7_CELL_03 = 0x4183;
  static const int POLL_7E7_CELL_04 = 0x4184;
  static const int POLL_7E7_CELL_05 = 0x4185;
  static const int POLL_7E7_CELL_06 = 0x4186;
  static const int POLL_7E7_CELL_07 = 0x4187;
  static const int POLL_7E7_CELL_08 = 0x4188;
  static const int POLL_7E7_CELL_09 = 0x4189;
  static const int POLL_7E7_CELL_10 = 0x418A;
  static const int POLL_7E7_CELL_11 = 0x418B;
  static const int POLL_7E7_CELL_12 = 0x418C;
  static const int POLL_7E7_CELL_13 = 0x418D;
  static const int POLL_7E7_CELL_14 = 0x418E;
  static const int POLL_7E7_CELL_15 = 0x418F;
  static const int POLL_7E7_CELL_16 = 0x4190;
  static const int POLL_7E7_CELL_17 = 0x4191;
  static const int POLL_7E7_CELL_18 = 0x4192;
  static const int POLL_7E7_CELL_19 = 0x4193;
  static const int POLL_7E7_CELL_20 = 0x4194;
  static const int POLL_7E7_CELL_21 = 0x4195;
  static const int POLL_7E7_CELL_22 = 0x4196;
  static const int POLL_7E7_CELL_23 = 0x4197;
  static const int POLL_7E7_CELL_24 = 0x4198;
  static const int POLL_7E7_CELL_25 = 0x4199;
  static const int POLL_7E7_CELL_26 = 0x419A;
  static const int POLL_7E7_CELL_27 = 0x419B;
  static const int POLL_7E7_CELL_28 = 0x419C;
  static const int POLL_7E7_CELL_29 = 0x419D;
  static const int POLL_7E7_CELL_30 = 0x419E;
  static const int POLL_7E7_CELL_31 = 0x419F;
  static const int POLL_7E7_CELL_32 = 0x4200;
  static const int POLL_7E7_CELL_33 = 0x4201;
  static const int POLL_7E7_CELL_34 = 0x4202;
  static const int POLL_7E7_CELL_35 = 0x4203;
  static const int POLL_7E7_CELL_36 = 0x4204;
  static const int POLL_7E7_CELL_37 = 0x4205;
  static const int POLL_7E7_CELL_38 = 0x4206;
  static const int POLL_7E7_CELL_39 = 0x4207;
  static const int POLL_7E7_CELL_40 = 0x4208;
  static const int POLL_7E7_CELL_41 = 0x4209;
  static const int POLL_7E7_CELL_42 = 0x420A;
  static const int POLL_7E7_CELL_43 = 0x420B;
  static const int POLL_7E7_CELL_44 = 0x420C;
  static const int POLL_7E7_CELL_45 = 0x420D;
  static const int POLL_7E7_CELL_46 = 0x420E;
  static const int POLL_7E7_CELL_47 = 0x420F;
  static const int POLL_7E7_CELL_48 = 0x4210;
  static const int POLL_7E7_CELL_49 = 0x4211;
  static const int POLL_7E7_CELL_50 = 0x4212;
  static const int POLL_7E7_CELL_51 = 0x4213;
  static const int POLL_7E7_CELL_52 = 0x4214;
  static const int POLL_7E7_CELL_53 = 0x4215;
  static const int POLL_7E7_CELL_54 = 0x4216;
  static const int POLL_7E7_CELL_55 = 0x4217;
  static const int POLL_7E7_CELL_56 = 0x4218;
  static const int POLL_7E7_CELL_57 = 0x4219;
  static const int POLL_7E7_CELL_58 = 0x421A;
  static const int POLL_7E7_CELL_59 = 0x421B;
  static const int POLL_7E7_CELL_60 = 0x421C;
  static const int POLL_7E7_CELL_61 = 0x421D;
  static const int POLL_7E7_CELL_62 = 0x421E;
  static const int POLL_7E7_CELL_63 = 0x421F;
  static const int POLL_7E7_CELL_64 = 0x4220;
  static const int POLL_7E7_CELL_65 = 0x4221;
  static const int POLL_7E7_CELL_66 = 0x4222;
  static const int POLL_7E7_CELL_67 = 0x4223;
  static const int POLL_7E7_CELL_68 = 0x4224;
  static const int POLL_7E7_CELL_69 = 0x4225;
  static const int POLL_7E7_CELL_70 = 0x4226;
  static const int POLL_7E7_CELL_71 = 0x4227;
  static const int POLL_7E7_CELL_72 = 0x4228;
  static const int POLL_7E7_CELL_73 = 0x4229;
  static const int POLL_7E7_CELL_74 = 0x422A;
  static const int POLL_7E7_CELL_75 = 0x422B;
  static const int POLL_7E7_CELL_76 = 0x422C;
  static const int POLL_7E7_CELL_77 = 0x422D;
  static const int POLL_7E7_CELL_78 = 0x422E;
  static const int POLL_7E7_CELL_79 = 0x422F;
  static const int POLL_7E7_CELL_80 = 0x4230;
  static const int POLL_7E7_CELL_81 = 0x4231;
  static const int POLL_7E7_CELL_82 = 0x4232;
  static const int POLL_7E7_CELL_83 = 0x4233;
  static const int POLL_7E7_CELL_84 = 0x4234;
  static const int POLL_7E7_CELL_85 = 0x4235;
  static const int POLL_7E7_CELL_86 = 0x4236;
  static const int POLL_7E7_CELL_87 = 0x4237;
  static const int POLL_7E7_CELL_88 = 0x4238;
  static const int POLL_7E7_CELL_89 = 0x4239;
  static const int POLL_7E7_CELL_90 = 0x423A;
  static const int POLL_7E7_CELL_91 = 0x423B;
  static const int POLL_7E7_CELL_92 = 0x423C;
  static const int POLL_7E7_CELL_93 = 0x423D;
  static const int POLL_7E7_CELL_94 = 0x423E;
  static const int POLL_7E7_CELL_95 = 0x423F;
  static const int POLL_7E7_CELL_96 = 0x4240;
  static const int POLL_7E7_CELL_97 = 0x4241;  // Normal pack ends at 96, these cells might be unpopulated
  static const int POLL_7E7_CELL_98 = 0x4242;
  static const int POLL_7E7_CELL_99 = 0x4243;
  static const int POLL_7E7_CELL_100 = 0x4244;
  static const int POLL_7E7_CELL_101 = 0x4245;
  static const int POLL_7E7_CELL_102 = 0x4246;
  static const int POLL_7E7_CELL_103 = 0x4247;
  static const int POLL_7E7_CELL_104 = 0x4248;
  static const int POLL_7E7_CELL_105 = 0x4249;
  static const int POLL_7E7_CELL_106 = 0x424A;
  static const int POLL_7E7_CELL_107 = 0x424B;
  static const int POLL_7E7_CELL_108 = 0x424C;
  static const int POLL_7E7_CELL_109 = 0x424D;
  static const int POLL_7E7_CELL_110 = 0x424E;
  static const int POLL_7E7_CELL_111 = 0x424F;
  static const int POLL_7E7_CELL_112 = 0x4250;
  static const int POLL_7E7_CELL_113 = 0x4251;
  static const int POLL_7E7_CELL_114 = 0x4252;
  static const int POLL_7E7_CELL_115 = 0x4253;
  static const int POLL_7E7_CELL_116 = 0x4254;
  static const int POLL_7E7_CELL_117 = 0x4255;
  static const int POLL_7E7_CELL_118 = 0x4256;
  static const int POLL_7E7_CELL_119 = 0x4257;
  static const int POLL_7E7_CELL_120 = 0x4258;

  unsigned long previousMillis20ms = 0;   // will store last time a 20ms CAN Message was send
  unsigned long previousMillis100ms = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis120ms = 0;  // will store last time a 120ms CAN Message was send

  CAN_frame BOLT_778 = {.FD = false,  // Unsure of what this message is, added only as example
                        .ext_ID = false,
                        .DLC = 7,
                        .ID = 0x778,
                        .data = {0x00, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BOLT_POLL_7E4 = {.FD = false,  // VICM_HV poll
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E4,
                             .data = {0x03, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BOLT_ACK_7E4 = {.FD = false,  //VICM_HV ack
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x7E4,
                            .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BOLT_POLL_7E7 = {.FD = false,  //VITM_HV poll
                             .ext_ID = false,
                             .DLC = 8,
                             .ID = 0x7E7,
                             .data = {0x03, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BOLT_ACK_7E7 = {.FD = false,  //VITM_HV ack
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x7E7,
                            .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // Other PID requests in the vehicle
  // All HV ECUs - 0x101
  // HPCC HV - 0x243 replies on 0x643
  // OBCM HV - 0x244 replies on 0x644
  // VICM_HV - 0x7E4 replies 0x7EC (This is battery)
  // VICM2_HV - 0x7E6 replies 0x7EF (Tis is battery also)
  // VITM_HV - 0x7E7 replies on 7EF (This is battery)

  uint16_t battery_cell_voltages[96];  //array with all the cellvoltages
  uint16_t battery_capacity_my17_18 = 0;
  uint16_t battery_capacity_my19plus = 0;
  uint16_t battery_SOC_display = 0;
  uint16_t battery_SOC_raw_highprec = 0;
  uint16_t battery_max_temperature = 0;
  uint16_t battery_min_temperature = 0;
  uint16_t battery_min_cell_voltage = 0;
  uint16_t battery_max_cell_voltage = 0;
  uint16_t battery_internal_resistance = 0;
  uint16_t battery_lowest_cell = 0;
  uint16_t battery_highest_cell = 0;
  uint16_t battery_voltage_polled = 0;
  uint16_t battery_voltage_periodic = 0;
  uint16_t battery_vehicle_isolation = 0;
  uint16_t battery_isolation_kohm = 0;
  uint16_t battery_HV_locked = 0;
  uint16_t battery_crash_event = 0;
  uint16_t battery_HVIL = 0;
  uint16_t battery_HVIL_status = 0;
  uint16_t battery_5V_ref = 0;
  int16_t battery_current_7E4 = 0;
  int16_t battery_module_temp_1 = 0;
  int16_t battery_module_temp_2 = 0;
  int16_t battery_module_temp_3 = 0;
  int16_t battery_module_temp_4 = 0;
  int16_t battery_module_temp_5 = 0;
  int16_t battery_module_temp_6 = 0;
  uint16_t battery_cell_average_voltage = 0;
  uint16_t battery_cell_average_voltage_2 = 0;
  uint16_t battery_terminal_voltage = 0;
  uint16_t battery_ignition_power_mode = 0;
  int16_t battery_current_7E7 = 0;
  int16_t temperature_1 = 0;
  int16_t temperature_2 = 0;
  int16_t temperature_3 = 0;
  int16_t temperature_4 = 0;
  int16_t temperature_5 = 0;
  int16_t temperature_6 = 0;
  int16_t temperature_highest = 0;
  int16_t temperature_lowest = 0;
  uint8_t mux = 0;
  uint8_t poll_index_7E4 = 0;
  uint16_t currentpoll_7E4 = POLL_7E4_CAPACITY_EST_GEN1;
  uint16_t reply_poll_7E4 = 0;
  uint8_t poll_index_7E7 = 0;
  uint16_t currentpoll_7E7 = POLL_7E7_CURRENT;
  uint16_t reply_poll_7E7 = 0;

  const uint16_t poll_commands_7E4[19] = {POLL_7E4_CAPACITY_EST_GEN1,
                                          POLL_7E4_CAPACITY_EST_GEN2,
                                          POLL_7E4_SOC_DISPLAY,
                                          POLL_7E4_SOC_RAW_HIGHPREC,
                                          POLL_7E4_MAX_TEMPERATURE,
                                          POLL_7E4_MIN_TEMPERATURE,
                                          POLL_7E4_MIN_CELL_V,
                                          POLL_7E4_MAX_CELL_V,
                                          POLL_7E4_INTERNAL_RES,
                                          POLL_7E4_LOWEST_CELL_NUMBER,
                                          POLL_7E4_HIGHEST_CELL_NUMBER,
                                          POLL_7E4_VOLTAGE,
                                          POLL_7E4_VEHICLE_ISOLATION,
                                          POLL_7E4_ISOLATION_TEST_KOHM,
                                          POLL_7E4_HV_LOCKED_OUT,
                                          POLL_7E4_CRASH_EVENT,
                                          POLL_7E4_HVIL,
                                          POLL_7E4_HVIL_STATUS,
                                          POLL_7E4_CURRENT};

  const uint16_t poll_commands_7E7[108] = {POLL_7E7_CURRENT,          POLL_7E7_5V_REF,
                                           POLL_7E7_MODULE_TEMP_1,    POLL_7E7_MODULE_TEMP_2,
                                           POLL_7E7_MODULE_TEMP_3,    POLL_7E7_MODULE_TEMP_4,
                                           POLL_7E7_MODULE_TEMP_5,    POLL_7E7_MODULE_TEMP_6,
                                           POLL_7E7_CELL_AVG_VOLTAGE, POLL_7E7_CELL_AVG_VOLTAGE_2,
                                           POLL_7E7_TERMINAL_VOLTAGE, POLL_7E7_IGNITION_POWER_MODE,
                                           POLL_7E7_CELL_01,          POLL_7E7_CELL_02,
                                           POLL_7E7_CELL_03,          POLL_7E7_CELL_04,
                                           POLL_7E7_CELL_05,          POLL_7E7_CELL_06,
                                           POLL_7E7_CELL_07,          POLL_7E7_CELL_08,
                                           POLL_7E7_CELL_09,          POLL_7E7_CELL_10,
                                           POLL_7E7_CELL_11,          POLL_7E7_CELL_12,
                                           POLL_7E7_CELL_13,          POLL_7E7_CELL_14,
                                           POLL_7E7_CELL_15,          POLL_7E7_CELL_16,
                                           POLL_7E7_CELL_17,          POLL_7E7_CELL_18,
                                           POLL_7E7_CELL_19,          POLL_7E7_CELL_20,
                                           POLL_7E7_CELL_21,          POLL_7E7_CELL_22,
                                           POLL_7E7_CELL_23,          POLL_7E7_CELL_24,
                                           POLL_7E7_CELL_25,          POLL_7E7_CELL_26,
                                           POLL_7E7_CELL_27,          POLL_7E7_CELL_28,
                                           POLL_7E7_CELL_29,          POLL_7E7_CELL_30,
                                           POLL_7E7_CELL_31,          POLL_7E7_CELL_32,
                                           POLL_7E7_CELL_33,          POLL_7E7_CELL_34,
                                           POLL_7E7_CELL_35,          POLL_7E7_CELL_36,
                                           POLL_7E7_CELL_37,          POLL_7E7_CELL_38,
                                           POLL_7E7_CELL_39,          POLL_7E7_CELL_40,
                                           POLL_7E7_CELL_41,          POLL_7E7_CELL_42,
                                           POLL_7E7_CELL_43,          POLL_7E7_CELL_44,
                                           POLL_7E7_CELL_45,          POLL_7E7_CELL_46,
                                           POLL_7E7_CELL_47,          POLL_7E7_CELL_48,
                                           POLL_7E7_CELL_49,          POLL_7E7_CELL_50,
                                           POLL_7E7_CELL_51,          POLL_7E7_CELL_52,
                                           POLL_7E7_CELL_53,          POLL_7E7_CELL_54,
                                           POLL_7E7_CELL_55,          POLL_7E7_CELL_56,
                                           POLL_7E7_CELL_57,          POLL_7E7_CELL_58,
                                           POLL_7E7_CELL_59,          POLL_7E7_CELL_60,
                                           POLL_7E7_CELL_61,          POLL_7E7_CELL_62,
                                           POLL_7E7_CELL_63,          POLL_7E7_CELL_64,
                                           POLL_7E7_CELL_65,          POLL_7E7_CELL_66,
                                           POLL_7E7_CELL_67,          POLL_7E7_CELL_68,
                                           POLL_7E7_CELL_69,          POLL_7E7_CELL_70,
                                           POLL_7E7_CELL_71,          POLL_7E7_CELL_72,
                                           POLL_7E7_CELL_73,          POLL_7E7_CELL_74,
                                           POLL_7E7_CELL_75,          POLL_7E7_CELL_76,
                                           POLL_7E7_CELL_77,          POLL_7E7_CELL_78,
                                           POLL_7E7_CELL_79,          POLL_7E7_CELL_80,
                                           POLL_7E7_CELL_81,          POLL_7E7_CELL_82,
                                           POLL_7E7_CELL_83,          POLL_7E7_CELL_84,
                                           POLL_7E7_CELL_85,          POLL_7E7_CELL_86,
                                           POLL_7E7_CELL_87,          POLL_7E7_CELL_88,
                                           POLL_7E7_CELL_89,          POLL_7E7_CELL_90,
                                           POLL_7E7_CELL_91,          POLL_7E7_CELL_92,
                                           POLL_7E7_CELL_93,          POLL_7E7_CELL_94,
                                           POLL_7E7_CELL_95,          POLL_7E7_CELL_96};
};

#endif
