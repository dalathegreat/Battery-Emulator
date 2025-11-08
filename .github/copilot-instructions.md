### Quick context (what this repo is)
- Firmware for an ESP32-based "Battery Emulator" (PlatformIO / Arduino framework) that translates EV battery packs to inverter interfaces.
- Source root for firmware: `Software/` (see `platformio.ini: src_dir = ./Software`).
- Unit tests live under `test/` and build with CMake + GoogleTest using small Arduino/FreeRTOS emulation in `test/emul/`.

### High-level architecture — essential facts
- Entry point: `Software/Software.cpp` — creates tasks (core/connectivity/logging/mqtt), registers transmitters, and orchestrates periodic updates.
- Data model: single global `datalayer` object (`Software/src/datalayer/*`) holds system/battery/inverter state. Many modules read/write `datalayer` directly.
- Pluggable batteries: `Software/src/battery/` contains per-car battery integrations. `BATTERIES.cpp` has the factory (`create_battery`) and `name_for_battery_type` — add new batteries here.
- Communication: CAN and RS485 are the main transports. CAN code lives in `Software/src/communication/can/` and uses third-party ACAN libraries under `Software/src/lib/`.
- Devboard / platform specifics: low-level hardware abstractions and peripherals (webserver, wifi, mqtt, sdcard) live under `Software/src/devboard/`. Hardware selection via `platformio.ini` macros (e.g. `-D HW_LILYGO`, `-D HW_LILYGO2CAN`).

### Build / test / debug workflows (concrete commands)
- Build firmware with PlatformIO (from repo root):
  - Build: `pio run -e <env>` (common envs: `lilygo_330`, `lilygo_2CAN_330`, `esp32dev`) — `platformio.ini` pins the espressif32 platform URL.
  - Upload: `pio run -e <env> -t upload`
  - Monitor serial: `pio device monitor -e <env>` (baud 115200; monitor filters configured in `platformio.ini`).
- Run unit tests (host):
  - Ensure CMake >= 3.28. From `test/`:
    - `mkdir -p build && cd build`
    - `cmake ..` then `cmake --build .` (or `make`)
    - `ctest --verbose` or `./tests` to run the GoogleTest suite.
  - Tests compile many `Software/src/*` files and use `test/emul/` to stub Arduino/FreeRTOS.

### Project-specific conventions and patterns (do not break these)
- File naming: many battery integration files use ALL-CAPS with hyphens, e.g. `RENAULT-ZOE-GEN1-BATTERY.cpp` — follow existing style when adding files.
- Battery integration contract: battery classes implement `setup()` and `update_values()` and expose a static `Name` constant used by `BATTERIES.cpp` factory. See `Software/src/battery/BATTERIES.cpp` and examples like `NISSAN-LEAF-BATTERY.cpp`.
- Initialization order matters: `setup_battery()` is called before `init_CAN()` in `Software.cpp` (CAN receivers should register before CAN is initialised).
- Double-battery support: code uses globals `battery` and `battery2` and `can_config.battery_double`; special constructors are used for secondary battery instances (see `BATTERIES.cpp`).
- Macros control platform behaviors: check `platformio.ini` build_flags (e.g. `-D HW_LILYGO2CAN`) and `system_settings.h` (task priorities, MAX_AMOUNT_CELLS).

### Integration points & external dependencies
- PlatformIO + espressif32 (custom platform URL pinned in `platformio.ini`)
- Libraries under `Software/src/lib/` are vendored and expected (eModbus, ACAN, AsyncWebServer, ElegantOTA, ArduinoJson, etc.).
- CAN integration is built on ACAN; many battery modules send/receive CAN frames via the `Transmitter`/`receive_can()` pattern in `Software.cpp`.
- NVM/settings persistence and web UI: see `Software/src/communication/nvm/comm_nvm.*` and `Software/src/devboard/webserver/`.

### How to add or modify a battery integration (concrete example)
1. Add new files under `Software/src/battery/`, name file consistent with repo style.
2. Add a class similar to existing batteries: static `Name` const, implement `setup()` and `update_values()`.
3. Register it in `BATTERIES.cpp` — add to `create_battery()` and `name_for_battery_type()`.
4. If you want unit tests, add the battery `.cpp` to `test/CMakeLists.txt`'s tests list (so host tests compile it with emulation).

### Quick code pointers for common tasks
- To find where a datapoint is stored/used: grep for `datalayer.<section>` (e.g. `datalayer.battery.status.*`).
- To add a periodic CAN transmitter: implement a `Transmitter` subclass or register via `register_transmitter()` (see `Software/Software.cpp` usage).
- To run a narrow compile locally (faster iteration): build a single PlatformIO environment: `pio run -e lilygo_2CAN_330`.

### Files you should inspect first
- `Software/Software.cpp` — main init & task layout
- `Software/src/battery/BATTERIES.cpp` — battery factory and registration
- `Software/src/datalayer/` — the central data model used across modules
- `platformio.ini` — build targets, macros and source root
- `test/CMakeLists.txt` and `test/emul/` — how unit tests emulate Arduino/FreeRTOS on host

If anything here is unclear or you want more examples (e.g. step-by-step for adding a new inverter module or how to mock CAN for tests), tell me which area to expand and I will update this file.
