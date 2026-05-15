# Modbus TCP-to-RTU Gateway Design

## Goal

Add a configurable Modbus TCP-to-RTU gateway to the LilyGo T-CAN485 firmware so Home Assistant's Sunsynk integration can connect directly to the ESP32 over WiFi instead of using `mbusd` on a laptop with a USB-RS485 adapter.

Current path:

`Inverter RS485 -> USB-RS485 adapter -> laptop mbusd on 192.168.1.247:502 -> Home Assistant`

Target path:

`Inverter RS485 -> LilyGo T-CAN485 -> WiFi TCP port 502 -> Home Assistant`

## Approach

Implement a small raw Modbus gateway rather than expanding the vendored eModbus library.

The firmware will translate Modbus framing only:

- TCP request: MBAP header + unit id + PDU
- RTU request: unit id + PDU + CRC16
- RTU response: unit id + PDU + CRC16
- TCP response: original transaction id + MBAP header + unit id + PDU

This keeps the gateway function-code agnostic. It does not need to understand register maps or implement Modbus handlers, which reduces flash usage and avoids adding new library dependencies.

## Runtime Behavior

When enabled, the gateway will:

1. Listen on a configurable TCP port, default `502`.
2. Accept one active TCP client at a time, matching the current Sunsynk use case.
3. Read one complete Modbus TCP ADU from the client.
4. Validate the MBAP header enough to reject malformed packets.
5. Convert the unit id and PDU to RTU framing and write it to `Serial2`.
6. Wait for an RTU response until the configured timeout expires.
7. Validate and strip the RTU CRC.
8. Return the response using the original TCP transaction id.

If the RTU side times out or returns an invalid CRC, the TCP request will fail by closing the client connection. This mirrors the practical behavior expected by Modbus TCP clients, which will retry according to their own settings.

## Settings

Add a Modbus gateway section to the existing settings web UI, following the current `Preferences`/NVS pattern.

Initial settings:

- Enable Modbus TCP-to-RTU gateway, default disabled.
- TCP port, default `502`.
- RTU baud rate, default `9600`.
- RTU timeout, default `60000` ms.
- Serial mode fixed to `8N1` for the first implementation.

Settings will be loaded during boot from `BatteryEmulatorSettingsStore` and exposed through the existing `/settings` form and `/saveSettings` handler.

## RS485 Ownership

The RS485 port can only be owned by one firmware feature at a time.

The gateway is mutually exclusive with RS485 inverter protocols such as `BYD-MODBUS`, `PYLON-LV-RS485`, and `KOSTAL-RS485`. If the gateway is enabled while an RS485 inverter protocol is selected, the firmware must not start the gateway. The settings UI should make this conflict clear with a warning near the gateway controls.

This is acceptable for the target deployment because the user's battery and inverter communication currently uses CAN, leaving RS485 available for the gateway.

## Code Structure

Add a focused gateway module under the existing firmware source tree, likely in `Software/src/devboard/` or `Software/src/communication/` depending on nearby patterns found during implementation.

Expected API:

- `setup_modbus_gateway()` initializes settings, Serial2, and the TCP listener when enabled.
- `loop_modbus_gateway()` accepts and services the TCP client without blocking unrelated firmware work longer than the configured serial timeout for an active request.
- A local CRC16 helper handles Modbus RTU CRC generation and validation.

Main firmware setup and loop will call these functions only when the feature is enabled and WiFi is available.

## Validation

Implementation validation should include:

- Build verification with `pio run -e lilygo_330`.
- CRC16 verification against a known Modbus frame if a suitable test location exists.
- Manual integration test with the Sunsynk Home Assistant add-on pointed at the LilyGo IP and configured TCP port.

Success means Home Assistant can read Sunsynk inverter data through the LilyGo gateway using the same serial parameters as the existing `mbusd` setup: `9600 8N1`, TCP port `502`, and a 60 second RTU timeout.
