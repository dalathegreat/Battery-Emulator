#ifndef _NISSAN_LEAF_HTML_H
#define _NISSAN_LEAF_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class NissanLeafHtmlRenderer : public BatteryHtmlRenderer {
 public:
  NissanLeafHtmlRenderer(DATALAYER_BATTERY_TYPE* battery_dl, DATALAYER_INFO_NISSAN_LEAF* dl)
      : battery_dl(battery_dl), nissan_dl(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    if (!nissan_dl) {
      return content;
    }

    content += "<h4>LEAF generation: ";
    switch (nissan_dl->LEAF_gen) {
      case 0:
        content += String("ZE0</h4>");
        break;
      case 1:
        content += String("AZE0</h4>");
        break;
      case 2:
        content += String("ZE1</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    char readableSerialNumber[16];  // One extra space for null terminator
    memcpy(readableSerialNumber, nissan_dl->BatterySerialNumber, sizeof(nissan_dl->BatterySerialNumber));
    readableSerialNumber[15] = '\0';  // Null terminate the string
    content += "<h4>Serial number: " + String(readableSerialNumber) + "</h4>";
    char readableFirmware[6];  // One extra space for null terminator
    memcpy(readableFirmware, nissan_dl->BatteryPartNumber, 5);
    readableFirmware[5] = '\0';  // Null terminate the string
    content += "<h4>Firmware: " + String(readableFirmware) + "</h4>";
    content += "<h4>GIDS: " + String(nissan_dl->GIDS) + "</h4>";
    content +=
        "<h4>Hx: " +
        (nissan_dl->battery_HX_pptt ? String(nissan_dl->battery_HX_pptt / 100.0f, 2) + " %" : String("Unknown")) +
        "</h4>";
    //A used pack always has AC charges on it, so a zero L1/L2 count means the group was not read yet.
    content +=
        "<h4>QC charge count: " + (nissan_dl->ChargeCountL1L2 ? String(nissan_dl->ChargeCountQC) : String("Unknown")) +
        "</h4>";
    content += "<h4>AC charge count: " +
               (nissan_dl->ChargeCountL1L2 ? String(nissan_dl->ChargeCountL1L2) : String("Unknown")) + "</h4>";
    content += "<h4>Regen kW: " + String(nissan_dl->ChargePowerLimit) + "</h4>";
    content += "<h4>Charge kW: " + String(nissan_dl->MaxPowerForCharger) + "</h4>";
    content += "<h4>Temperature 1: " + String(nissan_dl->temperature1 / 10.0) + " &deg;C</h4>";
    content += "<h4>Temperature 2: " + String(nissan_dl->temperature2 / 10.0) + " &deg;C</h4>";
    if (nissan_dl->LEAF_gen == 0) {
      content += "<h4>Temperature 3: " + String(nissan_dl->temperature3 / 10.0) + " &deg;C</h4>";
    }
    content += "<h4>Temperature 4: " + String(nissan_dl->temperature4 / 10.0) + " &deg;C</h4>";
    content += "<h4>Insulation: " + String(nissan_dl->Insulation) + " kΩ</h4>";
    content += "<h4>Fully charged: " + String(nissan_dl->Full) + "</h4>";
    content += "<h4>Battery empty: " + String(nissan_dl->Empty) + "</h4>";
    content += "<h4>Failsafe status: " + String(nissan_dl->FailsafeStatus) + "</h4>";
    content += "<h4>Interlock: " + String(nissan_dl->Interlock) + "</h4>";
    content += "<h4>Main relay ON: " + String(nissan_dl->MainRelayOn) + "</h4>";
    content += "<h4>Relay cut request: " + String(nissan_dl->RelayCutRequest) + "</h4>";
    content += "<h4>Heater present: " + String(nissan_dl->HeatExist) + "</h4>";
    content += "<h4>Heating requested: " + String(nissan_dl->HeaterSendRequest) + "</h4>";
    content += "<h4>Heating started: " + String(nissan_dl->HeatingStart) + "</h4>";
    content += "<h4>Heating stopped: " + String(nissan_dl->HeatingStop) + "</h4>";
    //Both challenge values only ever get filled by the Reset degradation data sequence. Until that
    //has run, incomingChallenge still holds its 0xFFFFFFFF default and the solved halves are zero,
    //so say so rather than printing placeholder numbers that look like readings.
    content += "<h4>CryptoChallenge: " +
               (nissan_dl->CryptoChallenge != 0xFFFFFFFF ? String(nissan_dl->CryptoChallenge) : String("Not run")) +
               "</h4>";
    content += "<h4>SolvedChallenge: " +
               ((nissan_dl->SolvedChallengeMSB || nissan_dl->SolvedChallengeLSB)
                    ? String(nissan_dl->SolvedChallengeMSB) + "-" + String(nissan_dl->SolvedChallengeLSB)
                    : String("Not run")) +
               "</h4>";
    content += "<h4>Challenge failed: " + String(nissan_dl->challengeFailed) + "</h4>";

    if (battery_dl) {
      content += render_dtc_section(battery_dl->dtc);
    }

    return content;
  }

 private:
  // The LBC reports standard 3-byte DTCs, but Nissan service data, LeafSpy and nissan_leaf_dtc.json
  // all use the 5-character short form (P33D7, U1000) built from the first two bytes only. That is
  // therefore what goes into data-dtc-code for the JSON loader to match on. The third byte is the
  // failure type: it is appended for display when set ("P33D7-2F") so nothing is silently dropped,
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
    content += "<p style='color:#bbb;'>" + String(dtc.dtc_count) + " codes";
    if (dtc.dtc_reported_count > dtc.dtc_count) {
      // The battery had more to say than there are slots to hold it. Say so, rather than presenting
      // a truncated list as if it were the whole story.
      content += " shown of " + String(dtc.dtc_reported_count) + " reported";
    }
    content += " &mdash; read " + String(age_s) + "s ago</p>";
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
    content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "nissan_leaf_dtc.json");

    return content;
  }

  DATALAYER_BATTERY_TYPE* battery_dl;
  DATALAYER_INFO_NISSAN_LEAF* nissan_dl;
};

#endif
