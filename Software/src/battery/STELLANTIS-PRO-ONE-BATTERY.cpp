#include "STELLANTIS-PRO-ONE-BATTERY.h"
#include <cstring>  //For unit test
#include <iomanip>
#include <sstream>
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

uint16_t estimate_SOC_based_on_v(uint16_t voltage) {
  // Voltage ranges between 3780dV when full, and 2880dV when empty
  // Range = 3780 - 2880 = 900dV → represents 100% SOC

  if (voltage <= 2880)
    return 0;
  if (voltage >= 3780)
    return 10000;

  return (uint32_t)(voltage - 2880) * 10000 / 900;
}

void StellantisProOneBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.voltage_dV = pack_voltage;

  datalayer.battery.status.real_soc =
      estimate_SOC_based_on_v(datalayer.battery.status.voltage_dV);  //TODO, locate real SOC and don't estimate!

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;  //TODO: locate

  datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;  //TODO: locate

  //datalayer.battery.status.soh_pptt; //TODO: locate
  //datalayer.battery.status.voltage_dV; //TODO: locate

  //datalayer.battery.status.remaining_capacity_Wh; //TODO: locate
  //datalayer.battery.status.max_discharge_power_W; //TODO: locate
  //datalayer.battery.status.max_charge_power_W; //TODO: locate

  //datalayer.battery.status.cell_max_voltage_mV; //Read in PID reply
  //datalayer.battery.status.cell_min_voltage_mV; //Read in PID reply

  if (temperaturesSampledOnce) {
    int8_t min_temp = celltemperatures[0];
    int8_t max_temp = celltemperatures[0];

    for (int i = 0; i < 30; i++) {
      if (celltemperatures[i] < min_temp)
        min_temp = celltemperatures[i];
      if (celltemperatures[i] > max_temp)
        max_temp = celltemperatures[i];
    }

    datalayer.battery.status.temperature_min_dC = (int16_t)min_temp * 10;
    datalayer.battery.status.temperature_max_dC = (int16_t)max_temp * 10;
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

std::string hexToAscii(uint64_t hexValue) {
  std::string result;
  // Find the most significant byte and work downward
  bool skipLeadingNonAscii = true;

  for (int shift = 48; shift >= 0; shift -= 8) {
    uint8_t byte = (hexValue >> shift) & 0xFF;
    if (skipLeadingNonAscii && (byte < 0x20 || byte > 0x7E)) {
      continue;  // skip non-printable/non-ASCII prefix bytes like 0x96
    }
    skipLeadingNonAscii = false;
    if (byte >= 0x20 && byte <= 0x7E) {
      result += static_cast<char>(byte);
    }
  }
  return result;
}

String StellantisProOneBattery::get_uds_info_html() {
  String content;
  content.reserve(600);

  // clang-format off
  content << "<h4>BMS HW version number: " << pid_hw_version_num << "</h4>"
              "<h4>SW Homologation Code: " << hexToAscii(pid_sw_homologation_code).c_str() << "</h4>"
              "<h4>PID AA06: " << pid_unknown_12 << "</h4>"
              "<h4>PID DA75: " << pid_unknown_178 << "</h4>"
              "<h4>PID DA76: " << pid_unknown_179 << "</h4>"
              "<h4>PID DA77: " << pid_unknown_180 << "</h4>"
              "<h4>PID DA78: " << pid_unknown_181 << "</h4>"
              "<h4>PID DA79: " << pid_unknown_182 << "</h4>"
              "<h4>306_1: " << unknown_306_0 << "</h4>"
              "<h4>306_2: " << unknown_306_1 << "</h4>"
              "<h4>306_3: " << unknown_306_2 << "</h4>"
              "<h4>285_1chg?: " << unknown_285_0 << "</h4>"
              "<h4>285_2chg?: " << unknown_285_1 << "</h4>"
              "<h4>285_3chg?: " << unknown_285_2 << "</h4>"
              "<h4>281_1: " << unknown_281_0 << "</h4>"
              "<h4>281_2: " << unknown_281_1 << "</h4>"
              "<h4>220_1chg?: " << unknown_220_0 << "</h4>"
              "<h4>220_2chg?: " << unknown_220_1 << "</h4>"
              "<h4>220_3chg?: " << unknown_220_2 << "</h4>"
              "<h4>Temperature sensors: </h4>"
           "<table style='border-collapse:collapse;font-size:0.85em;margin:auto'>";

for (int row = 0; row < 6; row++) {
  content << "<tr>";
  for (int col = 0; col < 5; col++) {
    int8_t t = celltemperatures[row * 5 + col];
    const char* color = t < 10 ? "#66aaff" : t < 35 ? "#44bb44" : t < 45 ? "#ffaa00" : "#ff4444";
    content << "<td style='padding:4px 8px;text-align:center;color:" << color << "'>" << (int)t << "°C</td>";
  }
  content << "</tr>";
}

content << "</table>";
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
      expectedCRC = CalculateCRC8SAEJ1850(rx_frame, 7);
      if (expectedCRC == rx_frame.data.u8[7]) {
        //Message is valid, process it
        battery_current = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) - 15000;
        //counter_095 = (rx_frame.data.u8[6] & 0xF0) >> 4;
      } else {
        //CRC error, ignore message
        datalayer.battery.status.CAN_error_counter++;
      }
      break;
    case 0xE0:  //Always empty
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x107:  //Always empty
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
    case 0x220:  //Allowed Charge/Discharge?
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      unknown_220_0 = (uint16_t)((rx_frame.data.u8[0] & 0x0F) << 8) | rx_frame.data.u8[1];
      unknown_220_1 = (uint16_t)((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[3];
      unknown_220_2 = (uint16_t)((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5];
      break;
    case 0x281:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      unknown_281_0 = rx_frame.data.u8[1];
      unknown_281_1 = (uint16_t)((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[3];
      pack_voltage = (((((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5]) / 8) * 10);
      break;
    case 0x285:  //Allowed Charge/Discharge?
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      unknown_285_0 = (uint16_t)((rx_frame.data.u8[0] & 0x0F) << 8) | rx_frame.data.u8[1];
      unknown_285_1 = (uint16_t)((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[3];
      unknown_285_2 = (uint16_t)((rx_frame.data.u8[4] & 0x0F) << 8) | rx_frame.data.u8[5];
      break;
    case 0x306:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      unknown_306_0 = rx_frame.data.u8[4];
      unknown_306_1 = rx_frame.data.u8[5];
      unknown_306_2 = (uint16_t)((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[7];
      break;
    case 0x307:  //Could be temperatures?
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x312:  //Always empty
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x354:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x358:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x359:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EB:
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
      pid_unknown_12 = value;
      break;
    case PID_CELL_MIN_MAX:  //0xA009
      //000007E7,03 22 A0 09 55 55 55 55
      //000007EF,10 0F 62 A0 09 0B 07 0E
      //000007EF,21 C2 0E BA 1E 1C 80 15
      //000007EF,22 80 14 CC CC CC CC CC
      /* 0B 07 — unknown, possibly cell indices or module IDs
    0E C2 = 3778 — highest cell voltage (mV)
    0E BA = 3770 — lowest cell voltage (mV)
    1E 1C = 7708 — possibly pack voltage × some scale?
    80 15 and 80 14 — unknown, possibly temperatures or current*/
      datalayer.battery.status.cell_max_voltage_mV = (uint16_t)(data[2] << 8) | data[3];
      datalayer.battery.status.cell_min_voltage_mV = (uint16_t)(data[4] << 8) | data[5];
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
    case PID_CELLVOLTAGES_1:  //Cells 1-12
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_2:  //Cells 13-24
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[12 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_3:  //Cells 25-36
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[24 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_4:  //Cells 37-48
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[36 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_5:  //Cells 49-60
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[48 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_6:  //Cells 61-72
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[60 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_7:  //Cells 73-84
      if (length >= 24) {
        for (int i = 0; i < 12; i++) {
          datalayer.battery.status.cell_voltages_mV[72 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLVOLTAGES_8:  //Cells 85-90 (only 6 cells!)
      if (length >= 12) {
        for (int i = 0; i < 6; i++) {
          datalayer.battery.status.cell_voltages_mV[84 + i] = (uint16_t)(data[i * 2] << 8) | data[i * 2 + 1];
        }
      }
      break;
    case PID_CELLTEMPERATURES_ALL:  //Cell temperatures 1-30
      if (length >= 30) {
        for (int i = 0; i < 30; i++) {
          celltemperatures[i] = (int8_t)(data[i] - 50);
        }
        temperaturesSampledOnce = true;
      }
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
      pid_unknown_178 = value;  //3340
      break;
    case PID_UNKNOWN_179:
      pid_unknown_179 = value;  //200
      break;
    case PID_UNKNOWN_180:
      pid_unknown_180 = value;  //01
      break;
    case PID_UNKNOWN_181:
      pid_unknown_181 = value;  //200
      break;
    case PID_UNKNOWN_182:
      pid_unknown_182 = value;  //980
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
      //Sticker on battery: 46353370
      //This poll: 46358569
      break;
    case PID_UNKNOWN_212:
      //52248211
      break;
    case PID_UNKNOWN_213:
      break;
    case PID_VIN:
      //FF FF FF FF FF FF FF FF FF (Unavailable?)
      break;
    case PID_UNKNOWN_215:
      //52228267
      break;
    case PID_UNKNOWN_216:
      //000007EF,10 0E 62 F1 92 36 38 35
      //000007EF,21 33 35 31 33 38 41 42
      //000007EF,22 20 CC CC CC CC CC CC
      break;
    case PID_HW_VERSION_NUM:
      pid_hw_version_num = value;
      break;
    case PID_UNKNOWN_218:
      //000007EF,10 0E 62 F1 94 35 32 32
      //000007EF,21 34 38 32 31 31 20 20
      //000007EF,22 20 CC CC CC CC CC CC
      break;
    case PID_UNKNOWN_219:
      //000007E7,03 22 F1 95 55 55 55 55
      //000007EF,05 62 F1 95 00 00 CC CC
      break;
    case PID_SW_HOMOLOGATION_CODE:
      pid_sw_homologation_code = value;
      break;
    case PID_UNKNOWN_221:
      //000007EF,10 14 62 F1 A0 FF FF FF
      //000007E7,30 08 01 55 55 55 55 55
      //000007EF,21 FF FF FF FF FF FF FF
      //000007EF,22 FF FF FF FF FF FF FF
      break;
    case PID_UNKNOWN_222:
      //000007EF,10 14 62 F1 B0 FF FF FF
      //000007E7,30 08 01 55 55 55 55 55
      //000007EF,21 FF FF FF FF FF FF FF
      //000007EF,22 FF FF FF FF FF FF FF
      break;
    case PID_UNKNOWN_223:
      //Multi frame reply
      //000007EF,10 14 62 F8 04 14 14 14
      //000007EF,21 14 14 14 14 14 14 14
      //000007EF,22 14 14 14 14 14 14 14
      break;
    case PID_UNKNOWN_224:
      //Multi frame reply
      //000007EF,10 08 62 F8 06 14 14 14
      //000007EF,21 14 14 CC CC CC CC CC
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
  datalayer.battery.info.number_of_cells = 90;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  // UDS: send requests to 0x7E7, accept replies from the BMS on 0x7EF.
  setup_uds(0x7E7, 0x7EF);
  static const uint16_t pid_scan_list[] = {
      PID_UNKNOWN_1,
      PID_UNKNOWN_2,
      PID_UNKNOWN_3,
      PID_UNKNOWN_4,
      PID_UNKNOWN_5,
      PID_UNKNOWN_6,
      PID_UNKNOWN_7,
      PID_UNKNOWN_8,
      PID_UNKNOWN_9,
      PID_UNKNOWN_10,
      PID_UNKNOWN_11,
      PID_UNKNOWN_12,
      PID_CELL_MIN_MAX,
      PID_UNKNOWN_14,
      PID_UNKNOWN_15,
      PID_UNKNOWN_16,
      PID_UNKNOWN_17,
      PID_UNKNOWN_18,
      PID_UNKNOWN_19,
      PID_UNKNOWN_20,
      PID_UNKNOWN_21,
      PID_UNKNOWN_22,
      PID_UNKNOWN_23,
      PID_UNKNOWN_24,
      PID_UNKNOWN_25,
      PID_UNKNOWN_26,
      PID_UNKNOWN_27,
      PID_UNKNOWN_28,
      PID_UNKNOWN_29,
      PID_UNKNOWN_30,
      PID_UNKNOWN_31,
      PID_UNKNOWN_32,
      PID_UNKNOWN_33,
      PID_UNKNOWN_34,
      PID_UNKNOWN_35,
      PID_UNKNOWN_36,
      PID_UNKNOWN_37,
      PID_UNKNOWN_38,
      PID_UNKNOWN_39,
      PID_UNKNOWN_40,
      PID_CELLVOLTAGES_1,
      PID_CELLVOLTAGES_2,
      PID_CELLVOLTAGES_3,
      PID_CELLVOLTAGES_4,
      PID_CELLVOLTAGES_5,
      PID_CELLVOLTAGES_6,
      PID_CELLVOLTAGES_7,
      PID_CELLVOLTAGES_8,
      PID_CELLTEMPERATURES_ALL,
      PID_UNKNOWN_162,
      PID_UNKNOWN_163,
      PID_UNKNOWN_164,
      PID_UNKNOWN_165,
      PID_UNKNOWN_166,
      PID_UNKNOWN_167,
      PID_UNKNOWN_168,
      PID_UNKNOWN_169,
      PID_UNKNOWN_170,
      PID_UNKNOWN_171,
      PID_UNKNOWN_172,
      PID_UNKNOWN_173,
      PID_UNKNOWN_174,
      PID_UNKNOWN_175,
      PID_UNKNOWN_176,
      PID_UNKNOWN_177,
      PID_UNKNOWN_178,
      PID_UNKNOWN_179,
      PID_UNKNOWN_180,
      PID_UNKNOWN_181,
      PID_UNKNOWN_182,
      PID_UNKNOWN_183,
      PID_UNKNOWN_184,
      PID_UNKNOWN_185,
      PID_UNKNOWN_186,
      PID_UNKNOWN_187,
      PID_UNKNOWN_188,
      PID_UNKNOWN_189,
      PID_UNKNOWN_190,
      PID_UNKNOWN_191,
      PID_UNKNOWN_192,
      PID_UNKNOWN_193,
      PID_UNKNOWN_194,
      PID_UNKNOWN_195,
      PID_UNKNOWN_196,
      PID_UNKNOWN_197,
      PID_UNKNOWN_198,
      PID_UNKNOWN_199,
      PID_UNKNOWN_200,
      PID_UNKNOWN_201,
      PID_UNKNOWN_202,
      PID_UNKNOWN_203,
      PID_UNKNOWN_204,
      PID_UNKNOWN_205,
      PID_UNKNOWN_206,
      PID_UNKNOWN_207,
      PID_UNKNOWN_208,
      PID_UNKNOWN_209,
      PID_UNKNOWN_210,
      PID_UNKNOWN_211,
      PID_UNKNOWN_212,
      PID_UNKNOWN_213,
      PID_VIN,
      PID_UNKNOWN_215,
      PID_UNKNOWN_216,
      PID_HW_VERSION_NUM,
      PID_UNKNOWN_218,
      PID_UNKNOWN_219,
      PID_SW_HOMOLOGATION_CODE,
      PID_UNKNOWN_221,
      PID_UNKNOWN_222,
      PID_UNKNOWN_223,
      PID_UNKNOWN_224,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
