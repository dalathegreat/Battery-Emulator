#include "faults_html.h"
#include "../../battery/Battery.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"

#include "../../battery/TESLA-ALERT-NAMES.h"

static int count_active(const bool* active, int n) {
  int c = 0;
  for (int i = 0; i < n; i++) {
    if (active[i]) {
      c++;
    }
  }
  return c;
}

// Appends one ECU's alert list to content, styled like the BE events page (.event-log / .event rows).
// Rendered directly into the single pre-reserved content String to keep peak heap usage low.
static void render_alert_table(String& content, const char* title, const char* canId, const bool* active,
                               const char* const* names, int n) {
  int ecu_active = count_active(active, n);

  content += "<div style=\"background-color:#303e47;padding:10px;margin-bottom:10px;border-radius:25px\">";
  content += "<h3 style='margin:6px 0'>";
  content += title;
  content += " <span style='color:#8fa5b0;font-weight:normal'>(";
  content += canId;
  content += ")</span> &mdash; ";
  if (ecu_active > 0) {
    content += "<span style='color:#ff5c5c'>";
    content += ecu_active;
    content += " active</span>";
  } else {
    content += "<span style='color:#04b34f'>no active faults</span>";
  }
  content += " / ";
  content += n;
  content += "</h3><div class='event-log'>";
  for (int i = 0; i < n; i++) {
    if (active[i]) {
      content += "<div class='event act' data-a=1>";
    } else {
      content += "<div class='event' data-a=0>";
    }
    content += names[i];
    content += "</div>";
  }
  content += "</div></div>";
}

String faults_processor(const String& var) {
  if (var == "X") {
    String content = "";
    content.reserve(26000);  // one allocation up front; full page is ~23 KB (kept small to fit ESP32 heap)

    // Styling mirrors the BE events page (dark log rows, rounded panels, standard button style).
    content +=
        "<style>body{background-color:#000;color:#fff}"
        ".event-log{display:flex;flex-direction:column}"
        ".event{border:1px solid #fff;padding:8px;text-align:left;word-break:break-word}"
        ".event[data-a]:nth-child(even){background-color:#455a64}"
        ".event[data-a]:nth-child(odd){background-color:#394b52}"
        ".event[data-a].act{background-color:#a6192e;color:#fff}"
        "button{background-color:#505E67;color:white;border:none;padding:10px 20px;margin:6px 6px 20px 0;"
        "cursor:pointer;border-radius:10px}button:hover{background-color:#3A4A52}</style>";

    bool isTesla = (user_selected_battery_type == BatteryType::TeslaModel3Y ||
                    user_selected_battery_type == BatteryType::TeslaModelSX);

    if (!isTesla) {
      content += "<div style=\"background-color:#303e47;padding:14px;margin-bottom:10px;border-radius:25px\">";
      content +=
          "<h4 style='color:#ff9900'>This page shows Tesla alert-matrix faults. The active battery is not a "
          "Tesla Model 3/Y or S/X, so no fault data is available.</h4></div>";
      content += "<button onclick=\"window.location.href='/'\">Back to main page</button>";
      return content;
    }

    // Pre-count so the summary banner can show the total before the (large) tables are appended.
    int total_active = count_active(datalayer_extended.tesla.BMS_alertMatrixActive, 100) +
                       count_active(datalayer_extended.tesla.PCS_alertMatrixActive, 94) +
                       count_active(datalayer_extended.tesla.CP_alertMatrixActive, 96);

    content += "<div style=\"background-color:#303e47;padding:12px;margin-bottom:10px;border-radius:25px\">";
    if (total_active > 0) {
      content += "<h3 style='margin:4px 0;color:#ff5c5c'>&#9888; ";
      content += total_active;
      content += " active fault(s) across BMS / PCS / CP</h3>";
    } else {
      content += "<h3 style='margin:4px 0;color:#04b34f'>&#10004; No active faults</h3>";
    }
    content += "<label style='cursor:pointer'><input type='checkbox' id='oa'> Show only active faults</label>";

    // Tables rendered directly into the single pre-reserved content String.
    render_alert_table(content, "BMS alert matrix", "0x320", datalayer_extended.tesla.BMS_alertMatrixActive,
                       TESLA_BMS_ALERT_NAMES, 100);
    render_alert_table(content, "PCS alert matrix", "0x3A4", datalayer_extended.tesla.PCS_alertMatrixActive,
                       TESLA_PCS_ALERT_NAMES, 94);
    render_alert_table(content, "CP alert matrix", "0x31E", datalayer_extended.tesla.CP_alertMatrixActive,
                       TESLA_CP_ALERT_NAMES, 96);

    content += "<button onclick=\"window.location.href='/'\">Back to main page</button>";
    content += "<button onclick='location.reload(true)'>Refresh</button>";

    // "Show only active" defaults to ON and is persisted in localStorage so it survives the 15 s auto-refresh
    // and manual reloads.
    content +=
        "<script>(function(){var K='beTeslaFaultsActiveOnly';var cb=document.getElementById('oa');"
        "var s=localStorage.getItem(K);cb.checked=(s===null)?true:(s==='1');"
        "function ap(){var o=cb.checked;document.querySelectorAll('.event[data-a]').forEach(function(r){"
        "r.style.display=(o&&r.getAttribute('data-a')!=='1')?'none':'';});}"
        "cb.addEventListener('change',function(){try{localStorage.setItem(K,cb.checked?'1':'0');}catch(e){}ap();});"
        "ap();setTimeout(function(){location.reload(true);},15000);})();</script>";

    return content;
  }
  return String();
}
