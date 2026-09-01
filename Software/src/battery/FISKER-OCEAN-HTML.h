#ifndef FISKER_OCEAN_HTML_H
#define FISKER_OCEAN_HTML_H

#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class FiskerOceanHtmlRenderer : public BatteryHtmlRenderer {
 public:
  explicit FiskerOceanHtmlRenderer(DATALAYER_INFO_FISKER_OCEAN* data) : fisker(data) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    content.reserve(12000);
    content += "<h4>State of charge: ";
    content += fisker->broadcast_soc_valid ? String(fisker->broadcast_soc_percent) + "%" : "Not available";
    content += "</h4>";

    content +=
        "<div style='overflow-x:auto'><table><thead><tr><th>PID</th><th>CAN response</th>"
        "</tr></thead><tbody>";
    for (uint8_t i = 0; i < DATALAYER_INFO_FISKER_OCEAN::DID_COUNT; i++) {
      const auto& result = fisker->did_results[i];
      add_did_row(content, result);
    }
    content += "</tbody></table></div>";
    content += BatteryHtmlRenderer::render_dtc_section_html(datalayer.battery.dtc, "fisker_ocean_dtc.json", true);
    return content;
  }

 private:
  DATALAYER_INFO_FISKER_OCEAN* fisker;

  static void add_did_row(String& content, const DATALAYER_INFO_FISKER_OCEAN::DID_RESULT& result) {
    char did[7];
    snprintf(did, sizeof(did), "0x%04X", result.did);
    content += "<tr><td style='text-align:left'><strong>" + String(did_name(result.did)) + "</strong><br><code>" +
               String(did) + "</code></td><td>";
    content += "<code>" + can_response(result) + "</code></td></tr>";
  }

  static String can_response(const DATALAYER_INFO_FISKER_OCEAN::DID_RESULT& result) {
    if (!result.valid)
      return "No response";
    String text = "0x7E9: 62 ";
    char bytes[4];
    snprintf(bytes, sizeof(bytes), "%02X ", result.did >> 8);
    text += bytes;
    snprintf(bytes, sizeof(bytes), "%02X", result.did & 0xFF);
    text += bytes;
    for (uint8_t i = 0; i < result.payload_length; i++) {
      snprintf(bytes, sizeof(bytes), " %02X", result.payload[i]);
      text += bytes;
    }
    return text;
  }

  static const char* did_name(uint16_t did) {
    switch (did) {
      case 0x2003:
        return "BatterySumVoltage";
      case 0x2004:
        return "BatteryCurrent";
      case 0x2005:
        return "BatteryCurrentValid";
      case 0x2008:
        return "DischrgCurrLimShortValueBranch";
      case 0x2009:
        return "RechrgCurrLimShortValueShortBranch";
      case 0x2011:
        return "ChrgOverCurrLimtBranch";
      case 0x2016:
        return "HALLSampleCurrentBranchA";
      case 0x2019:
        return "CSUSampleCurrentBranchA";
      case 0x2024:
        return "CSUCurrentStateBranchA";
      case 0x2026:
        return "InletWaterTem";
      case 0x2027:
        return "OutletWaterTem";
      case 0x2031:
        return "MaxBalanceCircuitTem";
      case 0x2032:
        return "SaLVMD_BalTempVld";
      case 0x2033:
        return "MaxChipTemp";
      case 0x2034:
        return "SaLVMD_Chip17823InsideTempVld";
      case 0x2038:
        return "AvgModuleTempDegC";
      case 0x2039:
        return "MaxModuleTempDegC";
      case 0x2040:
        return "MinModuleTempDegC";
      case 0x2041:
        return "MaxModuleTempCMCAndPointPstn";
      case 0x2042:
        return "MinModuleTempCMCAndPointPstn";
      case 0x2043:
        return "ModuleTempVld";
      case 0x2047:
        return "MaxVoltCellSOCPct";
      case 0x2048:
        return "MinVoltCellSOCPct";
      case 0x2049:
        return "AvgVoltCellSOCPct";
      case 0x2050:
        return "PackDispSOCPct";
      case 0x2053:
        return "UnExpectPowerDownFlt";
      case 0x2054:
        return "ModuleTempDaisyChainUpdated";
      case 0x2055:
        return "CellVoltDaisyChainUpdated";
      case 0x2056:
        return "CMCResetErrFlag";
      case 0x2057:
        return "VCU_CrashMsg_St";
      case 0x2058:
        return "HardwireSigPWMPeriod";
      case 0x2059:
        return "HardwirePWMDutyCycle";
      case 0x2060:
        return "ForceForbidIsoDetectCmd";
      case 0x2061:
        return "IsoMeasStatus";
      case 0x2062:
        return "IsoMeasState";
      case 0x2063:
        return "PosIsoMeasVoltRaw";
      case 0x2064:
        return "NegIsoMeasVoltRaw";
      case 0x2069:
        return "IsoMeasPosResKOhm";
      case 0x2070:
        return "IsoMeasNegResKOhm";
      case 0x2078:
        return "BalCircuitOpenErrCMCPstn";
      case 0x2079:
        return "BalCircuitOpenErrCellPstn";
      case 0x2080:
        return "BalCircuitShortErrCMCPstn";
      case 0x2081:
        return "BalCircuitShortErrCellPstn";
      case 0x2089:
        return "VoltOrCurrCh0HighVoltMv";
      case 0x2090:
        return "VoltOrCurrCh1HighVoltMv";
      case 0x2091:
        return "VoltOrCurrCh2HighVoltMv";
      case 0x2092:
        return "VoltOrCurrCh0LowVoltMv";
      case 0x2093:
        return "VoltOrCurrCh1LowVoltMv";
      case 0x2094:
        return "VoltOrCurrCh2LowVoltMv";
      case 0x2107:
        return "BatteryToG0Volt";
      case 0x2108:
        return "PVPosToG0Volt";
      case 0x2109:
        return "MainPosToG0Volt";
      case 0x2117:
        return "MainPosToG1Volt";
      case 0x2130:
        return "KL30CVoltage";
      case 0x2133:
        return "MaxCellVoltCMCAndPointPstn";
      case 0x2134:
        return "MinCellVoltCMCAndPointPstn";
      case 0x2136:
        return "AvgCellVolt";
      case 0x2137:
        return "MaxCellVoltMv";
      case 0x2138:
        return "MinCellVoltMv";
      case 0x2143:
        return "CellVoltVld";
      case 0x2144:
        return "PVPosContactorAging";
      case 0x2145:
        return "PVNegContactorAging";
      case 0xEFF6:
        return "TimeStamp";
      case 0xEFF7:
        return "VehicleSpeed";
      case 0xEFF8:
        return "Mileage";
      case 0xEFF9:
        return "BatteryVoltage";
      case 0xEFFE:
        return "STMin";
      case 0xF040:
        return "PVIUcontrol";
      case 0xF055:
        return "shippingmode";
      case 0xF060:
        return "Contactor_Control_Read";
      case 0xF184:
        return "ApplicationSoftwareFingerprint";
      case 0xF190:
        return "VehicleIdentificationNumber";
      default:
        return "Unknown";
    }
  }
};

#endif
