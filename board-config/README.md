# Board configuration

A board config file describes the GPIO wiring of one physical board. It lets a single
firmware image run on any board of the same CPU architecture, so there is one
ESP32-S3 build rather than one build per board.

Files here are plain JSON, validated against `board-config-schema-1.json`. Point your
editor at that schema (the `$schema` key at the top of each file already does this in
VS Code) to get completion and typo checking while editing.

## Loading

The active configuration lives on the filesystem partition as `/board.json`. There is
no configuration compiled into the firmware: a freshly flashed board comes up in
**minimal mode** with nothing but Wi-Fi, the web server and the BOOT button, and the
web UI offers a page to upload a file from this directory. After a successful upload
the device reboots and comes up configured, which unlocks the battery and inverter
settings.

The upload path never destroys a working configuration:

1. the uploaded file is parsed and validated entirely in RAM;
2. on success it is written as `/board.json.new`, the previous file is kept as
   `/board.json.bak`, and the new one is renamed into place;
3. on failure nothing is written and the errors are shown on the upload page.

If a file that was valid at upload time fails to load at boot — a corrupted
filesystem, or a firmware upgrade that tightened validation — it is moved to
`/board.invalid.json`, the device falls back to minimal mode, and the file stays
downloadable so the problem can be seen rather than guessed at.

The active configuration is downloadable at `GET /board.json`. The quickest way to
support a new board is to download the closest existing file, change the pins and
upload it back.

## Ports

`ports` is an array, not an object, because the same port type may appear several
times to describe alternative wiring of the same board. Each entry needs a `type`,
which the firmware matches on, and an `enabled` flag.

| Type | Pins | Extra fields |
|---|---|---|
| `statusled` | `pin` | `led_count`, `max_brightness` |
| `display_ssd1306` | `sda`, `scl` | |
| `contactor_control` | `positive`, `negative`, `precharge` | |
| `contactor_second_battery` | `pin` | |
| `contactor_third_battery` | `pin` | |
| `bms_power` | `pin` | `active_low`, `always_on`, `reset_hold` |
| `precharge_control` | `hia4v1`, `inverter_disconnect` | |
| `sma_enable` | `pin`, `led` | |
| `nativecan` | `tx`, `rx`, `se` | |
| `mcp2515` | `sck`, `mosi`, `miso`, `cs`, `int`, `rst` | `spi_bus`, `freq_hz` |
| `mcp2518fd` | `sck`, `sdi`, `sdo`, `cs`, `int` | `interface`, `spi_bus`, `freq_hz`, `clkodiv` |
| `rs485` | `tx`, `rx`, `de_re`, `en`, `se`, `pin_5v_en` | `de_active_high` |
| `e_stop` | `pin` | |
| `longpress_reset` | `pin` | |
| `battery_wakeup` | `wup1`, `wup2` | |
| `chademo` | `pin2`, `pin4`, `pin7`, `pin10`, `lock`, `ct` | |
| `sdcard` | `miso`, `mosi`, `sclk`, `cs` | `spi_bus` |

### Enabled and disabled ports

A disabled port is still parsed and validated, but its pins are not allocated. Pin
conflicts are only checked between *enabled* ports, which is what lets one file carry
several wiring options for the same pins.

- At most one enabled port per type, or per `(type, interface)` pair for `mcp2518fd`.
- Two enabled ports of the same type is an error.
- No enabled port of a type simply means the feature is absent on this board.

`enabled` describes what the board has, not what the installation uses. Whether a
second battery is actually connected, what kind of switch is wired to the equipment
stop input, and which inverter protocol is running all stay user settings in the web
UI. The file only says which pins exist and where they go.

### Sharing an SPI bus

Two `mcp2518fd` ports share a bus when they name the same `spi_bus` and only the first
declares `sck`, `sdi` and `sdo`. The second then carries just its own `cs` and `int`.

## Validation

Failures are reported in the log with the port name and the offending pin. Two
severities:

- **error** — the port is not enabled. The rest of the configuration still loads,
  so a single bad port does not push the board into minimal mode.
- **warning** — logged, and the port is enabled anyway.

Setting `"strict": true` at the top level promotes every warning to an error.

### ESP32-S3 pin rules

| Pins | Rule | Severity |
|---|---|---|
| 22–25 | Do not exist on this chip | error |
| 26–32 | SPI flash | error |
| 19, 20 | USB Serial/JTAG, in use because USB CDC is enabled | error |
| Any pin used twice by enabled ports | Conflict | error |
| `reset_hold` on a pin outside 0–21 | Not RTC-capable, cannot be latched across reset | error, flag dropped |
| `chademo.ct` outside 1–10 | ADC2 is unusable while Wi-Fi is up | error |
| 45, 46 driven as an output | Strapping: VDD_SPI voltage and boot mode | warning |
| 45, 46 read as an input that external equipment drives | A signal held high at reset changes how the chip boots | warning |
| 45, 46 read as an input with a fixed external pull to the safe level | Strapping value cannot change | none |
| 3 used at all | Strapping for JTAG source, ignored unless the eFuse is burned | warning |
| 0 used for anything but `longpress_reset` | Boot mode strapping | warning |
| 43, 44 | UART0, ROM bootloader output appears here after reset | warning |
| 33–37 | Free only because PSRAM is left uninitialised | none |

The three boards shipped here all trip at least one warning, which is why strapping
misuse warns by default rather than refusing to load. The one worth knowing about is
the BECom, which drives precharge from GPIO45; it works because the module has the
VDD_SPI eFuse burned, but that is a module property the firmware cannot verify.

## Files

| File | Board |
|---|---|
| `waveshare-esp32s3-rs485-can.json` | Waveshare ESP32-S3-RS485-CAN |
| `becom.json` | BECom |
| `lilygo-t2can-mcp2515.json` | LilyGo T-2CAN with an MCP2515 |
| `lilygo-t2can-mcp2518fd.json` | LilyGo T-2CAN with an MCP2518FD |

The two T-2CAN files are different hardware, not a user choice. The firmware no longer
probes the SPI bus at boot to tell the controllers apart; pick the file that matches
the board. If in doubt, read the controller's CANSTAT register: an MCP2515 answers
`0x80`.
