#ifndef _VOLVO_SPA_HTML_H
#define _VOLVO_SPA_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class VolvoSpaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    tr_h4(content, TrKey::DRV_BECM_SUPPLY_VOLTAGE, String(datalayer_extended.VolvoPolestar.BECMsupplyVoltage), " mV");

    tr_h4(content, TrKey::DRV_DYNAMIC_MAX_VOLTAGE, String(datalayer_extended.VolvoPolestar.BECMUDynMaxLim), " V");
    tr_h4(content, TrKey::DRV_DYNAMIC_MIN_VOLTAGE, String(datalayer_extended.VolvoPolestar.BECMUDynMinLim), " V");

    tr_h4(content, TrKey::DRV_DISCHARGE_POWER_LIMIT_1, String(datalayer_extended.VolvoPolestar.HvBattPwrLimDcha1),
          " kW");
    tr_h4(content, TrKey::DRV_DISCHARGE_SOFT_POWER_LIMIT, String(datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSoft),
          " kW");
    tr_h4(content, TrKey::DRV_DISCHARGE_POWER_LIMIT_SLOW_AGING,
          String(datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSlowAgi), " kW");
    tr_h4(content, TrKey::DRV_CHARGE_POWER_LIMIT_SLOW_AGING,
          String(datalayer_extended.VolvoPolestar.HvBattPwrLimChrgSlowAgi), " kW");
    tr_h4_open(content, TrKey::DRV_HVIL_CIRCUIT_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x01) {
      case 0x01:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    tr_h4_open(content, TrKey::DRV_HVIL_CIRCUIT_B_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x02) {
      case 0x02:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_HVIL_CIRCUIT_C_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x04) {
      case 0x04:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_PRECHARGE_CONTACTOR_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x08) {
      case 0x08:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_POSITIVE_CONTACTOR_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x10) {
      case 0x10:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_NEGATIVE_CONTACTOR_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x20) {
      case 0x20:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_RELAY_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVSysRlySts) {
      case 0:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      case 1:
        content += String(TR(TrKey::DRV_CLOSED));
        break;
      case 2:
        content += String(TR(TrKey::DRV_KEEPSTATUS));
        break;
      case 3:
        content += String(TR(TrKey::DRV_OPENANDREQUESTACTIVEDISCHARGE));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_RELAY_STATUS_1);
    switch (datalayer_extended.VolvoPolestar.HVSysDCRlySts1) {
      case 0:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      case 1:
        content += String(TR(TrKey::DRV_CLOSED));
        break;
      case 2:
        content += String(TR(TrKey::DRV_KEEPSTATUS));
        break;
      case 3:
        content += String(TR(TrKey::DRV_FAULT));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_RELAY_STATUS_2);
    switch (datalayer_extended.VolvoPolestar.HVSysDCRlySts2) {
      case 0:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      case 1:
        content += String(TR(TrKey::DRV_CLOSED));
        break;
      case 2:
        content += String(TR(TrKey::DRV_KEEPSTATUS));
        break;
      case 3:
        content += String(TR(TrKey::DRV_FAULT));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HV_SYSTEM_ISOLATION_RESISTANCE_MONITORING_STATUS);
    switch (datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts) {
      case 0:
        content += String(TR(TrKey::DRV_NOT_VALID_1));
        break;
      case 1:
        content += String(TR(TrKey::DRV_FALSE));
        break;
      case 2:
        content += String(TR(TrKey::DRV_TRUE));
        break;
      case 3:
        content += String(TR(TrKey::DRV_NOT_VALID_2));
        break;
      default:
        content += String(TR(TrKey::DRV_NOT_VALID));
    }

    content += render_dtc_section(datalayer.battery.dtc);

    return content;
  }

 private:
  // The BMS reports standard 3-byte DTCs, but Volvo service data and volvo_SPA_dtc.json
  // all use the 5-character short form (B11D5, U1000) built from the first two bytes only. That is
  // therefore what goes into data-dtc-code for the JSON loader to match on. The third byte is the
  // failure type: it is appended for display when set ("B11D5-2F") so nothing is silently dropped,
  // but it stays out of the lookup key.
  static String render_dtc_section(DATALAYER_BATTERY_DTC_TYPE& dtc) {
    String content;
    content.reserve(3300 + dtc.dtc_count * 200);

    content +=
        "<h4 style='margin-top:20px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>&#128295; " +
        TR(TrKey::DRV_DIAGNOSTIC_TROUBLE_CODES) + "</h4>";

    if (dtc.dtc_last_read_millis == 0) {
      content += "<p style='color:#bbb;'>" + TR(TrKey::DRV_NOT_READ_YET_USE_READ_DTC_BUTTON_BELOW_SCAN) + "</p>";
      return content;
    }
    if (dtc.dtc_read_failed) {
      content += "<p style='color:#ff8a80;'>&#9888; " + TR(TrKey::DRV_LAST_DTC_READ_FAILED_TIMED_OUT) + "</p>";
      return content;
    }
    if (dtc.dtc_count == 0) {
      content += "<p style='color:#69f0ae;'>&#10003; " + TR(TrKey::DRV_NO_DTCS_PRESENT) + "</p>";
      return content;
    }

    unsigned long age_s = (millis() - dtc.dtc_last_read_millis) / 1000;
    content += "<p style='color:#bbb;'>" + String(dtc.dtc_count) + " " + TR(TrKey::DRV_CODES) + " &mdash; read " +
               String(age_s) + TR(TrKey::DRV_S_AGO) + "</p>";
    content += "<div style='overflow-x:auto;margin-bottom:12px;'>";
    content +=
        "<table style='margin:0 auto;text-align:left;border-collapse:separate;border-spacing:0;"
        "border:1px solid #4a5a64;border-radius:8px;overflow:hidden;'>";
    content +=
        "<thead><tr style='background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;'>"
        // "DTC" is the standard automotive acronym and is used verbatim in every
        // language the catalog carries, so it stays untranslated.
        "<th style='padding:10px 18px;text-align:left;'>DTC</th>"
        "<th style='padding:10px 18px;text-align:left;'>" +
        TR(TrKey::DRV_STATUS) + "</th><th style='padding:10px 18px;text-align:left;'>" + TR(TrKey::DRV_DESCRIPTION) +
        "</th></tr></thead><tbody>";

    const char SYS[5] = "PCBU";
    for (uint8_t i = 0; i < dtc.dtc_count; i++) {
      uint32_t code = dtc.dtc_codes[i];
      uint8_t status = dtc.dtc_status[i];

      char matchKey[6];  // Letter plus four hex digits
      snprintf(matchKey, sizeof(matchKey), "%c%02lX%02lX", SYS[(code >> 22) & 0x03],
               (unsigned long)((code >> 16) & 0x3F), (unsigned long)((code >> 8) & 0xFF));

      char shown[10];  // Key plus "-XX"
      uint8_t failure_type = code & 0xFF;
      if (failure_type) {
        snprintf(shown, sizeof(shown), "%s-%02lX", matchKey, (unsigned long)failure_type);
      } else {
        snprintf(shown, sizeof(shown), "%s", matchKey);
      }

      // Status precedence: Active (bit 0x01) > Confirmed (bit 0x08) > Stored.
      String statusStr = TR(TrKey::DRV_STORED);
      const char* statusColor = "#9e9e9e";
      if (status & 0x08) {
        statusStr = TR(TrKey::DRV_CONFIRMED);
        statusColor = "#d29922";
      }
      if (status & 0x01) {
        statusStr = TR(TrKey::DRV_ACTIVE_STATUS);
        statusColor = "#ff5252";
      }

      content +=
          "<tr><td style='padding:8px 18px;border-top:1px solid #3a4750;font-family:monospace;"
          "font-weight:600;'>";
      content += shown;
      content += "</td><td style='padding:8px 18px;border-top:1px solid #3a4750;color:";
      content += statusColor;
      content += ";font-weight:600;'>";
      content += statusStr;
      content += "</td><td data-dtc-code='";
      content += matchKey;
      content += "' style='padding:8px 18px;border-top:1px solid #3a4750;'>" + TR(TrKey::UI_UNKNOWN) + "</td></tr>";
    }
    content += "</tbody></table></div>";
    content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "volvo_SPA_dtc.json");

    return content;
  }
  DATALAYER_BATTERY_TYPE* battery_dl;
};

#endif
