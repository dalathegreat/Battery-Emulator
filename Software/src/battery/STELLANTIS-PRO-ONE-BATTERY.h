#ifndef STELLANTIS_PRO_ONE_BATTERY_H
#define STELLANTIS_PRO_ONE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class StellantisProOneBattery : public UdsCanBattery {
 public:
  StellantisProOneBattery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis Pro One 110kWh (E-Ducato/ProMaster/Proace)";

  String get_uds_info_html() override;

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int MAX_PACK_VOLTAGE_DV = 3780;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2880;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  CAN_frame ONE_15A = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x15A, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_1D7 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D7,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_175 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x175,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_108 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x108,
                       .data = {0x00, 0x00, 0x00, 0x3D, 0x09, 0x00, 0x00, 0x00}};
  CAN_frame ONE_1D8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D8,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  uint8_t expectedCRC = 0;
  uint8_t counter_10ms = 0;        //Counter for the 10ms CAN message, goes from 0-0xF and starts over
  uint8_t counter_20ms = 0;        //Counter for the 20ms CAN message, goes from 0-0xF and starts over
  uint8_t sent_10ms_messages = 0;  //Counter for the number of 10ms messages sent, goes from 0-0xFF and starts over
  uint8_t sent_20ms_messages = 0;  //Counter for the number of 10ms messages sent, goes from 0-0xFF and starts over

  static const uint16_t PID_UNKNOWN_1 = 0x0100;  //Multi frame reply
  static const uint16_t PID_UNKNOWN_2 = 0x0103;
  static const uint16_t PID_UNKNOWN_3 = 0x0107;
  static const uint16_t PID_UNKNOWN_4 = 0x010C;
  static const uint16_t PID_UNKNOWN_5 = 0x7603;
  static const uint16_t PID_UNKNOWN_6 = 0xA000;
  static const uint16_t PID_UNKNOWN_7 = 0xA001;
  static const uint16_t PID_UNKNOWN_8 = 0xA002;
  static const uint16_t PID_UNKNOWN_9 = 0xA003;
  static const uint16_t PID_UNKNOWN_10 = 0xA004;
  static const uint16_t PID_UNKNOWN_11 = 0xA005;
  static const uint16_t PID_UNKNOWN_12 = 0xA006;
  static const uint16_t PID_CELL_MIN_MAX = 0xA009;
  static const uint16_t PID_UNKNOWN_14 = 0xA00A;
  static const uint16_t PID_UNKNOWN_15 = 0xA010;
  static const uint16_t PID_UNKNOWN_16 = 0xA011;
  static const uint16_t PID_UNKNOWN_17 = 0xA014;
  static const uint16_t PID_UNKNOWN_18 = 0xA017;
  static const uint16_t PID_UNKNOWN_19 = 0xA019;
  static const uint16_t PID_UNKNOWN_20 = 0xA01A;
  static const uint16_t PID_UNKNOWN_21 = 0xA020;
  static const uint16_t PID_UNKNOWN_22 = 0xA021;
  static const uint16_t PID_UNKNOWN_23 = 0xA022;
  static const uint16_t PID_UNKNOWN_24 = 0xA024;
  static const uint16_t PID_UNKNOWN_25 = 0xA029;
  static const uint16_t PID_UNKNOWN_26 = 0xA041;
  static const uint16_t PID_UNKNOWN_27 = 0xA042;
  static const uint16_t PID_UNKNOWN_28 = 0xA055;
  static const uint16_t PID_UNKNOWN_29 = 0xA056;
  static const uint16_t PID_UNKNOWN_30 = 0xA057;
  static const uint16_t PID_UNKNOWN_31 = 0xA058;
  static const uint16_t PID_UNKNOWN_32 = 0xA059;
  static const uint16_t PID_UNKNOWN_33 = 0xA060;
  static const uint16_t PID_UNKNOWN_34 = 0xA070;
  static const uint16_t PID_UNKNOWN_35 = 0xA071;
  static const uint16_t PID_UNKNOWN_36 = 0xA072;
  static const uint16_t PID_UNKNOWN_37 = 0xA073;
  static const uint16_t PID_UNKNOWN_38 = 0xA074;
  static const uint16_t PID_UNKNOWN_39 = 0xA075;
  static const uint16_t PID_UNKNOWN_40 = 0xA076;
  static const uint16_t PID_CELLVOLTAGES_1 = 0xA100;
  static const uint16_t PID_CELLVOLTAGES_2 = 0xA101;
  static const uint16_t PID_CELLVOLTAGES_3 = 0xA102;
  static const uint16_t PID_CELLVOLTAGES_4 = 0xA103;
  static const uint16_t PID_CELLVOLTAGES_5 = 0xA104;
  static const uint16_t PID_CELLVOLTAGES_6 = 0xA105;
  static const uint16_t PID_CELLVOLTAGES_7 = 0xA106;
  static const uint16_t PID_CELLVOLTAGES_8 = 0xA107;
  static const uint16_t PID_CELLTEMPERATURES_ALL = 0xA200;
  /*
  static const uint16_t PID_UNKNOWN_51 = 0xA222; //All of these PIDs contain the same temperatures over and over again
  static const uint16_t PID_UNKNOWN_52 = 0xA223; //It is like a broken record, some of them are really large multiframes with only
  static const uint16_t PID_UNKNOWN_53 = 0xA224; //10% of it populated with the same 30 measurements over and over again
  static const uint16_t PID_UNKNOWN_54 = 0xA225;
  static const uint16_t PID_UNKNOWN_55 = 0xA226;
  static const uint16_t PID_UNKNOWN_56 = 0xA227;
  static const uint16_t PID_UNKNOWN_57 = 0xA228;
  static const uint16_t PID_UNKNOWN_58 = 0xA229;
  static const uint16_t PID_UNKNOWN_59 = 0xA230;
  static const uint16_t PID_UNKNOWN_60 = 0xA231;
  static const uint16_t PID_UNKNOWN_61 = 0xA232;
  static const uint16_t PID_UNKNOWN_62 = 0xA233;
  static const uint16_t PID_UNKNOWN_63 = 0xA234;
  static const uint16_t PID_UNKNOWN_64 = 0xA235;
  static const uint16_t PID_UNKNOWN_65 = 0xA302;
  static const uint16_t PID_UNKNOWN_66 = 0xA303;
  static const uint16_t PID_UNKNOWN_67 = 0xA305;
  static const uint16_t PID_UNKNOWN_68 = 0xA309;
  static const uint16_t PID_UNKNOWN_69 = 0xA30C;
  static const uint16_t PID_UNKNOWN_70 = 0xA30D;
  static const uint16_t PID_UNKNOWN_71 = 0xA30E;
  static const uint16_t PID_UNKNOWN_72 = 0xA30F;
  static const uint16_t PID_UNKNOWN_73 = 0xA311;
  static const uint16_t PID_UNKNOWN_74 = 0xA312;
  static const uint16_t PID_UNKNOWN_75 = 0xA315;
  static const uint16_t PID_UNKNOWN_76 = 0xA319;
  static const uint16_t PID_UNKNOWN_77 = 0xA31B;
  static const uint16_t PID_UNKNOWN_78 = 0xA31D;
  static const uint16_t PID_UNKNOWN_79 = 0xA320;
  static const uint16_t PID_UNKNOWN_80 = 0xA322;
  static const uint16_t PID_UNKNOWN_81 = 0xA323;
  static const uint16_t PID_UNKNOWN_82 = 0xA324;
  static const uint16_t PID_UNKNOWN_83 = 0xA325;
  static const uint16_t PID_UNKNOWN_84 = 0xA326;
  static const uint16_t PID_UNKNOWN_85 = 0xA327;
  static const uint16_t PID_UNKNOWN_86 = 0xA328;
  static const uint16_t PID_UNKNOWN_87 = 0xA329;
  static const uint16_t PID_UNKNOWN_88 = 0xA32A;
  static const uint16_t PID_UNKNOWN_89 = 0xA330;
  static const uint16_t PID_UNKNOWN_90 = 0xA332;
  static const uint16_t PID_UNKNOWN_91 = 0xA334;
  static const uint16_t PID_UNKNOWN_92 = 0xA335;
  static const uint16_t PID_UNKNOWN_93 = 0xA340;
  static const uint16_t PID_UNKNOWN_94 = 0xA350;
  static const uint16_t PID_UNKNOWN_95 = 0xA351;
  static const uint16_t PID_UNKNOWN_96 = 0xA352;
  static const uint16_t PID_UNKNOWN_97 = 0xA353;
  static const uint16_t PID_UNKNOWN_98 = 0xA355;
  static const uint16_t PID_UNKNOWN_99 = 0xA357;
  static const uint16_t PID_UNKNOWN_100 = 0xA358;
  static const uint16_t PID_UNKNOWN_101 = 0xA359;
  static const uint16_t PID_UNKNOWN_102 = 0xA360;
  static const uint16_t PID_UNKNOWN_103 = 0xA361;
  static const uint16_t PID_UNKNOWN_104 = 0xA362;
  static const uint16_t PID_UNKNOWN_105 = 0xA363;
  static const uint16_t PID_UNKNOWN_106 = 0xA364;
  static const uint16_t PID_UNKNOWN_107 = 0xA365;
  static const uint16_t PID_UNKNOWN_108 = 0xA367;
  static const uint16_t PID_UNKNOWN_109 = 0xA368;
  static const uint16_t PID_UNKNOWN_110 = 0xA370;
  static const uint16_t PID_UNKNOWN_111 = 0xA371;
  static const uint16_t PID_UNKNOWN_112 = 0xA372;
  static const uint16_t PID_UNKNOWN_113 = 0xA373;
  static const uint16_t PID_UNKNOWN_114 = 0xA374;
  static const uint16_t PID_UNKNOWN_115 = 0xA375;
  static const uint16_t PID_UNKNOWN_116 = 0xA376;
  static const uint16_t PID_UNKNOWN_117 = 0xA377;
  static const uint16_t PID_UNKNOWN_118 = 0xA378;
  static const uint16_t PID_UNKNOWN_119 = 0xA379;
  static const uint16_t PID_UNKNOWN_120 = 0xA380;
  static const uint16_t PID_UNKNOWN_121 = 0xA381;
  static const uint16_t PID_UNKNOWN_122 = 0xA382;
  static const uint16_t PID_UNKNOWN_123 = 0xA383;
  static const uint16_t PID_UNKNOWN_124 = 0xA384;
  static const uint16_t PID_UNKNOWN_125 = 0xA385;
  static const uint16_t PID_UNKNOWN_126 = 0xA386;
  static const uint16_t PID_UNKNOWN_127 = 0xA387;
  static const uint16_t PID_UNKNOWN_128 = 0xA388;
  static const uint16_t PID_UNKNOWN_129 = 0xA389;
  static const uint16_t PID_UNKNOWN_130 = 0xA390;
  static const uint16_t PID_UNKNOWN_131 = 0xA391;
  static const uint16_t PID_UNKNOWN_132 = 0xA393;
  static const uint16_t PID_UNKNOWN_133 = 0xA394;
  static const uint16_t PID_UNKNOWN_134 = 0xA395;
  static const uint16_t PID_UNKNOWN_135 = 0xA396;
  static const uint16_t PID_UNKNOWN_136 = 0xA397;
  static const uint16_t PID_UNKNOWN_137 = 0xA398;
  static const uint16_t PID_UNKNOWN_138 = 0xA399;
  static const uint16_t PID_UNKNOWN_139 = 0xA400;
  static const uint16_t PID_UNKNOWN_140 = 0xA401;
  static const uint16_t PID_UNKNOWN_141 = 0xA402;
  static const uint16_t PID_UNKNOWN_142 = 0xA403;
  static const uint16_t PID_UNKNOWN_143 = 0xA404;
  static const uint16_t PID_UNKNOWN_144 = 0xA405;
  static const uint16_t PID_UNKNOWN_145 = 0xA406;
  static const uint16_t PID_UNKNOWN_146 = 0xA407;
  static const uint16_t PID_UNKNOWN_147 = 0xA408;
  static const uint16_t PID_UNKNOWN_148 = 0xA409;
  static const uint16_t PID_UNKNOWN_149 = 0xA410;
  static const uint16_t PID_UNKNOWN_150 = 0xA411;
  static const uint16_t PID_UNKNOWN_151 = 0xA412;
  static const uint16_t PID_UNKNOWN_152 = 0xA413;
  static const uint16_t PID_UNKNOWN_153 = 0xA414;
  static const uint16_t PID_UNKNOWN_154 = 0xA415;
  static const uint16_t PID_UNKNOWN_155 = 0xA416;
  static const uint16_t PID_UNKNOWN_156 = 0xA417;
  static const uint16_t PID_UNKNOWN_157 = 0xA418;
  static const uint16_t PID_UNKNOWN_158 = 0xA419;
  static const uint16_t PID_UNKNOWN_159 = 0xA420;
  static const uint16_t PID_UNKNOWN_160 = 0xA421;
  static const uint16_t PID_UNKNOWN_161 = 0xA422; //Temperatures stop here
  */
  static const uint16_t PID_UNKNOWN_162 = 0xB000;
  static const uint16_t PID_UNKNOWN_163 = 0xB001;
  static const uint16_t PID_UNKNOWN_164 = 0xB002;
  static const uint16_t PID_UNKNOWN_165 = 0xB003;
  static const uint16_t PID_UNKNOWN_166 = 0xB004;
  static const uint16_t PID_UNKNOWN_167 = 0xB005;
  static const uint16_t PID_UNKNOWN_168 = 0xB006;
  static const uint16_t PID_UNKNOWN_169 = 0xB007;
  static const uint16_t PID_UNKNOWN_170 = 0xB008;
  static const uint16_t PID_UNKNOWN_171 = 0xB009;
  static const uint16_t PID_UNKNOWN_172 = 0xB00A;
  static const uint16_t PID_UNKNOWN_173 = 0xB00B;
  static const uint16_t PID_UNKNOWN_174 = 0xB017;
  static const uint16_t PID_UNKNOWN_175 = 0xB018;
  static const uint16_t PID_UNKNOWN_176 = 0xB019;
  static const uint16_t PID_UNKNOWN_177 = 0xD001;
  static const uint16_t PID_UNKNOWN_178 = 0xDA75;
  static const uint16_t PID_UNKNOWN_179 = 0xDA76;
  static const uint16_t PID_UNKNOWN_180 = 0xDA77;
  static const uint16_t PID_UNKNOWN_181 = 0xDA78;
  static const uint16_t PID_UNKNOWN_182 = 0xDA79;
  static const uint16_t PID_UNKNOWN_183 = 0xDA7B;
  static const uint16_t PID_UNKNOWN_184 = 0xDA7C;
  static const uint16_t PID_UNKNOWN_185 = 0xDA7D;
  static const uint16_t PID_UNKNOWN_186 = 0xDA7E;
  static const uint16_t PID_UNKNOWN_187 = 0xDA7F;
  static const uint16_t PID_UNKNOWN_188 = 0xDA80;
  static const uint16_t PID_UNKNOWN_189 = 0xDA81;
  static const uint16_t PID_UNKNOWN_190 = 0xDA83;
  static const uint16_t PID_UNKNOWN_191 = 0xDA84;
  static const uint16_t PID_UNKNOWN_192 = 0xF010;
  static const uint16_t PID_UNKNOWN_193 = 0xF100;
  static const uint16_t PID_UNKNOWN_194 = 0xF10B;
  static const uint16_t PID_UNKNOWN_195 = 0xF10D;
  static const uint16_t PID_UNKNOWN_196 = 0xF112;
  static const uint16_t PID_UNKNOWN_197 = 0xF122;
  static const uint16_t PID_UNKNOWN_198 = 0xF132;
  static const uint16_t PID_UNKNOWN_199 = 0xF150;
  static const uint16_t PID_UNKNOWN_200 = 0xF151;
  static const uint16_t PID_UNKNOWN_201 = 0xF153;
  static const uint16_t PID_UNKNOWN_202 = 0xF154;
  static const uint16_t PID_UNKNOWN_203 = 0xF155;
  static const uint16_t PID_UNKNOWN_204 = 0xF158;
  static const uint16_t PID_UNKNOWN_205 = 0xF15B;
  static const uint16_t PID_UNKNOWN_206 = 0xF160;
  static const uint16_t PID_UNKNOWN_207 = 0xF161;
  static const uint16_t PID_UNKNOWN_208 = 0xF170;
  static const uint16_t PID_UNKNOWN_209 = 0xF171;
  static const uint16_t PID_UNKNOWN_210 = 0xF185;
  static const uint16_t PID_UNKNOWN_211 = 0xF187;  //Battery type?
  static const uint16_t PID_UNKNOWN_212 = 0xF188;
  static const uint16_t PID_UNKNOWN_213 = 0xF18C;
  static const uint16_t PID_VIN = 0xF190;
  static const uint16_t PID_UNKNOWN_215 = 0xF191;
  static const uint16_t PID_UNKNOWN_216 = 0xF192;
  static const uint16_t PID_HW_VERSION_NUM = 0xF193;
  static const uint16_t PID_UNKNOWN_218 = 0xF194;
  static const uint16_t PID_UNKNOWN_219 = 0xF195;
  static const uint16_t PID_SW_HOMOLOGATION_CODE = 0xF196;
  static const uint16_t PID_UNKNOWN_221 = 0xF1A0;
  static const uint16_t PID_UNKNOWN_222 = 0xF1B0;
  static const uint16_t PID_UNKNOWN_223 = 0xF804;
  static const uint16_t PID_UNKNOWN_224 = 0xF806;

  int8_t celltemperatures[30] = {0};
  bool temperaturesSampledOnce = false;
  bool cellvoltagesSampledOnce = false;  //TODO: Remove once pack voltage is found, crude sum method
  uint8_t pid_hw_version_num = 0;
  uint64_t pid_sw_homologation_code = 0;
  uint16_t pid_unknown_12 = 0;
  uint32_t pid_unknown_178 = 0;
  uint16_t pid_unknown_179 = 0;
  uint8_t pid_unknown_180 = 0;
  uint8_t pid_unknown_181 = 0;
  uint8_t pid_unknown_182 = 0;
};

#endif
