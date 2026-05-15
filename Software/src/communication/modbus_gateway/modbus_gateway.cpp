#include "modbus_gateway.h"

#ifndef UNIT_TEST

#include <Arduino.h>

#include "../../battery/BATTERIES.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/utils/logging.h"
#include "../../inverter/INVERTERS.h"

namespace {
constexpr uint8_t kTcpHeaderSize = 7;
constexpr uint16_t kMinMbapLength = 2;
constexpr uint16_t kMaxMbapLength = 254;
constexpr uint16_t kMaxRtuFrame = 256;
constexpr uint8_t kInterByteGapMs = 20;

WiFiServer* gateway_server = nullptr;
WiFiClient gateway_client;
bool gateway_started = false;

bool conflict_protocol_selected() {
  const bool inverter_uses_rs485 = user_selected_inverter_protocol == InverterProtocolType::BydModbus ||
                                   user_selected_inverter_protocol == InverterProtocolType::Kostal ||
                                   user_selected_inverter_protocol == InverterProtocolType::PylonLV485 ||
                                   (user_selected_inverter_protocol != InverterProtocolType::None &&
                                    can_config.inverter == CAN_Interface::NO_CAN_INTERFACE);

  return inverter_uses_rs485 ||
         (user_selected_battery_type != BatteryType::None && can_config.battery == CAN_Interface::NO_CAN_INTERFACE) ||
         (user_selected_second_battery && can_config.battery_double == CAN_Interface::NO_CAN_INTERFACE) ||
         (user_selected_triple_battery && can_config.battery_triple == CAN_Interface::NO_CAN_INTERFACE) ||
         (user_selected_charger_type != ChargerType::None && can_config.charger == CAN_Interface::NO_CAN_INTERFACE) ||
         (user_selected_shunt_type != ShuntType::None && can_config.shunt == CAN_Interface::NO_CAN_INTERFACE);
}

bool read_exact(WiFiClient& client, uint8_t* data, size_t length, uint32_t timeout_ms) {
  const unsigned long start = millis();
  size_t offset = 0;
  while (offset < length) {
    if (!client.connected()) {
      return false;
    }

    while (client.available() <= 0) {
      if (!client.connected()) {
        return false;
      }
      if ((millis() - start) >= timeout_ms) {
        return false;
      }
      delay(1);
    }

    const int value = client.read();
    if (value < 0) {
      return false;
    }
    data[offset++] = static_cast<uint8_t>(value);
  }
  return true;
}

bool read_rtu_response(uint8_t* frame, size_t& length) {
  length = 0;
  const unsigned long start = millis();
  unsigned long last_byte = 0;
  bool seen_any = false;

  while (true) {
    while (Serial2.available() > 0) {
      if (length >= kMaxRtuFrame) {
        return false;
      }
      const int value = Serial2.read();
      if (value < 0) {
        return false;
      }
      frame[length++] = static_cast<uint8_t>(value);
      last_byte = millis();
      seen_any = true;
    }

    if (seen_any && length >= 5 && (millis() - last_byte) > kInterByteGapMs) {
      return true;
    }

    if ((millis() - start) >= modbus_gateway_timeout_ms) {
      return false;
    }

    delay(1);
  }
}

bool handle_client_request() {
  if (!gateway_client || !gateway_client.connected()) {
    return false;
  }

  uint8_t tcp_request[kTcpHeaderSize + kMaxMbapLength];
  if (!read_exact(gateway_client, tcp_request, kTcpHeaderSize, modbus_gateway_timeout_ms)) {
    return false;
  }

  const uint16_t transaction_id = (static_cast<uint16_t>(tcp_request[0]) << 8) | tcp_request[1];
  const uint16_t protocol_id = (static_cast<uint16_t>(tcp_request[2]) << 8) | tcp_request[3];
  const uint16_t length = (static_cast<uint16_t>(tcp_request[4]) << 8) | tcp_request[5];
  const uint8_t unit_id = tcp_request[6];

  if (protocol_id != 0 || length < kMinMbapLength || length > kMaxMbapLength) {
    return false;
  }

  const size_t pdu_length = static_cast<size_t>(length - 1);
  if (!read_exact(gateway_client, tcp_request + kTcpHeaderSize, pdu_length, modbus_gateway_timeout_ms)) {
    return false;
  }

  uint8_t rtu_request[kMaxRtuFrame];
  rtu_request[0] = unit_id;
  memcpy(rtu_request + 1, tcp_request + kTcpHeaderSize, pdu_length);
  const size_t rtu_request_length = pdu_length + 1;
  const uint16_t request_crc = modbus_gateway_crc16(rtu_request, rtu_request_length);
  rtu_request[rtu_request_length] = static_cast<uint8_t>(request_crc & 0xFF);
  rtu_request[rtu_request_length + 1] = static_cast<uint8_t>(request_crc >> 8);
  Serial2.write(rtu_request, rtu_request_length + 2);
  Serial2.flush();

  uint8_t rtu_response[kMaxRtuFrame];
  size_t rtu_response_length = 0;
  if (!read_rtu_response(rtu_response, rtu_response_length)) {
    return false;
  }

  if (rtu_response_length < 5) {
    return false;
  }

  const uint16_t expected_crc = (static_cast<uint16_t>(rtu_response[rtu_response_length - 1]) << 8) |
                                rtu_response[rtu_response_length - 2];
  const uint16_t actual_crc = modbus_gateway_crc16(rtu_response, rtu_response_length - 2);
  if (actual_crc != expected_crc) {
    return false;
  }

  uint8_t tcp_response[kTcpHeaderSize + kMaxRtuFrame];
  const size_t tcp_payload_length = rtu_response_length - 2;
  tcp_response[0] = static_cast<uint8_t>(transaction_id >> 8);
  tcp_response[1] = static_cast<uint8_t>(transaction_id & 0xFF);
  tcp_response[2] = 0;
  tcp_response[3] = 0;
  tcp_response[4] = static_cast<uint8_t>(tcp_payload_length >> 8);
  tcp_response[5] = static_cast<uint8_t>(tcp_payload_length & 0xFF);
  tcp_response[6] = rtu_response[0];
  memcpy(tcp_response + kTcpHeaderSize, rtu_response + 1, tcp_payload_length - 1);
  gateway_client.write(tcp_response, kTcpHeaderSize + tcp_payload_length);
  return true;
}

}  // namespace

bool modbus_gateway_enabled = false;
uint16_t modbus_gateway_port = 502;
uint32_t modbus_gateway_baud = 9600;
uint32_t modbus_gateway_timeout_ms = 60000;

bool setup_modbus_gateway() {
  if (!modbus_gateway_enabled || gateway_started) {
    return gateway_started;
  }

  if (conflict_protocol_selected()) {
    logging.println("Modbus TCP-to-RTU gateway disabled because another selected protocol owns RS485.");
    return false;
  }

  if (!gateway_server) {
    gateway_server = new WiFiServer(modbus_gateway_port);
    if (!gateway_server) {
      logging.println("Modbus TCP-to-RTU gateway failed to create TCP server.");
      return false;
    }
  }

  const auto rs485_en_pin = esp32hal->RS485_EN_PIN();
  const auto rs485_se_pin = esp32hal->RS485_SE_PIN();
  const auto rs485_rx_pin = esp32hal->RS485_RX_PIN();
  const auto rs485_tx_pin = esp32hal->RS485_TX_PIN();
  const auto pin_5v_en = esp32hal->PIN_5V_EN();

  if (!esp32hal->alloc_pins_ignore_unused("Modbus Gateway", rs485_en_pin, rs485_se_pin, rs485_rx_pin, rs485_tx_pin,
                                           pin_5v_en)) {
    logging.println("Modbus TCP-to-RTU gateway failed to reserve RS485 pins.");
    return false;
  }

  if (rs485_en_pin != GPIO_NUM_NC) {
    pinMode(rs485_en_pin, OUTPUT);
    digitalWrite(rs485_en_pin, HIGH);
  }

  if (rs485_se_pin != GPIO_NUM_NC) {
    pinMode(rs485_se_pin, OUTPUT);
    digitalWrite(rs485_se_pin, HIGH);
  }

  if (pin_5v_en != GPIO_NUM_NC) {
    pinMode(pin_5v_en, OUTPUT);
    digitalWrite(pin_5v_en, HIGH);
  }

  Serial2.begin(modbus_gateway_baud, SERIAL_8N1, rs485_rx_pin, rs485_tx_pin);
  gateway_server->begin();
  gateway_started = true;
  logging.printf("Modbus TCP-to-RTU gateway listening on port %u\n", static_cast<unsigned>(modbus_gateway_port));
  return true;
}

void loop_modbus_gateway() {
  if (!gateway_started || !gateway_server) {
    return;
  }

  if (!gateway_client || !gateway_client.connected()) {
    gateway_client = gateway_server->accept();
    if (!gateway_client) {
      return;
    }
  }

  if (!handle_client_request()) {
    gateway_client.stop();
  }
}

bool modbus_gateway_has_conflict() {
  return conflict_protocol_selected();
}

bool modbus_gateway_should_own_rs485() {
  return modbus_gateway_enabled && !modbus_gateway_has_conflict();
}

#endif

uint16_t modbus_gateway_crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
