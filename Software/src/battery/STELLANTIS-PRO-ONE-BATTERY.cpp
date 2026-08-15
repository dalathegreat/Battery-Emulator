#include "STELLANTIS-PRO-ONE-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

static uint8_t CalculateCRC8SAEJ1850(CAN_frame rx_frame, uint8_t length) {
  uint8_t crc = 0xFF;  // initial value 0xFF
  for (uint8_t j = 0; j < length;
       j++) {  //Length is the number of bytes to calculate CRC for, not including the CRC byte itself!
    crc = crc8_table_SAE_J1850_ZER0[crc ^ rx_frame.data.u8[j]];
  }
  return crc ^ 0xFF;  // final XOR 0xFF
}

void StellantisProOneBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //datalayer.battery.status.real_soc; //TODO: locate
  //datalayer.battery.status.soh_pptt; //TODO: locate
  //datalayer.battery.status.voltage_dV; //TODO: locate
  //datalayer.battery.status.current_dA; //TODO: locate
  //datalayer.battery.status.remaining_capacity_Wh; //TODO: locate
  //datalayer.battery.status.max_discharge_power_W; //TODO: locate
  //datalayer.battery.status.max_charge_power_W; //TODO: locate
  //datalayer.battery.status.cell_max_voltage_mV; //TODO: locate
  //datalayer.battery.status.cell_min_voltage_mV; //TODO: locate
  //datalayer.battery.status.temperature_min_dC; //TODO: locate
  //datalayer.battery.status.temperature_max_dC; //TODO: locate
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String StellantisProOneBattery::get_uds_info_html() {
  String content;
  content.reserve(600);

  // clang-format off
  content << "<h4>Pack voltage: " << pid_pack_voltage << " dV</h4>"
              "<h4>12V voltage: " << lead_acid_voltage << "mV</h4>"
              "<h4>Lowest temperature: " << pid_lowest_temperature << "°C</h4>"
              "<h4>Highest temperature: " << pid_highest_temperature << "°C</h4>";
  // clang-format on

  return content;
}

void StellantisProOneBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x7E7 PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  switch (rx_frame.ID) {
    case 0x95:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0xE0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x107:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x150:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      expectedCRC = CalculateCRC8SAEJ1850(rx_frame, 7);
      if (expectedCRC == rx_frame.data.u8[7]) {
        //Message is valid, process it
      } else {
        //CRC error, ignore message
        datalayer.battery.status.CAN_error_counter++;
      }
      break;
    case 0x1D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x220:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x281:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x285:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x306:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x307:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x312:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x354:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x358:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EC:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3ED:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EF:  //UDS reply
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

uint16_t StellantisProOneBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_UNKNOWN_1:
      break;
    case PID_UNKNOWN_2:
      break;
    case PID_UNKNOWN_3:
      break;
    case PID_UNKNOWN_4:
      break;
    case PID_UNKNOWN_5:
      break;
    case PID_UNKNOWN_6:
      break;
    case PID_UNKNOWN_7:
      break;
    case PID_UNKNOWN_8:
      break;
    case PID_UNKNOWN_9:
      break;
    case PID_UNKNOWN_10:
      break;
    case PID_UNKNOWN_11:
      break;
    case PID_UNKNOWN_12:
      break;
    case PID_UNKNOWN_13:
      break;
    case PID_UNKNOWN_14:
      break;
    case PID_UNKNOWN_15:
      break;
    case PID_UNKNOWN_16:
      break;
    case PID_UNKNOWN_17:
      break;
    case PID_UNKNOWN_18:
      break;
    case PID_UNKNOWN_19:
      break;
    case PID_UNKNOWN_20:
      break;
    case PID_UNKNOWN_21:
      break;
    case PID_UNKNOWN_22:
      break;
    case PID_UNKNOWN_23:
      break;
    case PID_UNKNOWN_24:
      break;
    case PID_UNKNOWN_25:
      break;
    case PID_UNKNOWN_26:
      break;
    case PID_UNKNOWN_27:
      break;
    case PID_UNKNOWN_28:
      break;
    case PID_UNKNOWN_29:
      break;
    case PID_UNKNOWN_30:
      break;
    case PID_UNKNOWN_31:
      break;
    case PID_UNKNOWN_32:
      break;
    case PID_UNKNOWN_33:
      break;
    case PID_UNKNOWN_34:
      break;
    case PID_UNKNOWN_35:
      break;
    case PID_UNKNOWN_36:
      break;
    case PID_UNKNOWN_37:
      break;
    case PID_UNKNOWN_38:
      break;
    case PID_UNKNOWN_39:
      break;
    case PID_UNKNOWN_40:
      break;
    case PID_UNKNOWN_41:
      break;
    case PID_UNKNOWN_42:
      break;
    case PID_UNKNOWN_43:
      break;
    case PID_UNKNOWN_44:
      break;
    case PID_UNKNOWN_45:
      break;
    case PID_UNKNOWN_46:
      break;
    case PID_UNKNOWN_47:
      break;
    case PID_UNKNOWN_48:
      break;
    case PID_UNKNOWN_49:
      break;
    case PID_UNKNOWN_50:
      break;
    case PID_UNKNOWN_51:
      break;
    case PID_UNKNOWN_52:
      break;
    case PID_UNKNOWN_53:
      break;
    case PID_UNKNOWN_54:
      break;
    case PID_UNKNOWN_55:
      break;
    case PID_UNKNOWN_56:
      break;
    case PID_UNKNOWN_57:
      break;
    case PID_UNKNOWN_58:
      break;
    case PID_UNKNOWN_59:
      break;
    case PID_UNKNOWN_60:
      break;
    case PID_UNKNOWN_61:
      break;
    case PID_UNKNOWN_62:
      break;
    case PID_UNKNOWN_63:
      break;
    case PID_UNKNOWN_64:
      break;
    case PID_UNKNOWN_65:
      break;
    case PID_UNKNOWN_66:
      break;
    case PID_UNKNOWN_67:
      break;
    case PID_UNKNOWN_68:
      break;
    case PID_UNKNOWN_69:
      break;
    case PID_UNKNOWN_70:
      break;
    case PID_UNKNOWN_71:
      break;
    case PID_UNKNOWN_72:
      break;
    case PID_UNKNOWN_73:
      break;
    case PID_UNKNOWN_74:
      break;
    case PID_UNKNOWN_75:
      break;
    case PID_UNKNOWN_76:
      break;
    case PID_UNKNOWN_77:
      break;
    case PID_UNKNOWN_78:
      break;
    case PID_UNKNOWN_79:
      break;
    case PID_UNKNOWN_80:
      break;
    case PID_UNKNOWN_81:
      break;
    case PID_UNKNOWN_82:
      break;
    case PID_UNKNOWN_83:
      break;
    case PID_UNKNOWN_84:
      break;
    case PID_UNKNOWN_85:
      break;
    case PID_UNKNOWN_86:
      break;
    case PID_UNKNOWN_87:
      break;
    case PID_UNKNOWN_88:
      break;
    case PID_UNKNOWN_89:
      break;
    case PID_UNKNOWN_90:
      break;
    case PID_UNKNOWN_91:
      break;
    case PID_UNKNOWN_92:
      break;
    case PID_UNKNOWN_93:
      break;
    case PID_UNKNOWN_94:
      break;
    case PID_UNKNOWN_95:
      break;
    case PID_UNKNOWN_96:
      break;
    case PID_UNKNOWN_97:
      break;
    case PID_UNKNOWN_98:
      break;
    case PID_UNKNOWN_99:
      break;
    case PID_UNKNOWN_100:
      break;
    case PID_UNKNOWN_101:
      break;
    case PID_UNKNOWN_102:
      break;
    case PID_UNKNOWN_103:
      break;
    case PID_UNKNOWN_104:
      break;
    case PID_UNKNOWN_105:
      break;
    case PID_UNKNOWN_106:
      break;
    case PID_UNKNOWN_107:
      break;
    case PID_UNKNOWN_108:
      break;
    case PID_UNKNOWN_109:
      break;
    case PID_UNKNOWN_110:
      break;
    case PID_UNKNOWN_111:
      break;
    case PID_UNKNOWN_112:
      break;
    case PID_UNKNOWN_113:
      break;
    case PID_UNKNOWN_114:
      break;
    case PID_UNKNOWN_115:
      break;
    case PID_UNKNOWN_116:
      break;
    case PID_UNKNOWN_117:
      break;
    case PID_UNKNOWN_118:
      break;
    case PID_UNKNOWN_119:
      break;
    case PID_UNKNOWN_120:
      break;
    case PID_UNKNOWN_121:
      break;
    case PID_UNKNOWN_122:
      break;
    case PID_UNKNOWN_123:
      break;
    case PID_UNKNOWN_124:
      break;
    case PID_UNKNOWN_125:
      break;
    case PID_UNKNOWN_126:
      break;
    case PID_UNKNOWN_127:
      break;
    case PID_UNKNOWN_128:
      break;
    case PID_UNKNOWN_129:
      break;
    case PID_UNKNOWN_130:
      break;
    case PID_UNKNOWN_131:
      break;
    case PID_UNKNOWN_132:
      break;
    case PID_UNKNOWN_133:
      break;
    case PID_UNKNOWN_134:
      break;
    case PID_UNKNOWN_135:
      break;
    case PID_UNKNOWN_136:
      break;
    case PID_UNKNOWN_137:
      break;
    case PID_UNKNOWN_138:
      break;
    case PID_UNKNOWN_139:
      break;
    case PID_UNKNOWN_140:
      break;
    case PID_UNKNOWN_141:
      break;
    case PID_UNKNOWN_142:
      break;
    case PID_UNKNOWN_143:
      break;
    case PID_UNKNOWN_144:
      break;
    case PID_UNKNOWN_145:
      break;
    case PID_UNKNOWN_146:
      break;
    case PID_UNKNOWN_147:
      break;
    case PID_UNKNOWN_148:
      break;
    case PID_UNKNOWN_149:
      break;
    case PID_UNKNOWN_150:
      break;
    case PID_UNKNOWN_151:
      break;
    case PID_UNKNOWN_152:
      break;
    case PID_UNKNOWN_153:
      break;
    case PID_UNKNOWN_154:
      break;
    case PID_UNKNOWN_155:
      break;
    case PID_UNKNOWN_156:
      break;
    case PID_UNKNOWN_157:
      break;
    case PID_UNKNOWN_158:
      break;
    case PID_UNKNOWN_159:
      break;
    case PID_UNKNOWN_160:
      break;
    case PID_UNKNOWN_161:
      break;
    case PID_UNKNOWN_162:
      break;
    case PID_UNKNOWN_163:
      break;
    case PID_UNKNOWN_164:
      break;
    case PID_UNKNOWN_165:
      break;
    case PID_UNKNOWN_166:
      break;
    case PID_UNKNOWN_167:
      break;
    case PID_UNKNOWN_168:
      break;
    case PID_UNKNOWN_169:
      break;
    case PID_UNKNOWN_170:
      break;
    case PID_UNKNOWN_171:
      break;
    case PID_UNKNOWN_172:
      break;
    case PID_UNKNOWN_173:
      break;
    case PID_UNKNOWN_174:
      break;
    case PID_UNKNOWN_175:
      break;
    case PID_UNKNOWN_176:
      break;
    case PID_UNKNOWN_177:
      break;
    case PID_UNKNOWN_178:
      break;
    case PID_UNKNOWN_179:
      break;
    case PID_UNKNOWN_180:
      break;
    case PID_UNKNOWN_181:
      break;
    case PID_UNKNOWN_182:
      break;
    case PID_UNKNOWN_183:
      break;
    case PID_UNKNOWN_184:
      break;
    case PID_UNKNOWN_185:
      break;
    case PID_UNKNOWN_186:
      break;
    case PID_UNKNOWN_187:
      break;
    case PID_UNKNOWN_188:
      break;
    case PID_UNKNOWN_189:
      break;
    case PID_UNKNOWN_190:
      break;
    case PID_UNKNOWN_191:
      break;
    case PID_UNKNOWN_192:
      break;
    case PID_UNKNOWN_193:
      break;
    case PID_UNKNOWN_194:
      break;
    case PID_UNKNOWN_195:
      break;
    case PID_UNKNOWN_196:
      break;
    case PID_UNKNOWN_197:
      break;
    case PID_UNKNOWN_198:
      break;
    case PID_UNKNOWN_199:
      break;
    case PID_UNKNOWN_200:
      break;
    case PID_UNKNOWN_201:
      break;
    case PID_UNKNOWN_202:
      break;
    case PID_UNKNOWN_203:
      break;
    case PID_UNKNOWN_204:
      break;
    case PID_UNKNOWN_205:
      break;
    case PID_UNKNOWN_206:
      break;
    case PID_UNKNOWN_207:
      break;
    case PID_UNKNOWN_208:
      break;
    case PID_UNKNOWN_209:
      break;
    case PID_UNKNOWN_210:
      break;
    case PID_UNKNOWN_211:
      break;
    case PID_UNKNOWN_212:
      break;
    case PID_UNKNOWN_213:
      break;
    case PID_UNKNOWN_214:
      break;
    case PID_UNKNOWN_215:
      break;
    case PID_UNKNOWN_216:
      break;
    case PID_UNKNOWN_217:
      break;
    case PID_UNKNOWN_218:
      break;
    case PID_UNKNOWN_219:
      break;
    case PID_UNKNOWN_220:
      break;
    case PID_UNKNOWN_221:
      break;
    case PID_UNKNOWN_222:
      break;
    case PID_UNKNOWN_223:
      break;
    case PID_UNKNOWN_224:
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void StellantisProOneBattery::transmit_can(unsigned long currentMillis) {

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    //Counter goes from 0-0xF and starts over
    counter_10ms = (counter_10ms + 1) & 0x0F;
    //Sentmessages counter
    if (sent_10ms_messages < 254) {
      sent_10ms_messages++;
    }

    if (sent_10ms_messages == 37) {
      ONE_108.data.u8[0] = 0x33;
      ONE_108.data.u8[1] = 0x1D;
      ONE_108.data.u8[2] = 0xF0;
    } else if (sent_10ms_messages == 42) {
      ONE_108.data.u8[1] = 0x25;
      ONE_108.data.u8[2] = 0xC0;
    } else {
    }

    ONE_15A.data.u8[2] = counter_10ms;
    ONE_15A.data.u8[3] = CalculateCRC8SAEJ1850(ONE_15A, 3);
    ONE_1D7.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_1D7.data.u8[7] = CalculateCRC8SAEJ1850(ONE_1D7, 7);
    ONE_175.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_175.data.u8[7] = CalculateCRC8SAEJ1850(ONE_175, 7);
    ONE_108.data.u8[6] = (counter_10ms & 0x0F) << 4;
    ONE_108.data.u8[7] = CalculateCRC8SAEJ1850(ONE_108, 7);

    transmit_can_frame(&ONE_15A);
    transmit_can_frame(&ONE_1D7);
    transmit_can_frame(&ONE_175);
    transmit_can_frame(&ONE_108);
    //transmit_can_frame(&ONE_0F2);
    //transmit_can_frame(&ONE_0F0);
    //transmit_can_frame(&ONE_0B4);

    ONE_175.data.u8[3] = 0x2E;
  }

  // Send 20ms CAN Message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    //Counter goes from 0-0xF and starts over
    counter_20ms = (counter_20ms + 1) & 0x0F;
    //Sentmessages counter
    if (sent_20ms_messages < 254) {
      sent_20ms_messages++;
    }

    //Safety checks for Contactor closing
    if ((datalayer.system.status.inverter_allows_contactor_closing == true) &&
        (datalayer.system.status.system_status != FAULT) && (!datalayer.system.info.equipment_stop_active)) {
      ONE_1D8.data.u8[4] = 0x21;
      ONE_1D8.data.u8[5] = 0x4F;
    } else {  //Closing not allowed
      ONE_1D8.data.u8[4] = 0x00;
      ONE_1D8.data.u8[5] = 0x00;
    }

    ONE_1D8.data.u8[6] = (counter_20ms & 0x0F) << 4;
    ONE_1D8.data.u8[7] = CalculateCRC8SAEJ1850(ONE_1D8, 7);

    transmit_can_frame(&ONE_1D8);  // Required for contactor closing
    //transmit_can_frame(&ONE_212);
    //transmit_can_frame(&ONE_160);
  }

  // Send 50ms CAN Message
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;
    //transmit_can_frame(&ONE_242);
    //transmit_can_frame(&ONE_240);
    //transmit_can_frame(&ONE_234);
    //transmit_can_frame(&ONE_235);
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //transmit_can_frame(&ONE_3E0);
    //transmit_can_frame(&ONE_3E7);
    //transmit_can_frame(&ONE_3EB);
    //transmit_can_frame(&ONE_3EE);
    //transmit_can_frame(&ONE_320);
    //transmit_can_frame(&ONE_321);
    //transmit_can_frame(&ONE_322);
    //transmit_can_frame(&ONE_323);
  }

  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);
}

void StellantisProOneBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  // UDS: send requests to 0x7E7, accept replies from the BMS on 0x7EF.
  setup_uds(0x7E7, 0x7EF);
  static const uint16_t pid_scan_list[] = {
      PID_UNKNOWN_1,   PID_UNKNOWN_2,   PID_UNKNOWN_3,   PID_UNKNOWN_4,   PID_UNKNOWN_5,   PID_UNKNOWN_6,
      PID_UNKNOWN_7,   PID_UNKNOWN_8,   PID_UNKNOWN_9,   PID_UNKNOWN_10,  PID_UNKNOWN_11,  PID_UNKNOWN_12,
      PID_UNKNOWN_13,  PID_UNKNOWN_14,  PID_UNKNOWN_15,  PID_UNKNOWN_16,  PID_UNKNOWN_17,  PID_UNKNOWN_18,
      PID_UNKNOWN_19,  PID_UNKNOWN_20,  PID_UNKNOWN_21,  PID_UNKNOWN_22,  PID_UNKNOWN_23,  PID_UNKNOWN_24,
      PID_UNKNOWN_25,  PID_UNKNOWN_26,  PID_UNKNOWN_27,  PID_UNKNOWN_28,  PID_UNKNOWN_29,  PID_UNKNOWN_30,
      PID_UNKNOWN_31,  PID_UNKNOWN_32,  PID_UNKNOWN_33,  PID_UNKNOWN_34,  PID_UNKNOWN_35,  PID_UNKNOWN_36,
      PID_UNKNOWN_37,  PID_UNKNOWN_38,  PID_UNKNOWN_39,  PID_UNKNOWN_40,  PID_UNKNOWN_41,  PID_UNKNOWN_42,
      PID_UNKNOWN_43,  PID_UNKNOWN_44,  PID_UNKNOWN_45,  PID_UNKNOWN_46,  PID_UNKNOWN_47,  PID_UNKNOWN_48,
      PID_UNKNOWN_49,  PID_UNKNOWN_50,  PID_UNKNOWN_51,  PID_UNKNOWN_52,  PID_UNKNOWN_53,  PID_UNKNOWN_54,
      PID_UNKNOWN_55,  PID_UNKNOWN_56,  PID_UNKNOWN_57,  PID_UNKNOWN_58,  PID_UNKNOWN_59,  PID_UNKNOWN_60,
      PID_UNKNOWN_61,  PID_UNKNOWN_62,  PID_UNKNOWN_63,  PID_UNKNOWN_64,  PID_UNKNOWN_65,  PID_UNKNOWN_66,
      PID_UNKNOWN_67,  PID_UNKNOWN_68,  PID_UNKNOWN_69,  PID_UNKNOWN_70,  PID_UNKNOWN_71,  PID_UNKNOWN_72,
      PID_UNKNOWN_73,  PID_UNKNOWN_74,  PID_UNKNOWN_75,  PID_UNKNOWN_76,  PID_UNKNOWN_77,  PID_UNKNOWN_78,
      PID_UNKNOWN_79,  PID_UNKNOWN_80,  PID_UNKNOWN_81,  PID_UNKNOWN_82,  PID_UNKNOWN_83,  PID_UNKNOWN_84,
      PID_UNKNOWN_85,  PID_UNKNOWN_86,  PID_UNKNOWN_87,  PID_UNKNOWN_88,  PID_UNKNOWN_89,  PID_UNKNOWN_90,
      PID_UNKNOWN_91,  PID_UNKNOWN_92,  PID_UNKNOWN_93,  PID_UNKNOWN_94,  PID_UNKNOWN_95,  PID_UNKNOWN_96,
      PID_UNKNOWN_97,  PID_UNKNOWN_98,  PID_UNKNOWN_99,  PID_UNKNOWN_100, PID_UNKNOWN_101, PID_UNKNOWN_102,
      PID_UNKNOWN_103, PID_UNKNOWN_104, PID_UNKNOWN_105, PID_UNKNOWN_106, PID_UNKNOWN_107, PID_UNKNOWN_108,
      PID_UNKNOWN_109, PID_UNKNOWN_110, PID_UNKNOWN_111, PID_UNKNOWN_112, PID_UNKNOWN_113, PID_UNKNOWN_114,
      PID_UNKNOWN_115, PID_UNKNOWN_116, PID_UNKNOWN_117, PID_UNKNOWN_118, PID_UNKNOWN_119, PID_UNKNOWN_120,
      PID_UNKNOWN_121, PID_UNKNOWN_122, PID_UNKNOWN_123, PID_UNKNOWN_124, PID_UNKNOWN_125, PID_UNKNOWN_126,
      PID_UNKNOWN_127, PID_UNKNOWN_128, PID_UNKNOWN_129, PID_UNKNOWN_130, PID_UNKNOWN_131, PID_UNKNOWN_132,
      PID_UNKNOWN_133, PID_UNKNOWN_134, PID_UNKNOWN_135, PID_UNKNOWN_136, PID_UNKNOWN_137, PID_UNKNOWN_138,
      PID_UNKNOWN_139, PID_UNKNOWN_140, PID_UNKNOWN_141, PID_UNKNOWN_142, PID_UNKNOWN_143, PID_UNKNOWN_144,
      PID_UNKNOWN_145, PID_UNKNOWN_146, PID_UNKNOWN_147, PID_UNKNOWN_148, PID_UNKNOWN_149, PID_UNKNOWN_150,
      PID_UNKNOWN_151, PID_UNKNOWN_152, PID_UNKNOWN_153, PID_UNKNOWN_154, PID_UNKNOWN_155, PID_UNKNOWN_156,
      PID_UNKNOWN_157, PID_UNKNOWN_158, PID_UNKNOWN_159, PID_UNKNOWN_160, PID_UNKNOWN_161, PID_UNKNOWN_162,
      PID_UNKNOWN_163, PID_UNKNOWN_164, PID_UNKNOWN_165, PID_UNKNOWN_166, PID_UNKNOWN_167, PID_UNKNOWN_168,
      PID_UNKNOWN_169, PID_UNKNOWN_170, PID_UNKNOWN_171, PID_UNKNOWN_172, PID_UNKNOWN_173, PID_UNKNOWN_174,
      PID_UNKNOWN_175, PID_UNKNOWN_176, PID_UNKNOWN_177, PID_UNKNOWN_178, PID_UNKNOWN_179, PID_UNKNOWN_180,
      PID_UNKNOWN_181, PID_UNKNOWN_182, PID_UNKNOWN_183, PID_UNKNOWN_184, PID_UNKNOWN_185, PID_UNKNOWN_186,
      PID_UNKNOWN_187, PID_UNKNOWN_188, PID_UNKNOWN_189, PID_UNKNOWN_190, PID_UNKNOWN_191, PID_UNKNOWN_192,
      PID_UNKNOWN_193, PID_UNKNOWN_194, PID_UNKNOWN_195, PID_UNKNOWN_196, PID_UNKNOWN_197, PID_UNKNOWN_198,
      PID_UNKNOWN_199, PID_UNKNOWN_200, PID_UNKNOWN_201, PID_UNKNOWN_202, PID_UNKNOWN_203, PID_UNKNOWN_204,
      PID_UNKNOWN_205, PID_UNKNOWN_206, PID_UNKNOWN_207, PID_UNKNOWN_208, PID_UNKNOWN_209, PID_UNKNOWN_210,
      PID_UNKNOWN_211, PID_UNKNOWN_212, PID_UNKNOWN_213, PID_UNKNOWN_214, PID_UNKNOWN_215, PID_UNKNOWN_216,
      PID_UNKNOWN_217, PID_UNKNOWN_218, PID_UNKNOWN_219, PID_UNKNOWN_220, PID_UNKNOWN_221, PID_UNKNOWN_222,
      PID_UNKNOWN_223, PID_UNKNOWN_224,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
