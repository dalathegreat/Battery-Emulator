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

    //Styles shared by the two-column lists and the usage charts: rows that wrap, two 300 px columns
    //side by side and one on a narrow screen, like the chart grid.
    // MINIFIED to save flash. Edit the readable source here, re-minify, and replace the literal.
    /*
    <style>
      .hg, .hb, .ha { display: flex }
      .hg { flex-wrap: wrap }                            two columns, one on a narrow screen
      .hg>* { flex: 300px; margin: 5px }                 a list entry or a chart, each
      .hb { align-items: flex-end; height: 130px; padding-top: 20px;
            border: 1px solid #ccc }                    bar area, as the cell monitor graph
      .hb div { flex: 1; background: blue; border: 1px solid #fff; position: relative }
      .hb b { position: absolute; bottom: 100%; left: 0; right: 0 }   count above its bar
      .ha span { flex: 1 }                               bin labels under the bars
      .hb, .ha, .hg p { font-size: 12px }
      .hg p { margin: 0; text-align: right }             total under each chart
      h3 { margin-top: 3px }                             panel titles, the only h3s on the page: as
                                                         far below the top as content ends above the
                                                         bottom, not the browser's ~19 px
    </style>
    */
    content +=
        "<style>.hg,.hb,.ha{display:flex}.hg{flex-wrap:wrap}.hg>*{flex:300px;margin:5px}.hb{align-items:flex-end;"
        "height:130px;padding-top:20px;border:1px solid #ccc}.hb div{flex:1;background:blue;border:1px solid "
        "#fff;position:relative}.hb b{position:absolute;bottom:100%;left:0;right:0}.ha span{flex:1}.hb,.ha,.hg "
        "p{font-size:12px}.hg p{margin:0;text-align:right}h3{margin-top:3px}</style>";

    //Identity, in the page's own first panel under the battery heading
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

    new_panel(content, "Status");
    content += "<div class=hg>";
    content += "<h4>+12V BAT level: " +
               (nissan_dl->VBAT_mV ? String(nissan_dl->VBAT_mV / 1000.0f, 2) + " V" : String("Unknown")) + "</h4>";
    content += "<h4>Regen kW: " + String(nissan_dl->ChargePowerLimit) + "</h4>";
    content += "<h4>Charge kW: " + String(nissan_dl->MaxPowerForCharger) + "</h4>";
    content += "<h4>GIDS: " + String(nissan_dl->GIDS) + "</h4>";
    content += "<h4>Temperature 1: " + String(nissan_dl->temperature1 / 10.0) + " &deg;C</h4>";
    content += "<h4>Temperature 2: " + String(nissan_dl->temperature2 / 10.0) + " &deg;C</h4>";
    if (nissan_dl->LEAF_gen == 0) {
      content += "<h4>Temperature 3: " + String(nissan_dl->temperature3 / 10.0) + " &deg;C</h4>";
    }
    content += "<h4>Temperature 4: " + String(nissan_dl->temperature4 / 10.0) + " &deg;C</h4>";
    content += "<h4>Insulation: " + String(nissan_dl->Insulation) + " kΩ</h4>";
    //The flags from the LBC's status broadcasts, each unknown until the broadcast carrying it has
    //arrived since boot (bit 0 0x1DB, bit 1 0x55B, bit 2 0x5C0, see StatusSeen)
    const uint8_t seen = nissan_dl->StatusSeen;
    status_row(content, "Fully charged", nissan_dl->Full, seen & 0x01);
    status_row(content, "Battery empty", nissan_dl->Empty, seen & 0x02);
    status_row(content, "Failsafe status", nissan_dl->FailsafeStatus, seen & 0x01, true);  //0-7
    status_row(content, "Interlock", nissan_dl->Interlock, seen & 0x01);
    status_row(content, "Main relay ON", nissan_dl->MainRelayOn, seen & 0x01);
    status_row(content, "Relay cut request", nissan_dl->RelayCutRequest, seen & 0x01, true);  //0-3
    status_row(content, "Heater present", nissan_dl->HeatExist, seen & 0x04);
    status_row(content, "Heating requested", nissan_dl->HeaterSendRequest, seen & 0x04);
    status_row(content, "Heating started", nissan_dl->HeatingStart, seen & 0x04);
    status_row(content, "Heating stopped", nissan_dl->HeatingStop, seen & 0x04);
    content += "</div>";

    new_panel(content, "Health and lifetime usage");
    content += "<div class=hg>";
    //What the pack held when new, from the GID count the LBC reports at full charge. Constant per
    //pack size rather than something that tracks wear, which is what makes it the reference the
    //measured capacity below is judged against.
    content +=
        "<h4>Capacity as new: " +
        (nissan_dl->CapacityAsNewWh ? String(nissan_dl->CapacityAsNewWh / 1000.0f, 2) + " kWh" : String("Unknown")) +
        "</h4>";
    //Pack capacity as the LBC measures it, with the energy that works out to at the pack's nominal
    //voltage alongside it. The nominal differs by generation (96 cells at 3.75 V on ZE0/AZE0,
    //3.65 V on ZE1), so this is a nameplate-style figure and deliberately not derived from the live
    //pack voltage, which would make it swing with SoC. The bracketed energy is the total capacity
    //the rest of the system works from, and the ratio of it to the line above is the reported SOH.
    if (nissan_dl->CapacityCAh) {
      content += "<h4>Actual capacity: " + String(nissan_dl->CapacityCAh / 100.0f, 2) + " Ah (" +
                 String(nissan_dl->CapacityWh / 1000.0f, 2) + " kWh)</h4>";
    } else {
      content += String("<h4>Actual capacity: Unknown</h4>");
    }
    //The two state of health figures the LBC publishes for itself: the unfiltered one, and the
    //filtered figure it settles onto, which is the value the pack reports as its SOH. The raw one
    //moves first while a pack relearns after a degradation reset, so seeing the pair side by side
    //shows that relearning happening. Neither is the SOH shown on the status page - both are
    //erased by a degradation reset, so that one is derived from the capacities above instead.
    if (nissan_dl->battery_SOHraw_pptt) {
      content += "<h4>SOH raw: " + String(nissan_dl->battery_SOHraw_pptt / 100.0f, 2) + "% (avg " +
                 (nissan_dl->battery_SOHavg_pptt ? String(nissan_dl->battery_SOHavg_pptt / 100.0f, 2) + "%"
                                                 : String("Unknown")) +
                 ")</h4>";
    } else {
      content += String("<h4>SOH raw: Unknown</h4>");
    }
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
    //The usage tables from the rest of the same reply. They start at [0] on ZE0/AZE0 and at [2] on
    //ZE1, whose extra counter ahead of them moves them along by one count.
    const uint8_t* history = nissan_dl->UsageHistograms + ((nissan_dl->LEAF_gen == 2) ? 2 : 0);
    //The last table is not a plain histogram: its top bin counts charges to 100 % and the one below
    //it the times the pack was run down to turtle. Provisional, per the LBC history guide, and on
    //ZE1 read from the matching table by its parallel with ZE0.
    if (nissan_dl->ChargeCountL1L2) {
      content += "<h4>Charge to full count: ";
      content += (history[110] << 8) | history[111];
      content += "</h4><h4>Turtle count: ";
      content += (history[108] << 8) | history[109];
      content += "</h4>";
    }
    content += "</div>";

    //Lifetime usage histograms, the first six of those tables. They are drawn once the charge
    //counts are known; the page carries only the 48 counts, and the browser draws the six charts
    //from them in the cell monitor's bar style.
    if (nissan_dl->ChargeCountL1L2) {
      // The script below is MINIFIED to save flash. Edit the readable source here,
      // re-minify, and replace the literal.
      /*
      <script>
      (d => {  // 48 counts, 8 per table, in the LBC's table order (see UsageHistograms)
        let h = '<div class=hg>';
        // Peak temperatures first, then start temperatures, then SOC
        [2, 3, 0, 1, 4, 5].map((t, k) => {
          // Upper edge of bin j = 1..7: 35..65 degC in steps of 5, or SOC 20..70 % in steps of 10, then 85 %
          let f = j => k < 4 ? 30 + 5 * j : j < 7 ? 10 * j + 10 : 85,
              v = d.slice(t * 8, t * 8 + 8), m = Math.max(...v) || 1, n = 0, b = '', x = '';
          v.map((c, i) => {
            n += c;
            b += `<div style=height:${c * 100 / m}%><b>${c || ''}</b></div>`;
            // The top bin carries the unit: 65+ degC or 85+ %, kept on one line by the no-break space
            x += `<span>${i ? f(i) + (i < 7 ? '-' + f(i + 1) : '+&nbsp;' + (k < 4 ? '&deg;C' : '%'))
                            : '&lt;' + f(1)}</span>`;
          });
          h += `<div><h4>${k & 1 ? 'Charge' : 'Drive'}` +
               `${k < 4 ? ` temperature (${k < 2 ? 'peak' : 'start'})` : ' start SOC'}</h4>` +
               `<div class=hb>${b}</div><div class=ha>${x}</div><p>n = ${n}</p></div>`;
        });
        // Replace this script with the charts, so each battery's copy stays in its own section
        document.currentScript.outerHTML = h + '</div>';
      })([...48 counts...]);
      </script>
      */
      content +=
          "<script>(d=>{let h='<div class=hg>';[2,3,0,1,4,5].map((t,k)=>{let "
          "f=j=>k<4?30+5*j:j<7?10*j+10:85,v=d.slice(t*8,t*8+8),m=Math.max(...v)||1,n=0,b='',x='';v.map((c,i)=>{n+=c;b+="
          "`<div "
          "style=height:${c*100/m}%><b>${c||''}</b></"
          "div>`;x+=`<span>${i?f(i)+(i<7?'-'+f(i+1):'+&nbsp;'+(k<4?'&deg;C':'%')):'&lt;'+f(1)}</"
          "span>`});h+=`<div><h4>${k&1?'Charge':'Drive'}${k<4?` temperature (${k<2?'peak':'start'})`:' start "
          "SOC'}</h4><div class=hb>${b}</div><div class=ha>${x}</div><p>n = "
          "${n}</p></div>`});document.currentScript.outerHTML=h+'</div>'})([";
      for (uint8_t i = 0; i < 96; i += 2) {
        content += (history[i] << 8) | history[i + 1];
        content += ",";
      }
      content += "])</script>";
    }

    return content;
  }

  //The trouble codes, in a panel of their own after the status ones. The Read and Erase DTC
  //buttons the page adds next land in the same panel.
  String get_dtc_html() { return battery_dl ? render_dtc_section(battery_dl->dtc) : String(); }

  //The degradation reset gets a panel of its own: the challenge values its sequence fills in, above
  //the button that starts it.
  String get_command_prefix_html(const char* identifier) {
    String content;
    if (nissan_dl && strcmp(identifier, "resetSOH") == 0) {
      new_panel(content, "Reset degradation data");
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
    }
    return content;
  }

 private:
  //One status row, Unknown until the broadcast carrying it has arrived. A true/false flag shows as
  //a tick or a cross; a wider field, like failsafe status (3 bits) or relay cut request (2 bits),
  //shows as the number itself.
  static void status_row(String& content, const char* label, uint8_t value, bool known, bool as_number = false) {
    content += "<h4>";
    content += label;
    content += ": ";
    if (!known) {
      content += "Unknown";
    } else if (as_number) {
      content += (int)value;
    } else {
      content += value ? "&#10003;" : "&#10007;";
    }
    content += "</h4>";
  }

  //The page streams all of this inside a battery-panel div of its own and closes the last one after
  //the command buttons. Closing the current panel and opening the next is all it takes to split the
  //Leaf's information into several, each under a title of its own.
  static void new_panel(String& content, const char* title) {
    content += "</div><div class='battery-panel'><h3>";
    content += title;
    content += "</h3>";
  }

  // The LBC reports standard 3-byte DTCs, but Nissan service data, LeafSpy and nissan_leaf_dtc.json
  // all use the 5-character short form (P33D7, U1000) built from the first two bytes only. That is
  // therefore what goes into data-dtc-code for the JSON loader to match on. The third byte is the
  // failure type: it is appended for display when set ("P33D7-2F") so nothing is silently dropped,
  // but it stays out of the lookup key.
  static String render_dtc_section(DATALAYER_BATTERY_DTC_TYPE& dtc) {
    String content;
    content.reserve(3300 + dtc.dtc_count * 200);

    new_panel(content, "Diagnostic Trouble Codes");

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
