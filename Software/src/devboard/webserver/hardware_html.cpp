#include "hardware_html.h"

#ifdef HW_UNIFIED_S3

#include <LittleFS.h>
#include "../hal/board_config.h"
#include "html_escape.h"
#include "index_html.h"

const char hardware_html[] = INDEX_HTML_HEADER COMMON_JAVASCRIPT "%X%" INDEX_HTML_FOOTER;

String hardware_processor(const String& var) {
  if (var != "X") {
    return String();
  }

  const BoardConfig& cfg = board_config;
  String content;

  content += "<style>body{background-color:black;color:white}";
  content +=
      "button{background-color:#505E67;color:white;border:none;padding:10px 20px;margin-bottom:20px;"
      "cursor:pointer;border-radius:10px}";
  content += "button:hover{background-color:#3A4A52}";
  content += ".card{background-color:#303E47;padding:14px 20px;margin-bottom:12px;border-radius:20px;text-align:left}";
  content += ".err{color:#ff6b6b}.warn{color:#ffd166}.off{color:#8a949b}.bad{color:#ff6b6b}";
  content += "table{margin:0 auto;text-align:left}</style>";

  content += "<button onclick=\"window.location.href='/'\">Back to main page</button>";

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
    content += "<table>";
    content += "<tr><th style='padding-right:14px'>Port</th><th style='padding-right:14px'>Pins</th><th></th></tr>";
    for (const auto& row : cfg.rows) {
      if (row.status == PortStatus::Inactive) {
        content += "<tr class='off'>";
      } else if (row.status == PortStatus::Invalid) {
        content += "<tr class='bad'>";
      } else {
        content += "<tr>";
      }
      content += "<td style='padding:4px 14px 4px 0'>";
      content += html_escape(row.name.c_str());
      content += "<br><small>";
      content += html_escape(row.type.c_str());
      content += "</small></td><td style='padding:4px 14px 4px 0'><small>";
      content += html_escape(row.pins.c_str());
      content += "</small></td><td style='padding:4px 0'>";
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

  // ── Boot-time controller probe ─────────────────────────────────────────────
  if (cfg.probe_ran) {
    content += "<div class='card'>";
    content += "<h3>SPI CAN controller</h3>";
    content += "<p>Configured: <b>" + String(cfg.configured_controller()) + "</b><br>";
    content += "Detected at boot: <b>" + String(name_for_mcp_kind(cfg.probed)) + "</b></p>";
    if (cfg.probe_mismatch()) {
      content +=
          "<p class='err'>Mismatch. The controller fitted to this board is not the one the configuration "
          "describes, so this CAN interface will not work. Upload the configuration file for the other "
          "variant of this board.</p>";
    }
    content += "</div>";
  }

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
  content += "<div class='card'><h3>Files on the filesystem</h3>";
  File dir = LittleFS.open("/");
  bool any = false;
  if (dir && dir.isDirectory()) {
    content += "<ul style='text-align:left'>";
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
      if (f.isDirectory()) {
        continue;
      }
      String fname = String(f.name());
      if (fname.startsWith("/")) {
        fname = fname.substring(1);
      }
      content += "<li><a style='color:#8ab4f8' href='/hardware/file?name=" + fname + "'>" + html_escape(fname) +
                 "</a> &mdash; " + String(f.size()) + " bytes</li>";
      any = true;
    }
    content += "</ul>";
  }
  if (!any) {
    content += "<p>Nothing stored yet.</p>";
  }
  content += "</div>";

  content += "<div class='card'><h3>Upload a configuration</h3>";
  content +=
      "<form method='POST' action='/hardware/upload' enctype='multipart/form-data'>"
      "<input type='file' name='config' accept='.json,application/json' required "
      "style='color:white'><br><br>"
      "<button type='submit'>Upload and reboot</button></form>";
  content += "</div>";

  return content;
}

#endif  // HW_UNIFIED_S3
