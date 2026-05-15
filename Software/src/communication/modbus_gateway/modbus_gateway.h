#ifndef MODBUS_GATEWAY_H
#define MODBUS_GATEWAY_H

#include <stddef.h>
#include <stdint.h>

extern bool modbus_gateway_enabled;
extern uint16_t modbus_gateway_port;
extern uint32_t modbus_gateway_baud;
extern uint32_t modbus_gateway_timeout_ms;

#ifndef UNIT_TEST
#include <WiFi.h>

bool setup_modbus_gateway();
void loop_modbus_gateway();
bool modbus_gateway_has_conflict();
bool modbus_gateway_should_own_rs485();
#endif

uint16_t modbus_gateway_crc16(const uint8_t* data, size_t length);

#endif
