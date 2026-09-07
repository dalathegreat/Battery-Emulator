#ifndef _MODBUS_GATEWAY_H_
#define _MODBUS_GATEWAY_H_

// Inverter Modbus gateway (compiled in except on SMALL_FLASH_DEVICE/4 MB boards).
//
// When the inverter link uses CAN, the RS485/UART2 port is idle. This feature
// turns that idle RS485 into a transparent Modbus gateway to whatever Modbus
// device is wired to it (typically the inverter's Modbus/COM port): an eModbus
// RTU master owns the bus, and a Modbus-TCP server forwards network requests to
// it. Protocol-agnostic passthrough — no per-inverter register knowledge lives
// here; the meaning of registers stays with the network client.
//
// Phase 1 (this file): read-only transparent bridge, gated on a free RS485 bus.
// Phase 2 (later): HMAC-authenticated writes via the web API, IP-allowlist,
// audit, NVM/web configuration. Write function codes are intentionally NOT
// served here, so the raw TCP port is read-only.

// Call once from setup(), AFTER setup_inverter() (so the active inverter
// interface type is known for the free-bus gate). No-op on SMALL_FLASH_DEVICE
// boards, or unless the feature is enabled and the RS485 bus is free.
void modbus_gateway_init();

// Reserved for periodic housekeeping; currently a no-op (master and TCP server
// each run on their own task).
void modbus_gateway_loop();

// Register the HMAC-authenticated write endpoint (POST /modbus/write) on the
// emulator webserver. Call from init_webserver(). Writes never go over the raw
// Modbus-TCP port (that stays read-only); they are authenticated with an
// HMAC-SHA256 signature + monotonic nonce. No-op on SMALL_FLASH_DEVICE boards.
class AsyncWebServer;  // forward declaration to avoid pulling the async header here
void modbus_gateway_register_routes(AsyncWebServer& server);

#endif  // _MODBUS_GATEWAY_H_
