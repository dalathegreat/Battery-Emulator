# Board configuration

A board config file describes the GPIO wiring of one physical board. It lets a single
firmware image run on any board of the same CPU architecture, so there is one
ESP32-S3 build rather than one build per board.

Files here are plain JSON, validated against `board-config-schema-1.json`. Point your
editor at that schema (the `$schema` key at the top of each file already does this in
VS Code) to get completion and typo checking while editing.

## Loading

The active configuration lives on the filesystem partition as `/board.json`. The
partition is the one labelled `spiffs` in the existing 16 MB partition table, so
nothing about the flash layout changes, but LittleFS is what is put in it.

There is no configuration compiled into the firmware. A freshly flashed board comes up
in **minimal mode**: Wi-Fi, the web server and the BOOT button on GPIO0, and nothing
else. The main page shows one card and a short row of buttons, and the settings page
offers only the network and web interface sections. Uploading a configuration and
rebooting unlocks the rest.

The upload path never destroys a working configuration:

1. the uploaded file is parsed and validated entirely in RAM;
2. on success it is written as `/board.json.new`, the previous file is kept as
   `/board.json.bak`, and the new one is renamed into place;
3. on failure nothing is written and the findings are shown on the upload page.

If a file that was valid at upload time fails to load at boot - a corrupted
filesystem, or a firmware upgrade that tightened validation - it is moved to
`/board.invalid.json`, the device falls back to minimal mode, and the file stays
downloadable so the problem can be seen rather than guessed at.

A factory reset formats the partition, since the configuration does not live in NVS
and clearing settings alone would leave the board configured.

## The hardware page

`/hardware` lists every port the file describes, in file order, with the pins each one
claims and what became of it:

- **active** - enabled and applied;
- **not enabled** - described in the file but not claimed, so its pins stay free;
- **invalid** - enabled but rejected, so nothing was applied. The reason is in the
  findings below the table.

Below that come the validation findings, each prefixed `Error:` or `Warning:`, and a
**Backup hardware configuration** card listing every file on the partition with a
download link. The quickest way to support a new board is to download the closest
existing file, change the pins and upload it back. `GET /board.json` also serves the
active file directly.

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

`sdcard` is accepted by the schema so the same schema version can cover the ESP32
family later, but this image is built without SD support: an enabled `sdcard` port
warns and reads as invalid.

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

### Port names

`name` is free text and is what the web UI shows. For the ports that carry a
communication interface - `nativecan`, `mcp2515`, `mcp2518fd` and `rs485` - it is also
the name that appears in the battery, inverter and shunt interface selectors on the
settings page, so a file naming its ports `CAN`, `CAN FD 1`, `CAN FD 2` and `RS485`
produces exactly those four entries. Only configured interfaces are listed and
initialised; an interface with no port behind it never appears.

### Sharing an SPI bus

Two `mcp2518fd` ports share a bus when they name the same `spi_bus` and only the first
declares `sck`, `sdi` and `sdo`. The second then carries just its own `cs` and `int`.

## Validation

Findings are reported in the boot log and on the hardware page, with the port name and
the offending pin. Two severities:

- **error** - the port is not enabled and reads as invalid. The rest of the
  configuration still loads, so a single bad port does not push the board into
  minimal mode.
- **warning** - logged, and the port is enabled anyway.

Setting `"strict": true` at the top level promotes every warning to an error.

### ESP32-S3 pin rules

Several rules depend on whether the firmware drives a pin or reads one that something
else drives, because a strapping pin is sampled at reset, when our own outputs are
still high impedance. A receive line - `rx`, `miso`, `sdo` - counts as externally
driven.

| Pins | Rule | Severity |
|---|---|---|
| 22-25 | Do not exist on this chip | error |
| 26-32 | SPI flash | error |
| 19, 20 | USB Serial/JTAG, in use because USB CDC is enabled | error |
| Any pin used twice by enabled ports | Conflict | error |
| 0 read as an input, other than `longpress_reset` | Held low at reset it selects download mode, so the board would not boot | error |
| 0 driven as an output, or the boot button | Cannot move the strap; a status LED here is fine | none |
| `reset_hold` on a pin outside 0-21 | Not RTC-capable, so the latch is dropped and the port stays | warning |
| `chademo.ct` outside 1-10 | ADC2 is unusable while Wi-Fi is up | error |
| 45, 46 driven as an output | Strapping: VDD_SPI voltage and boot mode | warning |
| 45, 46 read as an input that external equipment drives | A signal held at reset changes how the chip boots | warning |
| 3 read as an input that external equipment drives | Straps the JTAG source, ignored unless EFUSE_STRAP_JTAG_SEL is burned | warning |
| 3 driven as an output | Sampled at reset, when our output is high impedance | none |
| 43, 44 | UART0, ROM bootloader output appears here after reset | warning |
| 33-37 | Free only because PSRAM is left uninitialised | none |

Strapping misuse warns rather than refusing, because shipping hardware does it: the
BECom drives precharge from GPIO45. That works because the module has the VDD_SPI
eFuse burned, but it is a module property the firmware cannot verify, so it says so
and carries on. GPIO0 is the exception and is a hard error, because a board that will
not leave download mode cannot be recovered through the web UI.

## Files

| File | Board |
|---|---|
| `waveshare-esp32s3-rs485-can.json` | Waveshare ESP32-S3-RS485-CAN |
| `waveshare-esp32s3-rs485-can_triple.json` | The same board wired for three packs and two CAN FD interfaces |
| `becom.json` | BECom |
| `lilygo-t2can-mcp2515.json` | LilyGo T-2CAN with an MCP2515 |
| `lilygo-t2can-mcp2518fd.json` | LilyGo T-2CAN with an MCP2518FD |

The two T-2CAN files are different hardware, not a user choice. Pick the one that
matches the board; if in doubt, upload either and read the SPI CAN controller section
of the hardware page. The firmware probes the bus at boot and reports the controller
it found against the one the file configured, so a mismatch shows up there rather than
as a CAN interface that silently receives nothing.
