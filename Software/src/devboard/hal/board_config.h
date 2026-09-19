#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#ifdef HW_UNIFIED_S3

#include <soc/gpio_num.h>
#include <cstdint>
#include <string>
#include <vector>
#include "../utils/types.h"
#include "mcp_probe.h"

// Paths on the filesystem partition. The active file is board.json; the two
// others exist so that neither a bad upload nor a firmware upgrade that
// tightens validation can destroy a configuration the user cannot get back.
#define BOARD_CONFIG_PATH "/board.json"
#define BOARD_CONFIG_BACKUP_PATH "/board.json.bak"
#define BOARD_CONFIG_INVALID_PATH "/board.invalid.json"

// Largest configuration we will accept. The four shipped files are 3-5 kB; the
// cap is here so a bad upload cannot exhaust the heap during parsing.
#define BOARD_CONFIG_MAX_BYTES 16384

// How bad a validation finding is.
enum class ConfigIssueLevel : uint8_t {
  Warning,  // logged, port still enabled
  Error,    // port not enabled; the rest of the file still loads
};

// One row of the hardware page: every port the file describes, in file order,
// whether it was enabled or not. Built while parsing so the page is a faithful
// view of the file rather than a hand-written list that can miss a port type.
struct PortRow {
  std::string type;
  std::string name;
  std::string pins;
  bool enabled;
};

struct ConfigIssue {
  ConfigIssueLevel level;
  std::string port;  // port name from the file, or "" for file-level findings
  std::string text;
};

// One SPI CAN controller as described by the file.
struct McpPorts {
  bool enabled = false;
  std::string name;
  uint8_t bus = 0;
  uint32_t freq = 0;  // 0 means unknown/autodetect
  int clkodiv = 0b11;
  gpio_num_t sck = GPIO_NUM_NC;
  gpio_num_t sdi = GPIO_NUM_NC;  // MOSI on the MCP2515
  gpio_num_t sdo = GPIO_NUM_NC;  // MISO on the MCP2515
  gpio_num_t cs = GPIO_NUM_NC;
  gpio_num_t intr = GPIO_NUM_NC;
  gpio_num_t rst = GPIO_NUM_NC;  // MCP2515 only
};

// The whole board, flattened out of the JSON so that nothing downstream has to
// know the file format. Every pin defaults to GPIO_NUM_NC, which is what the
// HAL base class returns for an absent peripheral, so an empty config behaves
// like a board with no peripherals at all rather than like a misconfigured one.
struct BoardConfig {
  bool valid = false;
  std::string name;
  std::string revision;

  // Status LED
  gpio_num_t led = GPIO_NUM_NC;
  uint8_t led_count = 1;
  uint8_t led_max_brightness = 40;

  // I2C display
  gpio_num_t display_sda = GPIO_NUM_NC;
  gpio_num_t display_scl = GPIO_NUM_NC;

  // Contactors
  gpio_num_t positive = GPIO_NUM_NC;
  gpio_num_t negative = GPIO_NUM_NC;
  gpio_num_t precharge = GPIO_NUM_NC;
  gpio_num_t second_battery = GPIO_NUM_NC;
  gpio_num_t third_battery = GPIO_NUM_NC;

  // BMS power
  gpio_num_t bms_power = GPIO_NUM_NC;
  bool bms_power_active_low = false;
  bool bms_power_always_on = false;
  bool bms_power_reset_hold = false;

  // Automatic precharge
  gpio_num_t hia4v1 = GPIO_NUM_NC;
  gpio_num_t inverter_disconnect = GPIO_NUM_NC;

  // SMA inverter contactor enable
  gpio_num_t sma_enable = GPIO_NUM_NC;
  gpio_num_t sma_led = GPIO_NUM_NC;

  // Native CAN
  gpio_num_t can_tx = GPIO_NUM_NC;
  gpio_num_t can_rx = GPIO_NUM_NC;
  gpio_num_t can_se = GPIO_NUM_NC;

  // SPI CAN controllers: one MCP2515 and two MCP2518FD interfaces
  McpPorts mcp2515;
  McpPorts mcp2518fd[2];

  // RS485
  gpio_num_t rs485_tx = GPIO_NUM_NC;
  gpio_num_t rs485_rx = GPIO_NUM_NC;
  gpio_num_t rs485_de = GPIO_NUM_NC;
  gpio_num_t rs485_en = GPIO_NUM_NC;
  gpio_num_t rs485_se = GPIO_NUM_NC;
  gpio_num_t pin_5v_en = GPIO_NUM_NC;
  bool rs485_de_active_high = true;

  // Discrete I/O
  gpio_num_t equipment_stop = GPIO_NUM_NC;
  gpio_num_t ap_button = GPIO_NUM_NC;
  gpio_num_t wup1 = GPIO_NUM_NC;
  gpio_num_t wup2 = GPIO_NUM_NC;

  // CHAdeMO
  gpio_num_t chademo_2 = GPIO_NUM_NC;
  gpio_num_t chademo_4 = GPIO_NUM_NC;
  gpio_num_t chademo_7 = GPIO_NUM_NC;
  gpio_num_t chademo_10 = GPIO_NUM_NC;
  gpio_num_t chademo_lock = GPIO_NUM_NC;
  gpio_num_t chademo_ct = GPIO_NUM_NC;

  std::vector<comm_interface> interfaces;
  std::vector<ConfigIssue> issues;
  std::vector<PortRow> rows;

  bool has_interface(comm_interface iface) const;

  // Result of the boot-time SPI probe against the configured CAN A pins.
  McpKind probed = McpKind::Absent;
  bool probe_ran = false;

  bool has_errors() const;
  bool probe_mismatch() const;
  const char* configured_controller() const;
};

// The one instance. Always readable; valid is false until a file has loaded.
extern BoardConfig board_config;

// Mounts the filesystem, reads BOARD_CONFIG_PATH, parses and validates it, and
// runs the SPI CAN probe. Safe to call when no file exists: board_config stays
// invalid and the firmware comes up in minimal mode. Call before init_hal().
void init_board_config();

// Validates a document held in RAM without touching the stored one. Used by the
// upload handler so a rejected file never reaches flash. issues is filled in
// either way; the return value is false when the document could not be used.
bool validate_board_config(const char* json, size_t length, std::vector<ConfigIssue>& issues);

// Writes a document that validate_board_config() already accepted, keeping the
// previous one as BOARD_CONFIG_BACKUP_PATH. Returns false if flash rejected it,
// in which case the previous configuration is still the active one.
bool store_board_config(const char* json, size_t length);

#endif  // HW_UNIFIED_S3
#endif  // _BOARD_CONFIG_H_
