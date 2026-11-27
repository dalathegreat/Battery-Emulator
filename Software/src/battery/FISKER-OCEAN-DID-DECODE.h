#ifndef FISKER_OCEAN_DID_DECODE_H
#define FISKER_OCEAN_DID_DECODE_H

#include <Arduino.h>

// Decoder for the Fisker Ocean BMS UDS 0x22 ReadDataByIdentifier responses.
// Scalings were recovered by decompiling the serializer callbacks in the 2024
// FRS390 21 NCM firmware and cross-checked against live responses and the
// broadcast-frame calibration. Header-only (inline) so both the public and the
// development builds can share one copy. Operates on the response data bytes
// that follow the "62 HH LL" echo. See FISKER_DID_DECODE_SPREADSHEET.csv.

namespace FiskerDidDecode {

inline uint16_t rd_u16(const uint8_t* d) {
  return (static_cast<uint16_t>(d[0]) << 8) | d[1];
}
inline uint32_t rd_u24(const uint8_t* d) {
  return (static_cast<uint32_t>(d[0]) << 16) | (static_cast<uint32_t>(d[1]) << 8) | d[2];
}
inline uint32_t rd_u32(const uint8_t* d) {
  return (static_cast<uint32_t>(d[0]) << 24) | (static_cast<uint32_t>(d[1]) << 16) |
         (static_cast<uint32_t>(d[2]) << 8) | d[3];
}
inline String hex_bytes(const uint8_t* p, uint8_t len) {
  String s;
  char b[4];
  for (uint8_t i = 0; i < len; i++) {
    snprintf(b, sizeof(b), "%02X", p[i]);
    if (i)
      s += ' ';
    s += b;
  }
  return s;
}
inline String ascii_bytes(const uint8_t* p, uint8_t len) {
  String s;
  for (uint8_t i = 0; i < len; i++)
    s += (p[i] >= 0x20 && p[i] < 0x7F) ? static_cast<char>(p[i]) : '.';
  return s;
}

// Human-readable name for each polled identifier (firmware signal names).
inline const char* name(uint16_t did) {
  switch (did) {
    case 0x2003: return "BatterySumVoltage";
    case 0x2004: return "BatteryCurrent";
    case 0x2005: return "BatteryCurrentValid";
    case 0x2008: return "DischargeCurrentLimitBranches";
    case 0x2009: return "RechargeCurrentLimitBranches";
    case 0x2011: return "ChargeOvercurrentLimitBranches";
    case 0x2016: return "HallSampleCurrentBranchA";
    case 0x2019: return "CSUSampleCurrentBranchA";
    case 0x2024: return "CSUCurrentStateBranchA";
    case 0x2026: return "InletWaterTemperature";
    case 0x2027: return "OutletWaterTemperature";
    case 0x2031: return "MaxBalanceCircuitTemperature";
    case 0x2032: return "BalanceTemperatureValidityVector";
    case 0x2033: return "MaxChipTemperature";
    case 0x2034: return "ChipTemperatureValidityVector";
    case 0x2038: return "AverageModuleTemperature";
    case 0x2039: return "MaxModuleTemperature";
    case 0x2040: return "MinModuleTemperature";
    case 0x2041: return "MaxModuleTemperaturePosition";
    case 0x2042: return "MinModuleTemperaturePosition";
    case 0x2043: return "ModuleTemperatureValidityVector";
    case 0x2047: return "MaxVoltageCellSOC";
    case 0x2048: return "MinVoltageCellSOC";
    case 0x2049: return "AverageCellSOC";
    case 0x2050: return "DisplayedPackSOC";
    case 0x2053: return "UnexpectedPowerDownFault";
    case 0x2054: return "ModuleTemperatureDaisyChainUpdated";
    case 0x2055: return "CellVoltageDaisyChainUpdated";
    case 0x2056: return "CMCResetErrorFlag";
    case 0x2057: return "VCUCrashMessageState";
    case 0x2058: return "HardwireCrashPWMPeriod";
    case 0x2059: return "HardwireCrashPWMDutyCycle";
    case 0x2060: return "ForceForbidIsolationDetectCommand";
    case 0x2061: return "IsolationMeasurementStatus";
    case 0x2062: return "IsolationMeasurementState";
    case 0x2063: return "PositiveIsolationMeasurementVoltageRaw";
    case 0x2064: return "NegativeIsolationMeasurementVoltageRaw";
    case 0x2069: return "PositiveIsolationResistance";
    case 0x2070: return "NegativeIsolationResistance";
    case 0x2078: return "BalancingCircuitOpenErrorCMCPosition";
    case 0x2079: return "BalancingCircuitOpenErrorCellPosition";
    case 0x2080: return "BalancingCircuitShortErrorCMCPosition";
    case 0x2081: return "BalancingCircuitShortErrorCellPosition";
    case 0x2089: return "Channel0HighVoltage";
    case 0x2090: return "Channel1HighVoltage";
    case 0x2091: return "Channel2HighVoltage";
    case 0x2092: return "Channel0LowVoltage";
    case 0x2093: return "Channel1LowVoltage";
    case 0x2094: return "Channel2LowVoltage";
    case 0x2107: return "BatteryToG0Voltage";
    case 0x2108: return "PVPositiveToG0Voltage";
    case 0x2109: return "MainPositiveToG0Voltage";
    case 0x2117: return "MainPositiveToG1Voltage";
    case 0x2130: return "KL30CVoltage";
    case 0x2133: return "MaxCellVoltagePosition";
    case 0x2134: return "MinCellVoltagePosition";
    case 0x2136: return "AverageCellVoltage";
    case 0x2137: return "MaxCellVoltage";
    case 0x2138: return "MinCellVoltage";
    case 0x2143: return "CellVoltageValidityVector";
    case 0x2144: return "PVPositiveContactorAging";
    case 0x2145: return "PVNegativeContactorAging";
    case 0xEFF6: return "Timestamp";
    case 0xEFF7: return "VehicleSpeed";
    case 0xEFF8: return "Mileage";
    case 0xEFF9: return "BatteryVoltage12VAux";
    case 0xEFFE: return "STMin";
    case 0xF040: return "PVIUControlRead";
    case 0xF055: return "ShippingModeRead";
    case 0xF060: return "ContactorControlRead";
    case 0xF184: return "ApplicationSoftwareFingerprint";
    case 0xF190: return "VIN";
    default: return "Unknown";
  }
}

// Decode the response data bytes to an engineering value with units.
inline String decode(uint16_t did, const uint8_t* p, uint8_t len) {
  if (len == 0)
    return "-";
  switch (did) {
    // Pack voltage: three u16 BE, each raw*0.1 V (first = pack sum).
    case 0x2003: {
      if (len < 2)
        break;
      String s = String(rd_u16(p) / 10.0f, 1) + " V";
      if (len >= 6)
        s += " / " + String(rd_u16(p + 2) / 10.0f, 1) + " / " + String(rd_u16(p + 4) / 10.0f, 1) + " V";
      return s;
    }
    // Pack current: u16 BE in bytes 2-3, current = (raw-20000)*0.05 A.
    case 0x2004: {
      if (len < 4)
        break;
      if (rd_u32(p) == 0xFFFFFFFFul)
        return "Invalid";
      return String((static_cast<int>(rd_u16(p + 2)) - 20000) * 0.05f, 1) + " A";
    }
    case 0x2005:
      return p[0] == 1 ? "Valid" : (p[0] == 0 ? "Invalid" : "-");
    // Current limit branches: three u16 BE, raw*0.1 A.
    case 0x2008:
    case 0x2009:
    case 0x2011:
      if (len >= 6)
        return String(rd_u16(p) / 10.0f, 1) + " / " + String(rd_u16(p + 2) / 10.0f, 1) + " / " +
               String(rd_u16(p + 4) / 10.0f, 1) + " A";
      break;
    // Low-level current samples: raw.
    case 0x2016:
    case 0x2019:
      if (len >= 4)
        return "raw " + String(rd_u32(p));
      break;
    case 0x2024:
      return "state " + String(p[0]);
    // Temperatures: u16 BE, raw*0.1 degC, 0xFFFF / 0xFF = invalid.
    case 0x2026:
    case 0x2027:
    case 0x2031:
    case 0x2033:
    case 0x2038:
    case 0x2039:
    case 0x2040: {
      if (len < 2)
        break;
      uint16_t r = rd_u16(p);
      if (r == 0xFFFF || p[0] == 0xFF)
        return "Invalid";
      return String(r * 0.1f, 1) + " &deg;C";
    }
    // Position/index byte groups and vectors: raw hex.
    case 0x2041:
    case 0x2042:
    case 0x2133:
    case 0x2134:
    case 0x2032:
    case 0x2034:
    case 0x2043:
    case 0x2143:
    case 0x2054:
    case 0x2055:
    case 0x2056:
    case 0xEFF6:
      return hex_bytes(p, len);
    // Cell SOC (3 fields) and pack SOC: field value / 10 = percent.
    case 0x2047:
    case 0x2048:
    case 0x2049:
      if (len >= 6)
        return String(rd_u16(p) / 10.0f, 1) + " / " + String(rd_u16(p + 2) / 10.0f, 1) + " / " +
               String(rd_u16(p + 4) / 10.0f, 1) + " %";
      if (len >= 2)
        return String(rd_u16(p) / 10.0f, 1) + " %";
      break;
    case 0x2050:
      if (len >= 2)
        return String(rd_u16(p) / 10.0f, 1) + " %";
      break;
    case 0x2053:
    case 0x2061:
    case 0x2062:
    case 0xF040:
    case 0xF055:
    case 0xF060:
    case 0xEFFE: {
      char b[8];
      snprintf(b, sizeof(b), "0x%02X", p[0]);
      return String(b);
    }
    case 0x2057:
      return p[0] == 1 ? "Set" : (p[0] == 0 ? "Clear" : "-");
    case 0x2058:
      if (len >= 4)
        return "raw " + String(rd_u32(p));
      break;
    case 0x2059:
      if (len >= 2)
        return String(rd_u16(p) / 100.0f, 2) + " %";
      break;
    case 0x2060:
      return p[0] ? "Forbidden" : "Allowed";
    // Isolation measurement raw ADC counts.
    case 0x2063:
    case 0x2064:
      if (len >= 2)
        return "raw " + String(rd_u16(p));
      break;
    // Isolation resistance in kOhm.
    case 0x2069:
    case 0x2070:
      if (len >= 2)
        return String(rd_u16(p)) + " k&Omega;";
      break;
    case 0x2078:
    case 0x2079:
    case 0x2080:
    case 0x2081:
      return p[0] == 0 ? "None" : ("pos " + String(p[0]));
    // Measurement-channel and cell voltages in mV.
    case 0x2089:
    case 0x2090:
    case 0x2091:
    case 0x2092:
    case 0x2093:
    case 0x2094:
    case 0x2136:
    case 0x2137:
    case 0x2138:
      if (len >= 2)
        return String(rd_u16(p)) + " mV";
      break;
    // HV isolation-node voltages: u16 BE, raw*0.1 V (candidate), 0xFF = invalid.
    case 0x2107:
    case 0x2108:
    case 0x2109:
    case 0x2117: {
      if (len < 2)
        break;
      uint16_t r = rd_u16(p);
      if (r == 0xFFFF || p[0] == 0xFF)
        return "Invalid";
      return String(r / 10.0f, 1) + " V";
    }
    case 0x2130:
      if (len >= 2) {
        uint16_t r = rd_u16(p);
        if (r == 0xFFFF)
          return "Invalid";
        return String(r / 1000.0f, 3) + " V";
      }
      break;
    // Contactor ageing counters (u24).
    case 0x2144:
    case 0x2145:
      if (len >= 3)
        return "count " + String(rd_u24(p));
      break;
    case 0xEFF7:
      if (len >= 2)
        return String(rd_u16(p) / 100.0f, 2) + " km/h";
      break;
    case 0xEFF8:
      if (len >= 4)
        return String(rd_u32(p)) + " km";
      break;
    case 0xEFF9:
      // 12 V aux battery: serializer outputs source/100 as whole volts.
      return String(p[len >= 2 ? 1 : 0]) + " V";
    case 0xF190:
    case 0xF184:
      return ascii_bytes(p, len);
    default:
      break;
  }
  return hex_bytes(p, len);  // fallback: raw hex
}

}  // namespace FiskerDidDecode

#endif
