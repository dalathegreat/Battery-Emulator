#ifndef _VOLVO_SPA_HTML_H
#define _VOLVO_SPA_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class VolvoSpaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h4>BECM supply voltage: " + String(datalayer_extended.VolvoPolestar.BECMsupplyVoltage) + " mV</h4>";

    content += "<h4>Dynamic max voltage: " + String(datalayer_extended.VolvoPolestar.BECMUDynMaxLim) + " V</h4>";
    content += "<h4>Dynamic min voltage: " + String(datalayer_extended.VolvoPolestar.BECMUDynMinLim) + " V</h4>";

    content +=
        "<h4>Discharge power limit 1: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimDcha1) + " kW</h4>";
    content +=
        "<h4>Discharge soft power limit: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSoft) + " kW</h4>";
    content +=
        "<h4>Discharge power limit slow aging: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSlowAgi) +
        " kW</h4>";
    content +=
        "<h4>Charge power limit slow aging: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimChrgSlowAgi) +
        " kW</h4>";
    content += "<h4>HVIL Circuit A status: ";
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x01) {
      case 0x01:
        content += String("Open");
        break;
      default:
        content += String("Not valid");
    }
    content += "<h4>HVIL Circuit B status: ";
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x02) {
      case 0x02:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>HVIL Circuit C status: ";
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x04) {
      case 0x04:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Precharge contactor status: ";
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x08) {
      case 0x08:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Positive Contactor status: ";
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x10) {
      case 0x10:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>Negative Contactor status: ";
    switch (datalayer_extended.VolvoPolestar.HVILstatusBits & 0x20) {
      case 0x20:
        content += String("Open");
        break;
      default:
        content += String("Closed");
    }
    content += "<h4>HV system relay status: ";
    switch (datalayer_extended.VolvoPolestar.HVSysRlySts) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("OpenAndRequestActiveDischarge");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system relay status 1: ";
    switch (datalayer_extended.VolvoPolestar.HVSysDCRlySts1) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system relay status 2: ";
    switch (datalayer_extended.VolvoPolestar.HVSysDCRlySts2) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system isolation resistance monitoring status: ";
    switch (datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts) {
      case 0:
        content += String("Not valid 1");
        break;
      case 1:
        content += String("False");
        break;
      case 2:
        content += String("True");
        break;
      case 3:
        content += String("Not valid 2");
        break;
      default:
        content += String("Not valid");
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
        "<h4 style='margin-top:20px;color:#27b06c;border-bottom:2px solid #27b06c;padding-bottom:5px;'>&#128295; "
        "Diagnostic Trouble Codes</h4>";

    if (dtc.dtc_last_read_millis == 0) {
      content += "<p style='color:#bbb;'>Not read yet &mdash; use the Read DTC button below to scan.</p>";
      return content;
    }
    if (dtc.dtc_read_failed) {
      content += "<p style='color:#ff8a80;'>&#9888; Last DTC read failed or timed out.</p>";
      return content;
    }
    if (dtc.dtc_count == 0) {
      content += "<p style='color:#69f0ae;'>&#10003; No DTCs present.</p>";
      return content;
    }

    unsigned long age_s = (millis() - dtc.dtc_last_read_millis) / 1000;
    content += "<p style='color:#bbb;'>" + String(dtc.dtc_count) + " codes &mdash; read " + String(age_s) + "s ago</p>";
    content += "<div style='overflow-x:auto;margin-bottom:12px;'>";
    content +=
        "<table style='margin:0 auto;text-align:left;border-collapse:separate;border-spacing:0;"
        "border:1px solid #4a5a64;border-radius:8px;overflow:hidden;'>";
    content +=
        "<thead><tr style='background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;'>"
        "<th style='padding:10px 18px;text-align:left;'>DTC</th>"
        "<th style='padding:10px 18px;text-align:left;'>Status</th>"
        "<th style='padding:10px 18px;text-align:left;'>Description</th></tr></thead><tbody>";

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
      const char* statusStr = "Stored";
      const char* statusColor = "#9e9e9e";
      if (status & 0x08) {
        statusStr = "Confirmed";
        statusColor = "#d29922";
      }
      if (status & 0x01) {
        statusStr = "Active";
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
      content += "' style='padding:8px 18px;border-top:1px solid #3a4750;'>Unknown</td></tr>";
    }
    content += "</tbody></table></div>";
    content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "volvo_SPA_dtc.json");

    return content;
  }
  DATALAYER_BATTERY_TYPE* battery_dl;
};

#endif
