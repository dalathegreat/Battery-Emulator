#ifndef RENAULT_ZOE_GEN2_BATTERY_H
#define RENAULT_ZOE_GEN2_BATTERY_H

#include "CanBattery.h"
#include "RENAULT-ZOE-GEN2-HTML.h"

class RenaultZoeGen2Battery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  RenaultZoeGen2Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_ZOE_PH2* extended,
                        CAN_Interface targetCan)
      : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    datalayer_zoePH2 = extended;

    battery_pack_voltage_periodic_dV = 0;
  }

  // Use the default constructor to create the first or single battery.
  RenaultZoeGen2Battery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_zoePH2 = &datalayer_extended.zoePH2;
  }
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Renault Zoe Gen2 50kWh";

  bool supports_reset_NVROL() { return true; }

  void reset_NVROL() { datalayer_extended.zoePH2.UserRequestNVROLReset = true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

  uint8_t calculate_crc_zoe(CAN_frame& frame, uint8_t crc_xor);

 private:
  RenaultZoeGen2HtmlRenderer renderer;

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_ZOE_PH2* datalayer_zoePH2;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  bool is_message_corrupt(CAN_frame rx_frame, uint8_t crc_xor);

  static const int MAX_PACK_VOLTAGE_DV = 4100;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  static const int POLL_SOC = 0x9001;
  static const int POLL_USABLE_SOC = 0x9002;
  static const int POLL_SOH = 0x9003;
  static const int POLL_PACK_VOLTAGE = 0x9005;
  static const int POLL_MAX_CELL_VOLTAGE = 0x9007;
  static const int POLL_MIN_CELL_VOLTAGE = 0x9009;
  static const int POLL_12V = 0x9011;
  static const int POLL_AVG_TEMP = 0x9012;
  static const int POLL_MIN_TEMP = 0x9013;
  static const int POLL_MAX_TEMP = 0x9014;
  static const int POLL_MAX_POWER = 0x9018;
  static const int POLL_INTERLOCK = 0x901A;
  static const int POLL_KWH = 0x91C8;
  static const int POLL_CURRENT = 0x925D;
  static const int POLL_CURRENT_OFFSET = 0x900C;
  static const int POLL_MAX_GENERATED = 0x900E;
  static const int POLL_MAX_AVAILABLE = 0x900F;
  static const int POLL_CURRENT_VOLTAGE = 0x9130;
  static const int POLL_CHARGING_STATUS = 0x9019;
  static const int POLL_REMAINING_CHARGE = 0xF45B;
  static const int POLL_BALANCE_CAPACITY_TOTAL = 0x924F;
  static const int POLL_BALANCE_TIME_TOTAL = 0x9250;
  static const int POLL_BALANCE_CAPACITY_SLEEP = 0x9251;
  static const int POLL_BALANCE_TIME_SLEEP = 0x9252;
  static const int POLL_BALANCE_CAPACITY_WAKE = 0x9262;
  static const int POLL_BALANCE_TIME_WAKE = 0x9263;
  static const int POLL_BMS_STATE = 0x9259;
  static const int POLL_BALANCE_SWITCHES = 0x912B;
  static const int POLL_ENERGY_COMPLETE = 0x9210;
  static const int POLL_ENERGY_PARTIAL = 0x9215;
  static const int POLL_SLAVE_FAILURES = 0x9129;
  static const int POLL_MILEAGE = 0x91CF;
  static const int POLL_FAN_SPEED = 0x912E;
  static const int POLL_FAN_PERIOD = 0x91F4;
  static const int POLL_FAN_CONTROL = 0x91C9;
  static const int POLL_FAN_DUTY = 0x91F5;
  static const int POLL_TEMPORISATION = 0x9281;
  static const int POLL_TIME = 0x9261;
  static const int POLL_PACK_TIME = 0x91C1;
  static const int POLL_SOC_MIN = 0x91B9;
  static const int POLL_SOC_MAX = 0x91BA;

  static const int POLL_CELL_0 = 0x9021;
  static const int POLL_CELL_1 = 0x9022;
  static const int POLL_CELL_2 = 0x9023;
  static const int POLL_CELL_3 = 0x9024;
  static const int POLL_CELL_4 = 0x9025;
  static const int POLL_CELL_5 = 0x9026;
  static const int POLL_CELL_6 = 0x9027;
  static const int POLL_CELL_7 = 0x9028;
  static const int POLL_CELL_8 = 0x9029;
  static const int POLL_CELL_9 = 0x902A;
  static const int POLL_CELL_10 = 0x902B;
  static const int POLL_CELL_11 = 0x902C;
  static const int POLL_CELL_12 = 0x902D;
  static const int POLL_CELL_13 = 0x902E;
  static const int POLL_CELL_14 = 0x902F;
  static const int POLL_CELL_15 = 0x9030;
  static const int POLL_CELL_16 = 0x9031;
  static const int POLL_CELL_17 = 0x9032;
  static const int POLL_CELL_18 = 0x9033;
  static const int POLL_CELL_19 = 0x9034;
  static const int POLL_CELL_20 = 0x9035;
  static const int POLL_CELL_21 = 0x9036;
  static const int POLL_CELL_22 = 0x9037;
  static const int POLL_CELL_23 = 0x9038;
  static const int POLL_CELL_24 = 0x9039;
  static const int POLL_CELL_25 = 0x903A;
  static const int POLL_CELL_26 = 0x903B;
  static const int POLL_CELL_27 = 0x903C;
  static const int POLL_CELL_28 = 0x903D;
  static const int POLL_CELL_29 = 0x903E;
  static const int POLL_CELL_30 = 0x903F;
  static const int POLL_CELL_31 = 0x9041;
  static const int POLL_CELL_32 = 0x9042;
  static const int POLL_CELL_33 = 0x9043;
  static const int POLL_CELL_34 = 0x9044;
  static const int POLL_CELL_35 = 0x9045;
  static const int POLL_CELL_36 = 0x9046;
  static const int POLL_CELL_37 = 0x9047;
  static const int POLL_CELL_38 = 0x9048;
  static const int POLL_CELL_39 = 0x9049;
  static const int POLL_CELL_40 = 0x904A;
  static const int POLL_CELL_41 = 0x904B;
  static const int POLL_CELL_42 = 0x904C;
  static const int POLL_CELL_43 = 0x904D;
  static const int POLL_CELL_44 = 0x904E;
  static const int POLL_CELL_45 = 0x904F;
  static const int POLL_CELL_46 = 0x9050;
  static const int POLL_CELL_47 = 0x9051;
  static const int POLL_CELL_48 = 0x9052;
  static const int POLL_CELL_49 = 0x9053;
  static const int POLL_CELL_50 = 0x9054;
  static const int POLL_CELL_51 = 0x9055;
  static const int POLL_CELL_52 = 0x9056;
  static const int POLL_CELL_53 = 0x9057;
  static const int POLL_CELL_54 = 0x9058;
  static const int POLL_CELL_55 = 0x9059;
  static const int POLL_CELL_56 = 0x905A;
  static const int POLL_CELL_57 = 0x905B;
  static const int POLL_CELL_58 = 0x905C;
  static const int POLL_CELL_59 = 0x905D;
  static const int POLL_CELL_60 = 0x905E;
  static const int POLL_CELL_61 = 0x905F;
  static const int POLL_CELL_62 = 0x9061;
  static const int POLL_CELL_63 = 0x9062;
  static const int POLL_CELL_64 = 0x9063;
  static const int POLL_CELL_65 = 0x9064;
  static const int POLL_CELL_66 = 0x9065;
  static const int POLL_CELL_67 = 0x9066;
  static const int POLL_CELL_68 = 0x9067;
  static const int POLL_CELL_69 = 0x9068;
  static const int POLL_CELL_70 = 0x9069;
  static const int POLL_CELL_71 = 0x906A;
  static const int POLL_CELL_72 = 0x906B;
  static const int POLL_CELL_73 = 0x906C;
  static const int POLL_CELL_74 = 0x906D;
  static const int POLL_CELL_75 = 0x906E;
  static const int POLL_CELL_76 = 0x906F;
  static const int POLL_CELL_77 = 0x9070;
  static const int POLL_CELL_78 = 0x9071;
  static const int POLL_CELL_79 = 0x9072;
  static const int POLL_CELL_80 = 0x9073;
  static const int POLL_CELL_81 = 0x9074;
  static const int POLL_CELL_82 = 0x9075;
  static const int POLL_CELL_83 = 0x9076;
  static const int POLL_CELL_84 = 0x9077;
  static const int POLL_CELL_85 = 0x9078;
  static const int POLL_CELL_86 = 0x9079;
  static const int POLL_CELL_87 = 0x907A;
  static const int POLL_CELL_88 = 0x907B;
  static const int POLL_CELL_89 = 0x907C;
  static const int POLL_CELL_90 = 0x907D;
  static const int POLL_CELL_91 = 0x907E;
  static const int POLL_CELL_92 = 0x907F;
  static const int POLL_CELL_93 = 0x9081;
  static const int POLL_CELL_94 = 0x9082;
  static const int POLL_CELL_95 = 0x9083;
  volatile unsigned long startTimeNVROL = 0;
  uint8_t NVROLstateMachine = 0;
  uint16_t battery_soc = 0;
  uint16_t battery_usable_soc = 5000;
  uint16_t battery_soh = 10000;
  uint16_t battery_pack_voltage_polled_dV = 3700;
  uint16_t battery_pack_voltage_periodic_dV = 3700;
  uint16_t battery_minimum_cell_voltage_mV = 3700;
  uint16_t battery_maximum_cell_voltage_mV = 3700;
  uint16_t battery_max_cell_voltage_polled = 3700;
  uint16_t battery_min_cell_voltage_polled = 3700;
  uint16_t battery_12v = 12000;
  uint16_t battery_avg_temp = 920;
  uint16_t battery_min_temp = 920;
  uint16_t battery_max_temp = 920;
  uint16_t battery_max_power = 0;
  uint16_t battery_interlock = 0xFFFE;
  uint16_t battery_interlock_polled = 0;
  uint16_t battery_kwh = 0;
  int32_t battery_current = 32640;
  uint16_t battery_current_offset = 0;
  uint16_t battery_max_generated = 0;
  uint16_t battery_max_available = 0;
  uint16_t battery_current_voltage = 0;
  uint16_t battery_charging_status = 0;
  uint16_t battery_remaining_charge = 0;
  uint16_t battery_balance_capacity_total = 0;
  uint16_t battery_balance_time_total = 0;
  uint16_t battery_balance_capacity_sleep = 0;
  uint16_t battery_balance_time_sleep = 0;
  uint16_t battery_balance_capacity_wake = 0;
  uint16_t battery_balance_time_wake = 0;
  uint16_t battery_bms_state = 0;
  uint16_t battery_balance_switches = 0;
  uint16_t battery_energy_complete = 0;
  uint16_t battery_energy_partial = 0;
  uint16_t battery_slave_failures = 0;
  uint16_t battery_mileage = 0;
  uint16_t battery_fan_speed = 0;
  uint16_t battery_fan_period = 0;
  uint16_t battery_fan_control = 0;
  uint16_t battery_fan_duty = 0;
  uint16_t battery_temporisation = 0;
  uint16_t battery_time = 0;
  uint16_t battery_pack_time = 0;
  uint16_t battery_soc_min = 0;
  uint16_t battery_soc_max = 0;
  uint16_t temporary_variable = 0;
  uint32_t ZOE_376_time_now_s = 1745452800;  // Initialized to make the battery think it is April 24, 2025
  unsigned long kProductionTimestamp_s =
      1614454107;  // Production timestamp in seconds since January 1, 1970. Production timestamp used: February 25, 2021 at 8:08:27 AM GMT
  bool battery_balancing_shunts[96];

  const uint8_t crctable[256] = {
      0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0, 0xF7,
      0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0, 0xF3, 0xEE,
      0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C, 0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19, 0xA2,
      0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40, 0xFB, 0xE6, 0xC1, 0xDC,
      0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78,
      0x65, 0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B, 0x08, 0x15, 0x32, 0x2F, 0x59, 0x44,
      0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01, 0x52,
      0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D, 0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8,
      0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73,
      0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B, 0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED,
      0xCA, 0xD7, 0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E, 0xF8,
      0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB, 0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95,
      0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09, 0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31,
      0x2C, 0x97, 0x8A, 0xAD, 0xB0, 0xE3, 0xFE, 0xD9, 0xC4};
  CAN_frame ZOE_0EE = {//Pedal position
                       .FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0EE,
                       .data = {0x32, 0x3, 0x20, 0xAA, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ZOE_373 = {//HEVC sender, wakeup message
                       .FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x373,
                       .data = {0xC1, 0x40, 0x5D, 0xB2, 0x00, 0x01, 0xff, 0xe3}};
  CAN_frame ZOE_375 = {//HEVC status message
                       .FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x375,
                       .data = {0x02, 0x29, 0x00, 0xBF, 0xFE, 0x64, 0x0, 0xff}};
  CAN_frame ZOE_376 = {
      //HEVC sender
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x376,
      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
               0x00}};  // fill first 6 bytes with 0's. The first 6 bytes are calculated based on the current time.
  CAN_frame ZOE_5F8 = { //Vehicle ID
                       .FD = false,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x5F8,
                       .data = {0x16, 0x44, 0x90, 0x8F}};
  CAN_frame ZOE_6BF = {//Total Boost Time
                       .FD = false,
                       .ext_ID = false,
                       .DLC = 3,
                       .ID = 0x6BF,
                       .data = {0x00, 0x00, 0x00}};
  CAN_frame ZOE_POLL_18DADBF1 = {.FD = false,
                                 .ext_ID = true,
                                 .DLC = 8,
                                 .ID = 0x18DADBF1,
                                 .data = {0x03, 0x22, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ZOE_POLL_FLOW_CONTROL = {.FD = false,
                                     .ext_ID = true,
                                     .DLC = 8,
                                     .ID = 0x18DADBF1,
                                     .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  //NVROL Reset
  CAN_frame ZOE_NVROL_1_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
  CAN_frame ZOE_NVROL_2_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA}};
  //Enable temporisation before sleep
  CAN_frame ZOE_SLEEP_1_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
  CAN_frame ZOE_SLEEP_2_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA}};

  const uint16_t poll_commands[163] = {POLL_SOC,
                                       POLL_USABLE_SOC,
                                       POLL_SOH,
                                       POLL_PACK_VOLTAGE,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_MAX_CELL_VOLTAGE,
                                       POLL_MIN_CELL_VOLTAGE,
                                       POLL_12V,
                                       POLL_AVG_TEMP,
                                       POLL_MIN_TEMP,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_MAX_TEMP,
                                       POLL_MAX_POWER,
                                       POLL_INTERLOCK,
                                       POLL_KWH,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CURRENT_OFFSET,
                                       POLL_MAX_GENERATED,
                                       POLL_MAX_AVAILABLE,
                                       POLL_CURRENT_VOLTAGE,
                                       POLL_CHARGING_STATUS,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_REMAINING_CHARGE,
                                       POLL_BALANCE_CAPACITY_TOTAL,
                                       POLL_BALANCE_TIME_TOTAL,
                                       POLL_BALANCE_CAPACITY_SLEEP,
                                       POLL_BALANCE_TIME_SLEEP,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_BALANCE_CAPACITY_WAKE,
                                       POLL_BALANCE_TIME_WAKE,
                                       POLL_BMS_STATE,
                                       POLL_BALANCE_SWITCHES,
                                       POLL_ENERGY_COMPLETE,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_ENERGY_PARTIAL,
                                       POLL_SLAVE_FAILURES,
                                       POLL_MILEAGE,
                                       POLL_FAN_SPEED,
                                       POLL_FAN_PERIOD,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_FAN_CONTROL,
                                       POLL_FAN_DUTY,
                                       POLL_TEMPORISATION,
                                       POLL_TIME,
                                       POLL_PACK_TIME,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_SOC_MIN,
                                       POLL_SOC_MAX,
                                       POLL_CELL_0,
                                       POLL_CELL_1,
                                       POLL_CELL_2,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_3,
                                       POLL_CELL_4,
                                       POLL_CELL_5,
                                       POLL_CELL_6,
                                       POLL_CELL_7,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_8,
                                       POLL_CELL_9,
                                       POLL_CELL_10,
                                       POLL_CELL_11,
                                       POLL_CELL_12,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_13,
                                       POLL_CELL_14,
                                       POLL_CELL_15,
                                       POLL_CELL_16,
                                       POLL_CELL_17,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_18,
                                       POLL_CELL_19,
                                       POLL_CELL_20,
                                       POLL_CELL_21,
                                       POLL_CELL_22,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_23,
                                       POLL_CELL_24,
                                       POLL_CELL_25,
                                       POLL_CELL_26,
                                       POLL_CELL_27,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_28,
                                       POLL_CELL_29,
                                       POLL_CELL_30,
                                       POLL_CELL_31,
                                       POLL_CELL_32,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_33,
                                       POLL_CELL_34,
                                       POLL_CELL_35,
                                       POLL_CELL_36,
                                       POLL_CELL_37,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_38,
                                       POLL_CELL_39,
                                       POLL_CELL_40,
                                       POLL_CELL_41,
                                       POLL_CELL_42,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_43,
                                       POLL_CELL_44,
                                       POLL_CELL_45,
                                       POLL_CELL_46,
                                       POLL_CELL_47,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_48,
                                       POLL_CELL_49,
                                       POLL_CELL_50,
                                       POLL_CELL_51,
                                       POLL_CELL_52,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_53,
                                       POLL_CELL_54,
                                       POLL_CELL_55,
                                       POLL_CELL_56,
                                       POLL_CELL_57,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_58,
                                       POLL_CELL_59,
                                       POLL_CELL_60,
                                       POLL_CELL_61,
                                       POLL_CELL_62,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_63,
                                       POLL_CELL_64,
                                       POLL_CELL_65,
                                       POLL_CELL_66,
                                       POLL_CELL_67,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_68,
                                       POLL_CELL_69,
                                       POLL_CELL_70,
                                       POLL_CELL_71,
                                       POLL_CELL_72,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_73,
                                       POLL_CELL_74,
                                       POLL_CELL_75,
                                       POLL_CELL_76,
                                       POLL_CELL_77,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_78,
                                       POLL_CELL_79,
                                       POLL_CELL_80,
                                       POLL_CELL_81,
                                       POLL_CELL_82,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_83,
                                       POLL_CELL_84,
                                       POLL_CELL_85,
                                       POLL_CELL_86,
                                       POLL_CELL_87,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_88,
                                       POLL_CELL_89,
                                       POLL_CELL_90,
                                       POLL_CELL_91,
                                       POLL_CELL_92,
                                       POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                       POLL_CELL_93,
                                       POLL_CELL_94,
                                       POLL_CELL_95};
  uint8_t counter_373 = 0;
  uint8_t poll_index = 0;
  uint16_t currentpoll = POLL_SOC;
  uint16_t reply_poll = 0;
  uint8_t counter_10ms = 0;
  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis200 = 0;   // will store last time a 200ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  /**
 * @brief Transmit CAN frame 0x376
 * 
 * @param[in] void
 * 
 * @return void
 * 
 */
  void transmit_can_frame_376(void);

  /**
 * @brief Reset NVROL, by sending specific frames
 *
 * @param[in] void
 *
 * @return void
 */
  void transmit_reset_nvrol_frames(void);

  /**
 * @brief Wait function
 *
 * @param[in] duration_ms wait duration in ms
 *
 * @return void
 */
  void wait_ms(int duration_ms);
};

#endif
