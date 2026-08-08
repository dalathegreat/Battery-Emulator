#ifndef _FORD_MACH_E_BATTERY_HTML_H
#define _FORD_MACH_E_BATTERY_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class FordMachEHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    tr_h3(content, TrKey::DRV_FORD_MACH_E_EXTRA_INFORMATION);
    //If values are not sampled yet (255), show "N/A" instead of 255

    tr_h4_start(content, TrKey::DRV_POLLED_ALLOWED_CHARGE_POWER);
    if (datalayer_extended.fordMachE.pid_hvb_max_charge_current == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_max_charge_current) + " A</h4>";
    }

    tr_h4_start(content, TrKey::DRV_AVERAGE_TEMPERATURE);
    if (datalayer_extended.fordMachE.pid_hvb_temp == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_temp) + " °C</h4>";
    }

    tr_h4_start(content, TrKey::DRV_HIGH_PRECISION_VOLTAGE);
    if (datalayer_extended.fordMachE.pid_hvb_voltage == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_voltage / 100.0, 2) + " V</h4>";
    }

    tr_h4_start(content, TrKey::DRV_STATE_HEALTH);
    if (datalayer_extended.fordMachE.pid_hvb_soh == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_soh) + " %</h4>";
    }

    tr_h4_start(content, TrKey::DRV_STATE_CHARGE);
    if (datalayer_extended.fordMachE.pid_hvb_soc == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_soc / 1000.0, 3) + " %</h4>";
    }

    tr_h4_start(content, TrKey::DRV_CONTACTOR_STATUS);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 0xA00A8400) {
        tr_h4_end(content, TrKey::DRV_INTERLOCK_SEATED_OK);
      } else if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 0) {
        tr_h4_end(content, TrKey::DRV_INTERLOCK_NOT_EVALUATED_YET);
      } else if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 0x00000400) {
        tr_h4_end(content, TrKey::DRV_INTERLOCK_OPEN);
      } else {
        content += TR(TrKey::DRV_UNKNOWN_ENUMERATION) + " " +
                   String(datalayer_extended.fordMachE.pid_hvb_contactor_status) + "</h4>";
      }
    }

    tr_h4_start(content, TrKey::DRV_POS_CONTACTOR_LEAK_VOLTAGE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_leak_voltage == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_leak_voltage) + " mV</h4>";
    }

    tr_h4_start(content, TrKey::DRV_NEG_CONTACTOR_LEAK_VOLTAGE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_leak_voltage == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_leak_voltage) + " mV</h4>";
    }

    tr_h4_start(content, TrKey::DRV_POS_CONTACTOR_VOLTAGE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_voltage == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_voltage) + " mV</h4>";
    }

    tr_h4_start(content, TrKey::DRV_NEG_CONTACTOR_VOLTAGE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_voltage == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_voltage) + " mV</h4>";
    }

    tr_h4_start(content, TrKey::DRV_POS_CONTACTOR_BUS_LEAK_RESISTANCE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_bus_leak_resistance == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content +=
          " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_bus_leak_resistance) + " kOhm</h4>";
    }

    tr_h4_start(content, TrKey::DRV_NEG_CONTACTOR_BUS_LEAK_RESISTANCE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_bus_leak_resistance == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content +=
          " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_bus_leak_resistance) + " kOhm</h4>";
    }

    tr_h4_start(content, TrKey::DRV_OVERALL_CONTACTOR_LEAK_RESISTANCE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_overall_leak_resistance == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_overall_leak_resistance) + " kOhm</h4>";
    }

    tr_h4_start(content, TrKey::DRV_OPEN_CONTACTOR_LEAK_RESISTANCE);
    if (datalayer_extended.fordMachE.pid_hvb_contactor_open_leak_resistance == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_open_leak_resistance) + " kOhm</h4>";
    }

    tr_h4_start(content, TrKey::DRV_CAPACITY);
    if (datalayer_extended.fordMachE.pid_battery_capacity_ah == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_battery_capacity_ah / 10.0, 1) + " Ah</h4>";
    }

    tr_h4_start(content, TrKey::DRV_MAINTENANCE_REBALANCE_STATUS);
    if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x04) {
        content += " " + TR(TrKey::DRV_INITIALIZING) + "</h4>";
      } else if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x01) {
        content += " " + TR(TrKey::DRV_PROGRESS) + "</h4>";
      } else if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x02) {
        content += " " + TR(TrKey::DRV_SUCCESSFULLY) + "</h4>";
      } else if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x03) {
        content += " " + TR(TrKey::DRV_ABORTED_PACK_FAULT) + "</h4>";
      } else {
        content += " " + String(datalayer_extended.fordMachE.pid_maintenance_rebalance_status) + "</h4>";
      }
    }

    tr_h4_start(content, TrKey::DRV_CALENDAR_AGE);
    if (datalayer_extended.fordMachE.pid_hvb_calendar_age_months == 255) {
      tr_h4_end(content, TrKey::DRV_NOT_APPLICABLE);
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_calendar_age_months / 100.0, 0) + " " +
                 TR(TrKey::DRV_MONTHS) + "</h4>";
    }

    content += render_dtc_section(datalayer.battery.dtc);

    return content;
  }

 private:
  // The BMS reports standard 3-byte DTCs, but Ford service data and ford_machE_dtc.json
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
    content += "<p style='color:#bbb;'>" + String(dtc.dtc_count) + " codes &mdash; read " + String(age_s) +
               TR(TrKey::DRV_S_AGO) + "</p>";
    content += "<div style='overflow-x:auto;margin-bottom:12px;'>";
    content +=
        "<table style='margin:0 auto;text-align:left;border-collapse:separate;border-spacing:0;"
        "border:1px solid #4a5a64;border-radius:8px;overflow:hidden;'>";
    content +=
        "<thead><tr style='background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;'>"
        "<th style='padding:10px 18px;text-align:left;'>DTC</th>"
        "<th style='padding:10px 18px;text-align:left;'>Status</th>"
        "<th style='padding:10px 18px;text-align:left;'>" +
        TR(TrKey::DRV_DESCRIPTION) + "</th></tr></thead><tbody>";

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
      content += "' style='padding:8px 18px;border-top:1px solid #3a4750;'>Unknown</td></tr>";
    }
    content += "</tbody></table></div>";
    content += get_dtc_json_loader_html(GITHUB_RAW_BASE_URL, "ford_machE_dtc.json");

    return content;
  }
  DATALAYER_BATTERY_TYPE* battery_dl;
};

#endif
