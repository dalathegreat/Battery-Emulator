#include "hardware_html.h"

#ifdef HW_UNIFIED_S3

#include <LittleFS.h>
#include <utility>
#include <vector>
#include "../../battery/BATTERIES.h"
#include "../../charger/CHARGERS.h"
#include "../../inverter/INVERTERS.h"
#include "../../shunt/Shunt.h"
#include "../hal/board_config.h"
#include "html_escape.h"
#include "index_html.h"

const char hardware_html[] = INDEX_HTML_HEADER COMMON_JAVASCRIPT "%X%" INDEX_HTML_FOOTER;

namespace {

// The running components and the interface each one is on, in the order the
// settings page lists them. Components on something the file did not configure
// (an ADC sensor, a battery with its own interface) are left out.
std::vector<std::pair<comm_interface, const char*>> interface_users() {
  std::vector<std::pair<comm_interface, const char*>> users;
  auto add = [&users](const char* description, const char* who) {
    comm_interface comm;
    if (interface_for_description(description, comm)) {
      users.push_back({comm, who});
    }
  };
  if (battery) {
    add(battery->interface_name(), battery2 ? "Battery 1" : "Battery");
  }
  if (battery2) {
    add(battery2->interface_name(), "Battery 2");
  }
  if (battery3) {
    add(battery3->interface_name(), "Battery 3");
  }
  if (inverter) {
    add(inverter->interface_name(), "Inverter");
  }
  if (shunt) {
    add(shunt->interface_name(), "Measurement");
  }
  if (charger) {
    add(getCANInterfaceName(charger->interface()), "Charger");
  }
  return users;
}

}  // namespace

String hardware_processor(const String& var) {
  if (var != "X") {
    return String();
  }

  const BoardConfig& cfg = board_config;
  String content;

  // The shared button look and navigation row come from /nav.css; the page only
  // sizes the one button of its own, in the upload form.
  content += INDEX_HTML_NAV_LINK "<style>body{background-color:black;color:white}";
  content += "form button{padding:6px 20px}";
  // box-sizing keeps the padding inside the card's own width rather than adding
  // to it, which is what pushes content past the rounded edge on a narrow screen.
  content +=
      ".card{background-color:#303E47;padding:14px 20px;margin-bottom:12px;border-radius:20px;text-align:left;"
      "box-sizing:border-box;overflow-wrap:break-word}";
  content += ".err{color:#ff6b6b}.warn{color:#ffd166}.off{color:#8a949b}.bad{color:#ff6b6b}";
  // The table takes the card's width and the cells carry their own padding, so
  // the columns give way to each other instead of the table growing past the card.
  content += "table{width:100%;border-collapse:collapse;margin:12px 0 0 0;text-align:left;table-layout:fixed}";
  content += "th,td{vertical-align:top;padding:4px 10px 4px 0}";
  // Fixed layout takes its column widths from the first row, so name them: the
  // status column is a fixed label, the port column gets a share, and pins take
  // whatever is left, since that is the column that actually runs long.
  content += "th:first-child,td:first-child{width:38%}";
  // Pins are the column that runs long; let it break mid-token rather than
  // widen. The status column is short and reads badly wrapped, so it does not.
  content += "td:nth-child(2){overflow-wrap:anywhere}";
  content += "th:last-child,td:last-child{padding-right:0;white-space:nowrap;width:5.5em}";
  content += "@media(max-width:520px){.card{padding:12px 14px}h3{font-size:1.15em;margin:0 0 2px}}";
  // stretch is the flex default, but say it: it is what makes the two cards
  // match heights instead of each ending where its own content does. Once they
  // wrap, each card is alone on its line and takes its natural height again.
  content += ".row{display:flex;flex-wrap:wrap;gap:12px;align-items:stretch;margin-bottom:12px}";
  content += ".row>.card{flex:1 1 320px;min-width:0;margin-bottom:0}</style>";

  content +=
      INDEX_HTML_NAV_START "<button onclick=\"location.href='/settings'\">Change settings</button>" INDEX_HTML_NAV_END;

  // Active configuration: every port the file describes, in file order, with the
  // ones it did not enable greyed out. Driven by what the parser recorded, so a
  // port type can never be missing from this table.
  content += "<div class='card'>";
  if (cfg.valid) {
    content += "<h3 style='margin-bottom:2px'>" + html_escape(cfg.name.c_str()) + "</h3>";
    if (!cfg.revision.empty()) {
      content += "<small>" + html_escape(cfg.revision.c_str()) + "</small><br>";
    }
    if (!cfg.notes.empty()) {
      content += "<small>" + html_escape(cfg.notes.c_str()) + "</small><br>";
    }
    const auto users = interface_users();
    content += "<table>";
    content += "<tr><th>Port</th><th>Pins</th><th></th></tr>";
    for (const auto& row : cfg.rows) {
      if (row.status == PortStatus::Inactive) {
        content += "<tr class='off'>";
      } else if (row.status == PortStatus::Invalid) {
        content += "<tr class='bad'>";
      } else {
        content += "<tr>";
      }
      content += "<td>";
      content += html_escape(row.name.c_str());
      content += "<br><small>";
      content += html_escape(row.type.c_str());
      if (row.has_interface && row.status == PortStatus::Active) {
        String used_by;
        for (const auto& user : users) {
          if (user.first == row.interface) {
            used_by += used_by.length() ? ", " : "";
            used_by += user.second;
          }
        }
        if (used_by.length()) {
          content += "<br>Used by " + used_by;
        }
      }
      content += "</small></td><td><small>";
      content += html_escape(row.pins.c_str());
      content += "</small></td><td>";
      content += name_for_port_status(row.status);
      content += "</td></tr>";
    }
    content += "</table>";
  } else {
    content += "<h3>No hardware configuration loaded</h3>";
    content +=
        "<p>The emulator is running with network and web interface only. Upload a board configuration below to "
        "enable the battery, inverter and contactor settings.</p>";
  }
  content += "</div>";

  // ── Validation findings ────────────────────────────────────────────────────
  if (!cfg.issues.empty()) {
    content += "<div class='card'><h3>Validation</h3><ul style='text-align:left'>";
    for (const auto& issue : cfg.issues) {
      content += "<li class='";
      content += (issue.level == ConfigIssueLevel::Error) ? "err" : "warn";
      content += "'>";
      content += (issue.level == ConfigIssueLevel::Error) ? "Error: " : "Warning: ";
      if (!issue.port.empty()) {
        content += html_escape(issue.port.c_str()) + " &mdash; ";
      }
      content += html_escape(issue.text.c_str());
      content += "</li>";
    }
    content += "</ul></div>";
  }

  // ── Upload ─────────────────────────────────────────────────────────────────
  // Built here, emitted last, and only when the partition holds something.
  String files;
  File dir = LittleFS.open("/");
  if (dir && dir.isDirectory()) {
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
      if (f.isDirectory()) {
        continue;
      }
      String fname = String(f.name());
      if (fname.startsWith("/")) {
        fname = fname.substring(1);
      }
      files += "<li><a style='color:#8ab4f8' href='/boardfile?name=" + fname + "'>" + html_escape(fname) +
               "</a> &mdash; " + String(f.size()) + " bytes</li>";
    }
  }

  content += "<div class='row'>";

  content += "<div class='card'><h3>Upload a configuration</h3>";
  content +=
      "<form method='POST' action='/hardware/upload' enctype='multipart/form-data'>"
      "<input type='file' name='config' accept='.json,application/json' required "
      "style='color:white'><br><br>"
      "<button type='submit'>Upload and reboot</button></form>";
  content += "</div>";

  if (files.length() > 0) {
    content += "<div class='card'><h3>Backup</h3><ul style='text-align:left;margin:0;padding-left:20px'>";
    content += files;
    content += "</ul></div>";
  }

  content += "</div>";

  return content;
}

#endif  // HW_UNIFIED_S3
