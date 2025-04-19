#ifndef BOLT_AMPERA_BATTERY_H
#define BOLT_AMPERA_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#define BATTERY_SELECTED

#define MAX_PACK_VOLTAGE_DV 4150  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 2500
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

#define POLL_7E4_CAPACITY_EST_GEN1 0x41A3
#define POLL_7E4_CAPACITY_EST_GEN2 0x45F9
#define POLL_7E4_SOC_DISPLAY 0x8334
#define POLL_7E4_SOC_RAW_HIGHPREC 0x43AF
#define POLL_7E4_MAX_TEMPERATURE 0x4349
#define POLL_7E4_MIN_TEMPERATURE 0x434A
#define POLL_7E4_MIN_CELL_V 0x4329
#define POLL_7E4_MAX_CELL_V 0x432B
#define POLL_7E4_INTERNAL_RES 0x40E9
#define POLL_7E4_LOWEST_CELL_NUMBER 0x433B
#define POLL_7E4_HIGHEST_CELL_NUMBER 0x433C
#define POLL_7E4_VOLTAGE 0x432D
#define POLL_7E4_VEHICLE_ISOLATION 0x41EC
#define POLL_7E4_ISOLATION_TEST_KOHM 0x43A6
#define POLL_7E4_HV_LOCKED_OUT 0x44F8
#define POLL_7E4_CRASH_EVENT 0x4522
#define POLL_7E4_HVIL 0x4310
#define POLL_7E4_HVIL_STATUS 0x4311
#define POLL_7E4_CURRENT 0x4356

#define POLL_7E7_CURRENT 0x40D4
#define POLL_7E7_5V_REF 0x40D3
#define POLL_7E7_MODULE_TEMP_1 0x40D7
#define POLL_7E7_MODULE_TEMP_2 0x40D9
#define POLL_7E7_MODULE_TEMP_3 0x40DB
#define POLL_7E7_MODULE_TEMP_4 0x40DD
#define POLL_7E7_MODULE_TEMP_5 0x40DF
#define POLL_7E7_MODULE_TEMP_6 0x40E1
#define POLL_7E7_CELL_AVG_VOLTAGE 0xC218
#define POLL_7E7_CELL_AVG_VOLTAGE_2 0x44B9
#define POLL_7E7_TERMINAL_VOLTAGE 0x82A3
#define POLL_7E7_IGNITION_POWER_MODE 0x8002
#define POLL_7E7_GMLAN_HIGH_SPEED_STATUS 0x8043
#define POLL_7E7_HV_ISOLATION_RESISTANCE 0x43A6
#define POLL_7E7_HV_BUS_VOLTAGE 0x438B
#define POLL_7E7_HYBRID_CELL_BALANCING_ID_1 0x4323
#define POLL_7E7_HYBRID_CELL_BALANCING_ID_2 0x4324
#define POLL_7E7_HYBRID_CELL_BALANCING_ID_3 0x4325
#define POLL_7E7_HYBRID_CELL_BALANCING_ID_4 0x4326
#define POLL_7E7_HYBRID_CELL_BALANCING_ID_5 0x4327
#define POLL_7E7_HYBRID_CELL_BALANCING_ID_6 0x4340
#define POLL_7E7_HYBRID_BATTERY_CELL_BALANCE_STATUS 0x431C
#define POLL_7E7_5V_REF_VOLTAGE_1 0x428F
#define POLL_7E7_5V_REF_VOLTAGE_2 0x4290
#define POLL_7E7_CELL_01 0x4181
#define POLL_7E7_CELL_02 0x4182
#define POLL_7E7_CELL_03 0x4183
#define POLL_7E7_CELL_04 0x4184
#define POLL_7E7_CELL_05 0x4185
#define POLL_7E7_CELL_06 0x4186
#define POLL_7E7_CELL_07 0x4187
#define POLL_7E7_CELL_08 0x4188
#define POLL_7E7_CELL_09 0x4189
#define POLL_7E7_CELL_10 0x418A
#define POLL_7E7_CELL_11 0x418B
#define POLL_7E7_CELL_12 0x418C
#define POLL_7E7_CELL_13 0x418D
#define POLL_7E7_CELL_14 0x418E
#define POLL_7E7_CELL_15 0x418F
#define POLL_7E7_CELL_16 0x4190
#define POLL_7E7_CELL_17 0x4191
#define POLL_7E7_CELL_18 0x4192
#define POLL_7E7_CELL_19 0x4193
#define POLL_7E7_CELL_20 0x4194
#define POLL_7E7_CELL_21 0x4195
#define POLL_7E7_CELL_22 0x4196
#define POLL_7E7_CELL_23 0x4197
#define POLL_7E7_CELL_24 0x4198
#define POLL_7E7_CELL_25 0x4199
#define POLL_7E7_CELL_26 0x419A
#define POLL_7E7_CELL_27 0x419B
#define POLL_7E7_CELL_28 0x419C
#define POLL_7E7_CELL_29 0x419D
#define POLL_7E7_CELL_30 0x419E
#define POLL_7E7_CELL_31 0x419F
#define POLL_7E7_CELL_32 0x4200
#define POLL_7E7_CELL_33 0x4201
#define POLL_7E7_CELL_34 0x4202
#define POLL_7E7_CELL_35 0x4203
#define POLL_7E7_CELL_36 0x4204
#define POLL_7E7_CELL_37 0x4205
#define POLL_7E7_CELL_38 0x4206
#define POLL_7E7_CELL_39 0x4207
#define POLL_7E7_CELL_40 0x4208
#define POLL_7E7_CELL_41 0x4209
#define POLL_7E7_CELL_42 0x420A
#define POLL_7E7_CELL_43 0x420B
#define POLL_7E7_CELL_44 0x420C
#define POLL_7E7_CELL_45 0x420D
#define POLL_7E7_CELL_46 0x420E
#define POLL_7E7_CELL_47 0x420F
#define POLL_7E7_CELL_48 0x4210
#define POLL_7E7_CELL_49 0x4211
#define POLL_7E7_CELL_50 0x4212
#define POLL_7E7_CELL_51 0x4213
#define POLL_7E7_CELL_52 0x4214
#define POLL_7E7_CELL_53 0x4215
#define POLL_7E7_CELL_54 0x4216
#define POLL_7E7_CELL_55 0x4217
#define POLL_7E7_CELL_56 0x4218
#define POLL_7E7_CELL_57 0x4219
#define POLL_7E7_CELL_58 0x421A
#define POLL_7E7_CELL_59 0x421B
#define POLL_7E7_CELL_60 0x421C
#define POLL_7E7_CELL_61 0x421D
#define POLL_7E7_CELL_62 0x421E
#define POLL_7E7_CELL_63 0x421F
#define POLL_7E7_CELL_64 0x4220
#define POLL_7E7_CELL_65 0x4221
#define POLL_7E7_CELL_66 0x4222
#define POLL_7E7_CELL_67 0x4223
#define POLL_7E7_CELL_68 0x4224
#define POLL_7E7_CELL_69 0x4225
#define POLL_7E7_CELL_70 0x4226
#define POLL_7E7_CELL_71 0x4227
#define POLL_7E7_CELL_72 0x4228
#define POLL_7E7_CELL_73 0x4229
#define POLL_7E7_CELL_74 0x422A
#define POLL_7E7_CELL_75 0x422B
#define POLL_7E7_CELL_76 0x422C
#define POLL_7E7_CELL_77 0x422D
#define POLL_7E7_CELL_78 0x422E
#define POLL_7E7_CELL_79 0x422F
#define POLL_7E7_CELL_80 0x4230
#define POLL_7E7_CELL_81 0x4231
#define POLL_7E7_CELL_82 0x4232
#define POLL_7E7_CELL_83 0x4233
#define POLL_7E7_CELL_84 0x4234
#define POLL_7E7_CELL_85 0x4235
#define POLL_7E7_CELL_86 0x4236
#define POLL_7E7_CELL_87 0x4237
#define POLL_7E7_CELL_88 0x4238
#define POLL_7E7_CELL_89 0x4239
#define POLL_7E7_CELL_90 0x423A
#define POLL_7E7_CELL_91 0x423B
#define POLL_7E7_CELL_92 0x423C
#define POLL_7E7_CELL_93 0x423D
#define POLL_7E7_CELL_94 0x423E
#define POLL_7E7_CELL_95 0x423F
#define POLL_7E7_CELL_96 0x4240
#define POLL_7E7_CELL_97 0x4241  // Normal pack ends at 96, these cells might be unpopulated
#define POLL_7E7_CELL_98 0x4242
#define POLL_7E7_CELL_99 0x4243
#define POLL_7E7_CELL_100 0x4244
#define POLL_7E7_CELL_101 0x4245
#define POLL_7E7_CELL_102 0x4246
#define POLL_7E7_CELL_103 0x4247
#define POLL_7E7_CELL_104 0x4248
#define POLL_7E7_CELL_105 0x4249
#define POLL_7E7_CELL_106 0x424A
#define POLL_7E7_CELL_107 0x424B
#define POLL_7E7_CELL_108 0x424C
#define POLL_7E7_CELL_109 0x424D
#define POLL_7E7_CELL_110 0x424E
#define POLL_7E7_CELL_111 0x424F
#define POLL_7E7_CELL_112 0x4250
#define POLL_7E7_CELL_113 0x4251
#define POLL_7E7_CELL_114 0x4252
#define POLL_7E7_CELL_115 0x4253
#define POLL_7E7_CELL_116 0x4254
#define POLL_7E7_CELL_117 0x4255
#define POLL_7E7_CELL_118 0x4256
#define POLL_7E7_CELL_119 0x4257
#define POLL_7E7_CELL_120 0x4258

class BoltAmperaBattery : public CanBattery {
 public:
  BoltAmperaBattery() : CanBattery(BoltAmpera) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Chevrolet Bolt EV/Opel Ampera-e";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

 private:
  /*TODO, messages we might need to send towards the battery to keep it happy and close contactors
0x262 Battery Block Voltage Diag Status HV (Who sends this? Battery?)
0x272 Battery Cell Voltage Diag Status HV (Who sends this? Battery?)
0x274 Battery Temperature Sensor diagnostic status HV (Who sends this? Battery?)
0x270 Battery VoltageSensor BalancingSwitches diagnostic status (Who sends this? Battery?)
0x214 Charger coolant temp info HV
0x20E Hybrid balancing request HV
0x30E High Voltage Charger Command HV
0x30C HVEM Provide Charging HV
0x316 OBHV Charge Process PEV HV
0x30F OBHV Charg Statn Current stat HV
0x312 OBHV Charg Statns Energy allocation HV
0x310 OBHV Charg Statn Vlt Energy Power HV
0x306 Off board HVCS Limit HV
0x309 Off board HVCS Min Limit HV
0x305 Vehicle Charging limit stat HV
0x314 Vehicle req energy transfer HV <<<<<<<<<< Sounds like contactor request resides here TODO
0x460 Energy Storage System Temp HV (Who sends this? Battery?)
*/

  /* Do not change code below unless you are sure what you are doing */
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
};

#endif
