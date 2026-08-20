#ifndef _NISSAN_LEAF_HTML_H
#define _NISSAN_LEAF_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
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

    tr_h4_open(content, TrKey::DRV_LEAF_GENERATION);
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
        content += String(TR(TrKey::UI_UNKNOWN) + "</h4>");
    }
    char readableSerialNumber[16];  // One extra space for null terminator
    memcpy(readableSerialNumber, nissan_dl->BatterySerialNumber, sizeof(nissan_dl->BatterySerialNumber));
    readableSerialNumber[15] = '\0';  // Null terminate the string
    tr_h4(content, TrKey::DRV_SERIAL_NUMBER, String(readableSerialNumber));
    char readablePartNumber[8];  // One extra space for null terminator
    memcpy(readablePartNumber, nissan_dl->BatteryPartNumber, sizeof(nissan_dl->BatteryPartNumber));
    readablePartNumber[7] = '\0';  // Null terminate the string
    tr_h4(content, TrKey::DRV_PART_NUMBER, String(readablePartNumber));
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
    tr_h4(content, TrKey::DRV_REGEN_KW, String(nissan_dl->ChargePowerLimit));
    tr_h4(content, TrKey::DRV_CHARGE_KW, String(nissan_dl->MaxPowerForCharger));
    tr_h4(content, TrKey::DRV_TEMPERATURE_1, String(nissan_dl->temperature1 / 10.0), " &deg;C");
    tr_h4(content, TrKey::DRV_TEMPERATURE_2, String(nissan_dl->temperature2 / 10.0), " &deg;C");
    if (nissan_dl->LEAF_gen == 0) {
      tr_h4(content, TrKey::DRV_TEMPERATURE_3, String(nissan_dl->temperature3 / 10.0), " &deg;C");
    }
    tr_h4(content, TrKey::DRV_TEMPERATURE_4, String(nissan_dl->temperature4 / 10.0), " &deg;C");
    tr_h4(content, TrKey::DRV_INSULATION, String(nissan_dl->Insulation), " kΩ");
    tr_h4(content, TrKey::DRV_FULLY_CHARGED, String(nissan_dl->Full));
    tr_h4(content, TrKey::DRV_BATTERY_EMPTY, String(nissan_dl->Empty));
    tr_h4(content, TrKey::DRV_FAILSAFE_STATUS, String(nissan_dl->FailsafeStatus));
    tr_h4_colon(content, TrKey::DRV_INTERLOCK, String(nissan_dl->Interlock));
    tr_h4(content, TrKey::DRV_MAIN_RELAY, String(nissan_dl->MainRelayOn));
    tr_h4(content, TrKey::DRV_RELAY_CUT_REQUEST, String(nissan_dl->RelayCutRequest));
    tr_h4(content, TrKey::DRV_HEATER_PRESENT, String(nissan_dl->HeatExist));
    tr_h4(content, TrKey::DRV_HEATING_REQUESTED, String(nissan_dl->HeaterSendRequest));
    tr_h4(content, TrKey::DRV_HEATING_STARTED, String(nissan_dl->HeatingStart));
    tr_h4(content, TrKey::DRV_HEATING_STOPPED, String(nissan_dl->HeatingStop));
    //Both challenge values only ever get filled by the Reset degradation data sequence. Until that
    //has run, incomingChallenge still holds its 0xFFFFFFFF default and the solved halves are zero,
    //so say so rather than printing placeholder numbers that look like readings.
    tr_h4(content, TrKey::DRV_CRYPTOCHALLENGE,
          nissan_dl->CryptoChallenge != 0xFFFFFFFF ? String(nissan_dl->CryptoChallenge) : String("Not run"));
    tr_h4(content, TrKey::DRV_SOLVEDCHALLENGE,
          (nissan_dl->SolvedChallengeMSB || nissan_dl->SolvedChallengeLSB)
              ? String(nissan_dl->SolvedChallengeMSB) + "-" + String(nissan_dl->SolvedChallengeLSB)
              : String("Not run"));
    tr_h4(content, TrKey::DRV_CHALLENGE_FAILED, String(nissan_dl->challengeFailed));

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
    content += "<p style='color:#bbb;'>" + String(dtc.dtc_count) + " " + TR(TrKey::DRV_CODES);
    if (dtc.dtc_reported_count > dtc.dtc_count) {
      // The battery had more to say than there are slots to hold it. Say so, rather than presenting
      // a truncated list as if it were the whole story.
      content += " " + TR(TrKey::DRV_SHOWN_OF) + " " + String(dtc.dtc_reported_count) + " " + TR(TrKey::DRV_REPORTED);
    }
    content += " &mdash; read " + String(age_s) + TR(TrKey::DRV_S_AGO) + "</p>";
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
    content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "nissan_leaf_dtc.json");

    return content;
  }

  DATALAYER_BATTERY_TYPE* battery_dl;
  DATALAYER_INFO_NISSAN_LEAF* nissan_dl;
};

#endif
