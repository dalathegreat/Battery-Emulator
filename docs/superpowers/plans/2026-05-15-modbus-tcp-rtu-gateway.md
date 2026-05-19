# Modbus TCP-to-RTU Gateway Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a configurable Modbus TCP-to-RTU gateway that lets Home Assistant/Sunsynk connect to the LilyGo T-CAN485 over WiFi on port 502 and transparently forward requests to the inverter over RS485.

**Architecture:** Add a small raw gateway module that translates Modbus TCP MBAP framing to `Serial2` RTU frames, without interpreting Modbus function codes. Store gateway settings in the existing ESP32 `Preferences` settings path and expose them in the existing settings web UI.

**Tech Stack:** ESP32 Arduino, PlatformIO, `WiFiServer`/`WiFiClient`, `HardwareSerial Serial2`, existing `BatteryEmulatorSettingsStore`, existing settings HTML/template processor.

---

## Implementation Tasks

1. Add `Software/src/communication/modbus_gateway/modbus_gateway.h` and `.cpp` with settings globals, CRC helper, `setup_modbus_gateway()`, and `loop_modbus_gateway()`.
2. Load `MBGWEN`, `MBGWPORT`, `MBGWBAUD`, and `MBGWTIME` from `Software/src/communication/nvm/comm_nvm.cpp`.
3. Save those settings from `/saveSettings` in `Software/src/devboard/webserver/webserver.cpp`.
4. Add settings placeholders and a Modbus gateway settings card in `Software/src/devboard/webserver/settings_html.cpp`.
5. Implement raw Modbus TCP-to-RTU forwarding in `modbus_gateway.cpp`: parse MBAP, add CRC for RTU request, read RTU response, verify CRC, re-wrap TCP response with original transaction id.
6. Integrate startup and loop calls in `Software/Software.cpp`.
7. Prevent RS485 double ownership by not starting the gateway when `BydModbus`, `Kostal`, or `PylonLV485` inverter protocols are selected.
8. Verify with `pio run -e lilygo_330` and inspect the final diff.

## Maintainer-Friendly Constraints

- Keep the gateway implementation self-contained in `Software/src/communication/modbus_gateway/`.
- Avoid vendoring new libraries or updating existing vendored code.
- Avoid broad refactors and formatting churn.
- Preserve existing settings patterns and key length limits.
- Keep web UI additions localized to one new card and the smallest possible placeholder additions.
