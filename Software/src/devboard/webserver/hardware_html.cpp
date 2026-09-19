#include "hardware_html.h"

#ifdef HW_UNIFIED_S3

#include "../../datalayer/datalayer.h"
#include "../hal/board_config.h"
#include "html_escape.h"
#include "index_html.h"

static String pin_cell(gpio_num_t pin) {
  return pin == GPIO_NUM_NC ? String("&mdash;") : ("GPIO" + String((int)pin));
}

// One row per configured peripheral. Only ports the file actually enabled show
// up: a row here means a pin is claimed, which is the question a user reading
// this page is trying to answer.
static void add_row(String& out, const char* label, const String& pins) {
  out += "<tr><td style='padding:4px 12px 4px 0'>";
  out += label;
  out += "</td><td style='padding:4px 0;font-family:monospace'>";
  out += pins;
  out += "</td></tr>";
}

static String mcp_pins(const McpPorts& m, bool is_2515) {
  String s;
  if (m.sck != GPIO_NUM_NC) {
    s += "sck " + pin_cell(m.sck) + ", " + (is_2515 ? "mosi " : "sdi ") + pin_cell(m.sdi) + ", " +
         (is_2515 ? "miso " : "sdo ") + pin_cell(m.sdo) + ", ";
  } else {
    s += "shared bus, ";
  }
  s += "cs " + pin_cell(m.cs) + ", int " + pin_cell(m.intr);
  if (m.rst != GPIO_NUM_NC) {
    s += ", rst " + pin_cell(m.rst);
  }
  if (m.freq != 0) {
    s += " @ " + String(m.freq / 1000000) + " MHz";
  }
  return s;
}

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
  content += ".err{color:#ff6b6b}.warn{color:#ffd166}";
  content += "table{margin:0 auto;text-align:left}</style>";

  content += "<h2>Hardware configuration</h2>";
  content += "<button onclick=\"window.location.href='/'\">Back to main page</button>";

  // ── Active configuration ───────────────────────────────────────────────────
  content += "<div class='card'>";
  if (cfg.valid) {
    content += "<h3>Active: " + html_escape(cfg.name.c_str());
    if (!cfg.revision.empty()) {
      content += " (" + html_escape(cfg.revision.c_str()) + ")";
    }
    content += "</h3><table>";

    String led = "&mdash;";
    if (cfg.led != GPIO_NUM_NC) {
      led = pin_cell(cfg.led) + ", " + String(cfg.led_count) + " px, max brightness " +
            String(cfg.led_max_brightness);
    }
    add_row(content, "Status LED", led);
    if (cfg.display_sda != GPIO_NUM_NC) {
      add_row(content, "I2C display", "sda " + pin_cell(cfg.display_sda) + ", scl " + pin_cell(cfg.display_scl));
    }
    if (cfg.positive != GPIO_NUM_NC) {
      add_row(content, "Primary contactors",
              "positive " + pin_cell(cfg.positive) + ", negative " + pin_cell(cfg.negative) + ", precharge " +
                  pin_cell(cfg.precharge));
    }
    if (cfg.second_battery != GPIO_NUM_NC) {
      add_row(content, "Second battery", pin_cell(cfg.second_battery));
    }
    if (cfg.third_battery != GPIO_NUM_NC) {
      add_row(content, "Third battery", pin_cell(cfg.third_battery));
    }
    if (cfg.bms_power != GPIO_NUM_NC) {
      String s = pin_cell(cfg.bms_power);
      if (cfg.bms_power_active_low) {
        s += ", active low";
      }
      if (cfg.bms_power_always_on) {
        s += ", always on";
      }
      if (cfg.bms_power_reset_hold) {
        s += ", held across reset";
      }
      add_row(content, "BMS power", s);
    }
    if (cfg.hia4v1 != GPIO_NUM_NC) {
      add_row(content, "Automatic precharge",
              "hia4v1 " + pin_cell(cfg.hia4v1) + ", inverter disconnect " + pin_cell(cfg.inverter_disconnect));
    }
    if (cfg.sma_enable != GPIO_NUM_NC) {
      add_row(content, "SMA contactor enable", pin_cell(cfg.sma_enable));
    }
    if (cfg.can_tx != GPIO_NUM_NC) {
      add_row(content, "CAN (native)", "tx " + pin_cell(cfg.can_tx) + ", rx " + pin_cell(cfg.can_rx));
    }
    if (cfg.mcp2515.enabled) {
      add_row(content, "CAN (MCP2515)", mcp_pins(cfg.mcp2515, true));
    }
    for (int i = 0; i < 2; i++) {
      if (cfg.mcp2518fd[i].enabled) {
        add_row(content, i == 0 ? "CAN FD 1 (MCP2518FD)" : "CAN FD 2 (MCP2518FD)", mcp_pins(cfg.mcp2518fd[i], false));
      }
    }
    if (cfg.rs485_tx != GPIO_NUM_NC) {
      String s = "tx " + pin_cell(cfg.rs485_tx) + ", rx " + pin_cell(cfg.rs485_rx);
      if (cfg.rs485_de != GPIO_NUM_NC) {
        s += ", de/re " + pin_cell(cfg.rs485_de) + (cfg.rs485_de_active_high ? " (active high)" : " (active low)");
      }
      add_row(content, "RS485", s);
    }
    if (cfg.equipment_stop != GPIO_NUM_NC) {
      add_row(content, "Equipment stop", pin_cell(cfg.equipment_stop));
    }
    if (cfg.wup1 != GPIO_NUM_NC) {
      add_row(content, "Battery wake-up", "wup1 " + pin_cell(cfg.wup1) + ", wup2 " + pin_cell(cfg.wup2));
    }
    if (cfg.chademo_lock != GPIO_NUM_NC) {
      add_row(content, "CHAdeMO",
              "2 " + pin_cell(cfg.chademo_2) + ", 4 " + pin_cell(cfg.chademo_4) + ", 7 " + pin_cell(cfg.chademo_7) +
                  ", 10 " + pin_cell(cfg.chademo_10) + ", lock " + pin_cell(cfg.chademo_lock) + ", ct " +
                  pin_cell(cfg.chademo_ct));
    }
    add_row(content, "Long-press button", pin_cell(cfg.ap_button));
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
      if (!issue.port.empty()) {
        content += html_escape(issue.port.c_str()) + " &mdash; ";
      }
      content += html_escape(issue.text.c_str());
      content += "</li>";
    }
    content += "</ul></div>";
  }

  // ── Upload ─────────────────────────────────────────────────────────────────
  content += "<div class='card'><h3>Upload a configuration</h3>";
  content +=
      "<form method='POST' action='/hardware/upload' enctype='multipart/form-data'>"
      "<input type='file' name='config' accept='.json,application/json' required "
      "style='color:white'><br><br>"
      "<button type='submit'>Upload and reboot</button></form>";
  if (cfg.valid) {
    content += "<p><a href='/board.json' style='color:#8ab4f8'>Download the active configuration</a></p>";
  }
  content += "</div>";

  return content;
}

#endif  // HW_UNIFIED_S3
