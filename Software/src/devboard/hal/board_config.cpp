#include "board_config.h"

#ifdef HW_UNIFIED_S3

#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include <map>
#include <memory>
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../utils/logging.h"

BoardConfig board_config;

namespace {

// ── ESP32-S3 pin capabilities ────────────────────────────────────────────────
//
// Everything here is a property of the chip, not of any particular board, which
// is why it lives in code rather than in the configuration file.

// How the firmware uses a pin. Decides which strapping rule applies: driving a
// strapping pin and letting external equipment drive it are different hazards,
// and a bus pin is driven by us but only after boot.
enum class PinRole : uint8_t { Output, Input, Bus };

bool pin_exists(int pin) {
  // GPIO0-21 and GPIO26-48. 22-25 are not bonded out on the S3.
  return (pin >= 0 && pin <= 21) || (pin >= 26 && pin <= 48);
}

bool pin_is_flash(int pin) {
  return pin >= 26 && pin <= 32;
}

bool pin_is_usb(int pin) {
  // USB Serial/JTAG, claimed because the unified image builds with USB CDC on boot.
  return pin == 19 || pin == 20;
}

bool pin_is_rtc(int pin) {
  return pin >= 0 && pin <= 21;
}

bool pin_is_adc1(int pin) {
  return pin >= 1 && pin <= 10;
}

bool pin_is_uart0(int pin) {
  return pin == 43 || pin == 44;
}

const char* strapping_reason(int pin) {
  switch (pin) {
    case 0:
      return "boot mode";
    case 3:
      return "JTAG source select";
    case 45:
      return "VDD_SPI voltage";
    case 46:
      return "boot mode";
    default:
      return nullptr;
  }
}

// ── Validation state ─────────────────────────────────────────────────────────

class Validator {
 public:
  Validator(std::vector<ConfigIssue>& issues, bool strict) : issues_(issues), strict_(strict) {}

  void warn(const std::string& port, const std::string& text) {
    // In strict mode the user has asked for every finding to stop the port.
    issues_.push_back({strict_ ? ConfigIssueLevel::Error : ConfigIssueLevel::Warning, port, text});
    if (strict_) {
      port_failed_ = true;
    }
  }

  void error(const std::string& port, const std::string& text) {
    issues_.push_back({ConfigIssueLevel::Error, port, text});
    port_failed_ = true;
  }

  // Validates one pin and records who claimed it. Returns GPIO_NUM_NC when the
  // pin cannot be used, so the caller can carry on collecting further findings
  // instead of bailing out on the first one.
  gpio_num_t take(const std::string& port, const char* role_name, int pin, PinRole role) {
    if (pin < 0) {
      return GPIO_NUM_NC;  // absent, which is legitimate for optional pins
    }
    if (!pin_exists(pin)) {
      error(port, std::string(role_name) + ": GPIO" + std::to_string(pin) + " does not exist on the ESP32-S3");
      return GPIO_NUM_NC;
    }
    if (pin_is_flash(pin)) {
      error(port, std::string(role_name) + ": GPIO" + std::to_string(pin) + " is wired to the SPI flash");
      return GPIO_NUM_NC;
    }
    if (pin_is_usb(pin)) {
      error(port, std::string(role_name) + ": GPIO" + std::to_string(pin) + " is the USB Serial/JTAG port");
      return GPIO_NUM_NC;
    }

    auto claimed = claims_.find(pin);
    if (claimed != claims_.end()) {
      error(port, std::string(role_name) + ": GPIO" + std::to_string(pin) + " is already used by " + claimed->second);
      return GPIO_NUM_NC;
    }
    claims_[pin] = port + " " + role_name;

    const char* strap = strapping_reason(pin);
    if (strap != nullptr) {
      if (pin == 0) {
        // GPIO0 is the BOOT button on every S3 board we know of, so using it for
        // the button is the expected case and worth no remark at all.
        if (role != PinRole::Input) {
          warn(port, std::string(role_name) + ": GPIO0 is the boot mode strapping pin");
        }
      } else if (pin == 3) {
        warn(port, std::string(role_name) +
                       ": GPIO3 straps the JTAG source, which is ignored unless EFUSE_STRAP_JTAG_SEL is burned");
      } else if (role == PinRole::Input) {
        warn(port, std::string(role_name) + ": GPIO" + std::to_string(pin) + " straps the " + strap +
                       "; equipment driving it high at reset changes how the chip boots");
      } else {
        warn(port, std::string(role_name) + ": GPIO" + std::to_string(pin) + " straps the " + strap +
                       "; its level at reset decides how the chip boots");
      }
    }

    if (pin_is_uart0(pin)) {
      warn(port, std::string(role_name) + ": GPIO" + std::to_string(pin) +
                     " is UART0, so ROM bootloader output appears here after every reset");
    }

    return static_cast<gpio_num_t>(pin);
  }

  void begin_port() { port_failed_ = false; }
  bool port_failed() const { return port_failed_; }

 private:
  std::vector<ConfigIssue>& issues_;
  bool strict_;
  bool port_failed_ = false;
  std::map<int, std::string> claims_;
};

int pin_of(JsonObjectConst gpio, const char* key) {
  return gpio[key] | -1;
}

// ── Parsing ──────────────────────────────────────────────────────────────────

// Fills out, which may be nullptr when we only want the findings. Returns false
// when the document cannot be used at all, as opposed to individual ports being
// rejected.
bool parse_document(const char* json, size_t length, BoardConfig* out, std::vector<ConfigIssue>& issues) {
  if (length == 0 || length > BOARD_CONFIG_MAX_BYTES) {
    issues.push_back({ConfigIssueLevel::Error, "",
                      "File is empty or larger than the " + std::to_string(BOARD_CONFIG_MAX_BYTES) + " byte limit"});
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json, length);
  if (err) {
    issues.push_back({ConfigIssueLevel::Error, "", std::string("Not valid JSON: ") + err.c_str()});
    return false;
  }

  int version = doc["config_version"] | 0;
  if (version != 1) {
    issues.push_back({ConfigIssueLevel::Error, "",
                      "config_version is " + std::to_string(version) + ", this firmware understands 1"});
    return false;
  }

  const char* mcu = doc["board"]["mcu"] | "";
  if (strcmp(mcu, "esp32s3") != 0) {
    issues.push_back(
        {ConfigIssueLevel::Error, "", std::string("board.mcu is \"") + mcu + "\", this firmware runs on esp32s3"});
    return false;
  }

  JsonArrayConst ports = doc["ports"];
  if (ports.isNull() || ports.size() == 0) {
    issues.push_back({ConfigIssueLevel::Error, "", "No ports defined"});
    return false;
  }

  BoardConfig scratch;
  BoardConfig& cfg = (out != nullptr) ? *out : scratch;
  cfg = BoardConfig();
  cfg.name = doc["board"]["name"] | "Unnamed board";
  cfg.revision = doc["board"]["revision"] | "";

  Validator v(issues, doc["strict"] | false);
  std::map<std::string, int> enabled_count;

  for (JsonObjectConst port : ports) {
    const char* type = port["type"] | "";
    const char* name = port["name"] | type;
    bool enabled = port["enabled"] | false;
    JsonObjectConst gpio = port["gpio"];

    if (!enabled) {
      continue;  // described but not claimed, so its pins are free for others
    }

    // Two enabled ports of the same type would both want the same HAL slot.
    std::string key = type;
    if (strcmp(type, "mcp2518fd") == 0) {
      key += std::to_string(port["interface"] | 1);
    }
    if (++enabled_count[key] > 1) {
      issues.push_back({ConfigIssueLevel::Error, name,
                        std::string("A second \"") + type + "\" port is already enabled; only one may be"});
      continue;
    }

    v.begin_port();

    if (strcmp(type, "statusled") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.led = pin;
      cfg.led_count = port["led_count"] | 1;
      cfg.led_max_brightness = port["max_brightness"] | 40;

    } else if (strcmp(type, "display_ssd1306") == 0) {
      gpio_num_t sda = v.take(name, "sda", pin_of(gpio, "sda"), PinRole::Bus);
      gpio_num_t scl = v.take(name, "scl", pin_of(gpio, "scl"), PinRole::Bus);
      if (v.port_failed())
        continue;
      cfg.display_sda = sda;
      cfg.display_scl = scl;

    } else if (strcmp(type, "contactor_control") == 0) {
      gpio_num_t pos = v.take(name, "positive", pin_of(gpio, "positive"), PinRole::Output);
      gpio_num_t neg = v.take(name, "negative", pin_of(gpio, "negative"), PinRole::Output);
      gpio_num_t pre = v.take(name, "precharge", pin_of(gpio, "precharge"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.positive = pos;
      cfg.negative = neg;
      cfg.precharge = pre;

    } else if (strcmp(type, "contactor_second_battery") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.second_battery = pin;

    } else if (strcmp(type, "contactor_third_battery") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.third_battery = pin;

    } else if (strcmp(type, "bms_power") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Output);
      bool hold = port["reset_hold"] | false;
      if (hold && pin != GPIO_NUM_NC && !pin_is_rtc(pin)) {
        // The port stays usable, the latch does not: RTC hold only reaches GPIO0-21.
        // Reported as a warning for that reason - failing the whole port here would
        // take BMS power away over a feature the board can simply do without.
        issues.push_back({ConfigIssueLevel::Warning, name,
                          "reset_hold needs an RTC-capable pin (GPIO0-21), so the latch is dropped"});
        hold = false;
      }
      if (v.port_failed())
        continue;
      cfg.bms_power = pin;
      cfg.bms_power_active_low = port["active_low"] | false;
      cfg.bms_power_always_on = port["always_on"] | false;
      cfg.bms_power_reset_hold = hold;

    } else if (strcmp(type, "precharge_control") == 0) {
      gpio_num_t hia = v.take(name, "hia4v1", pin_of(gpio, "hia4v1"), PinRole::Output);
      gpio_num_t dis = v.take(name, "inverter_disconnect", pin_of(gpio, "inverter_disconnect"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.hia4v1 = hia;
      cfg.inverter_disconnect = dis;

    } else if (strcmp(type, "sma_enable") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Input);
      gpio_num_t led = v.take(name, "led", pin_of(gpio, "led"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.sma_enable = pin;
      cfg.sma_led = led;

    } else if (strcmp(type, "nativecan") == 0) {
      gpio_num_t tx = v.take(name, "tx", pin_of(gpio, "tx"), PinRole::Bus);
      gpio_num_t rx = v.take(name, "rx", pin_of(gpio, "rx"), PinRole::Bus);
      gpio_num_t se = v.take(name, "se", pin_of(gpio, "se"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.can_tx = tx;
      cfg.can_rx = rx;
      cfg.can_se = se;
      cfg.interfaces.push_back(comm_interface::CanNative);

    } else if (strcmp(type, "mcp2515") == 0) {
      McpPorts m;
      m.sck = v.take(name, "sck", pin_of(gpio, "sck"), PinRole::Bus);
      m.sdi = v.take(name, "mosi", pin_of(gpio, "mosi"), PinRole::Bus);
      m.sdo = v.take(name, "miso", pin_of(gpio, "miso"), PinRole::Bus);
      m.cs = v.take(name, "cs", pin_of(gpio, "cs"), PinRole::Output);
      m.intr = v.take(name, "int", pin_of(gpio, "int"), PinRole::Input);
      m.rst = v.take(name, "rst", pin_of(gpio, "rst"), PinRole::Output);
      if (v.port_failed())
        continue;
      m.enabled = true;
      m.name = name;
      m.bus = (strcmp(port["spi_bus"] | "hspi", "fspi") == 0) ? FSPI : HSPI;
      m.freq = port["freq_hz"] | 0;
      cfg.mcp2515 = m;
      cfg.interfaces.push_back(comm_interface::CanAddonMcp2515);

    } else if (strcmp(type, "mcp2518fd") == 0) {
      int index = (port["interface"] | 1) - 1;
      if (index < 0 || index > 1) {
        issues.push_back({ConfigIssueLevel::Error, name, "interface must be 1 or 2"});
        continue;
      }
      McpPorts m;
      m.sck = v.take(name, "sck", pin_of(gpio, "sck"), PinRole::Bus);
      m.sdi = v.take(name, "sdi", pin_of(gpio, "sdi"), PinRole::Bus);
      m.sdo = v.take(name, "sdo", pin_of(gpio, "sdo"), PinRole::Bus);
      m.cs = v.take(name, "cs", pin_of(gpio, "cs"), PinRole::Output);
      m.intr = v.take(name, "int", pin_of(gpio, "int"), PinRole::Input);
      if (v.port_failed())
        continue;
      if (m.cs == GPIO_NUM_NC || m.intr == GPIO_NUM_NC) {
        issues.push_back({ConfigIssueLevel::Error, name, "cs and int are required"});
        continue;
      }
      m.enabled = true;
      m.name = name;
      m.bus = (strcmp(port["spi_bus"] | "fspi", "hspi") == 0) ? HSPI : FSPI;
      m.freq = port["freq_hz"] | 0;
      m.clkodiv = port["clkodiv"] | 0b11;
      cfg.mcp2518fd[index] = m;
      cfg.interfaces.push_back(index == 0 ? comm_interface::CanFdAddonMcp2518 : comm_interface::CanFdAddonMcp2518_2);

    } else if (strcmp(type, "rs485") == 0) {
      gpio_num_t tx = v.take(name, "tx", pin_of(gpio, "tx"), PinRole::Bus);
      gpio_num_t rx = v.take(name, "rx", pin_of(gpio, "rx"), PinRole::Bus);
      gpio_num_t de = v.take(name, "de_re", pin_of(gpio, "de_re"), PinRole::Output);
      gpio_num_t en = v.take(name, "en", pin_of(gpio, "en"), PinRole::Output);
      gpio_num_t se = v.take(name, "se", pin_of(gpio, "se"), PinRole::Output);
      gpio_num_t en5v = v.take(name, "pin_5v_en", pin_of(gpio, "pin_5v_en"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.rs485_tx = tx;
      cfg.rs485_rx = rx;
      cfg.rs485_de = de;
      cfg.rs485_en = en;
      cfg.rs485_se = se;
      cfg.pin_5v_en = en5v;
      cfg.rs485_de_active_high = port["de_active_high"] | true;
      cfg.interfaces.push_back(comm_interface::RS485);
      cfg.interfaces.push_back(comm_interface::Modbus);

    } else if (strcmp(type, "e_stop") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Input);
      if (v.port_failed())
        continue;
      cfg.equipment_stop = pin;

    } else if (strcmp(type, "longpress_reset") == 0) {
      gpio_num_t pin = v.take(name, "pin", pin_of(gpio, "pin"), PinRole::Input);
      if (v.port_failed())
        continue;
      cfg.ap_button = pin;

    } else if (strcmp(type, "battery_wakeup") == 0) {
      gpio_num_t w1 = v.take(name, "wup1", pin_of(gpio, "wup1"), PinRole::Output);
      gpio_num_t w2 = v.take(name, "wup2", pin_of(gpio, "wup2"), PinRole::Output);
      if (v.port_failed())
        continue;
      cfg.wup1 = w1;
      cfg.wup2 = w2;

    } else if (strcmp(type, "chademo") == 0) {
      gpio_num_t p2 = v.take(name, "pin2", pin_of(gpio, "pin2"), PinRole::Input);
      gpio_num_t p4 = v.take(name, "pin4", pin_of(gpio, "pin4"), PinRole::Input);
      gpio_num_t p7 = v.take(name, "pin7", pin_of(gpio, "pin7"), PinRole::Output);
      gpio_num_t p10 = v.take(name, "pin10", pin_of(gpio, "pin10"), PinRole::Output);
      gpio_num_t lock = v.take(name, "lock", pin_of(gpio, "lock"), PinRole::Output);
      gpio_num_t ct = v.take(name, "ct", pin_of(gpio, "ct"), PinRole::Input);
      if (ct != GPIO_NUM_NC && !pin_is_adc1(ct)) {
        v.error(name, "ct must be on ADC1 (GPIO1-10); ADC2 is unusable while Wi-Fi is up");
      }
      if (v.port_failed())
        continue;
      cfg.chademo_2 = p2;
      cfg.chademo_4 = p4;
      cfg.chademo_7 = p7;
      cfg.chademo_10 = p10;
      cfg.chademo_lock = lock;
      cfg.chademo_ct = ct;

    } else if (strcmp(type, "sdcard") == 0) {
      // Accepted by the schema for the ESP32 family, but this image is built
      // without SD support, so say so rather than silently dropping it.
      issues.push_back({ConfigIssueLevel::Warning, name, "SD card support is not built into this image"});

    } else {
      issues.push_back({ConfigIssueLevel::Warning, name, std::string("Unknown port type \"") + type + "\", ignored"});
    }
  }

  cfg.valid = true;
  return true;
}

}  // namespace

bool BoardConfig::has_errors() const {
  for (const auto& issue : issues) {
    if (issue.level == ConfigIssueLevel::Error) {
      return true;
    }
  }
  return false;
}

const char* BoardConfig::configured_controller() const {
  if (mcp2518fd[0].enabled) {
    return "MCP2518FD";
  }
  if (mcp2515.enabled) {
    return "MCP2515";
  }
  return "none";
}

bool BoardConfig::probe_mismatch() const {
  if (!probe_ran) {
    return false;
  }
  return strcmp(configured_controller(), name_for_mcp_kind(probed)) != 0;
}

bool validate_board_config(const char* json, size_t length, std::vector<ConfigIssue>& issues) {
  issues.clear();
  if (!parse_document(json, length, nullptr, issues)) {
    return false;
  }
  for (const auto& issue : issues) {
    if (issue.level == ConfigIssueLevel::Error) {
      // Individual ports may fail and the file still be worth storing, but the
      // upload page reports them so the user can fix the file before rebooting.
      return true;
    }
  }
  return true;
}

bool store_board_config(const char* json, size_t length) {
  LittleFS.remove(BOARD_CONFIG_BACKUP_PATH);
  if (LittleFS.exists(BOARD_CONFIG_PATH)) {
    LittleFS.rename(BOARD_CONFIG_PATH, BOARD_CONFIG_BACKUP_PATH);
  }

  File f = LittleFS.open(BOARD_CONFIG_PATH, "w");
  if (!f) {
    logging.println("Board config: could not open " BOARD_CONFIG_PATH " for writing");
    LittleFS.rename(BOARD_CONFIG_BACKUP_PATH, BOARD_CONFIG_PATH);
    return false;
  }
  size_t written = f.write(reinterpret_cast<const uint8_t*>(json), length);
  f.close();

  if (written != length) {
    logging.printf("Board config: wrote %u of %u bytes, restoring the previous file\n", (unsigned)written,
                   (unsigned)length);
    LittleFS.remove(BOARD_CONFIG_PATH);
    LittleFS.rename(BOARD_CONFIG_BACKUP_PATH, BOARD_CONFIG_PATH);
    return false;
  }

  logging.printf("Board config: stored %u bytes\n", (unsigned)written);
  return true;
}

void init_board_config() {
  // The partition is labelled spiffs for backwards compatibility with the
  // existing partition tables, but LittleFS is what we put in it: it mounts in
  // milliseconds where SPIFFS scans the whole 3.4 MB, and it survives a power
  // cut mid-write. Formatting on first boot after an upgrade takes a few
  // seconds and happens here, before any task that could be starved by it.
  if (!LittleFS.begin(true, "/littlefs", 10, "spiffs")) {
    logging.println("Board config: filesystem could not be mounted, staying unconfigured");
    return;
  }

  File f = LittleFS.open(BOARD_CONFIG_PATH, "r");
  if (!f || f.isDirectory()) {
    logging.println("Board config: no " BOARD_CONFIG_PATH ", booting minimal - upload one from the web UI");
    return;
  }

  size_t length = f.size();
  if (length == 0 || length > BOARD_CONFIG_MAX_BYTES) {
    logging.printf("Board config: %s is %u bytes, refusing to parse it\n", BOARD_CONFIG_PATH, (unsigned)length);
    f.close();
    return;
  }

  std::unique_ptr<char[]> buffer(new (std::nothrow) char[length + 1]);
  if (!buffer) {
    logging.println("Board config: not enough heap to read the file");
    f.close();
    return;
  }
  f.readBytes(buffer.get(), length);
  buffer[length] = '\0';
  f.close();

  std::vector<ConfigIssue> issues;
  bool usable = parse_document(buffer.get(), length, &board_config, issues);
  board_config.issues = issues;

  if (!usable) {
    // The file was valid when it was uploaded, so something changed under it.
    // Keep it for inspection rather than leaving the user guessing.
    board_config = BoardConfig();
    board_config.issues = issues;
    LittleFS.remove(BOARD_CONFIG_INVALID_PATH);
    LittleFS.rename(BOARD_CONFIG_PATH, BOARD_CONFIG_INVALID_PATH);
    logging.println("Board config: file rejected, kept as " BOARD_CONFIG_INVALID_PATH ", booting minimal");
    for (const auto& issue : issues) {
      logging.printf("  %s\n", issue.text.c_str());
    }
    return;
  }

  logging.printf("Board config: loaded \"%s\"\n", board_config.name.c_str());
  for (const auto& issue : board_config.issues) {
    logging.printf("  %s: %s %s\n", issue.level == ConfigIssueLevel::Error ? "ERROR" : "warning", issue.port.c_str(),
                   issue.text.c_str());
  }

  // Probe whatever the configuration calls CAN A, before any driver claims the
  // pins, so a file written for the other T-2CAN variant shows up as a mismatch
  // on the hardware page instead of as a CAN bus that never receives anything.
  const McpPorts& first = board_config.mcp2518fd[0].enabled ? board_config.mcp2518fd[0] : board_config.mcp2515;
  if (first.enabled) {
    board_config.probed = probe_mcp(first.bus, first.sck, first.sdo, first.sdi, first.cs, first.rst);
    board_config.probe_ran = true;
    if (board_config.probe_mismatch()) {
      logging.printf("Board config: %s is configured but a %s answered on those pins\n",
                     board_config.configured_controller(), name_for_mcp_kind(board_config.probed));
    }
  }
}

#endif  // HW_UNIFIED_S3
