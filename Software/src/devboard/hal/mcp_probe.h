#ifndef _MCP_PROBE_H_
#define _MCP_PROBE_H_

#include <Arduino.h>
#include <SPI.h>
#include <soc/gpio_num.h>
#include "../../../src/devboard/utils/logging.h"

// Which SPI CAN controller is sitting on a given set of pins.
enum class McpKind : uint8_t {
  Absent,   // nothing answered
  Mcp2515,  // classic CAN
  Mcp2518,  // CAN FD
};

inline const char* name_for_mcp_kind(McpKind kind) {
  switch (kind) {
    case McpKind::Mcp2515:
      return "MCP2515";
    case McpKind::Mcp2518:
      return "MCP2518FD";
    default:
      return "none";
  }
}

// Bit-bang the MCP2515 "read CANSTAT" sequence and decide from the answer which
// controller is fitted. An MCP2515 replies 0x80 after a reset; an MCP2518FD has
// no such register at that address and replies with something else. 0xFF is read
// back when nothing drives the bus at all, which we report as Absent rather than
// guessing at a controller that is not there.
//
// Takes about 2 ms, and leaves every pin it touched back in high impedance so the
// real driver can claim them afterwards.
//
// rst may be GPIO_NUM_NC on boards that do not break the reset line out.
inline McpKind probe_mcp(uint8_t spi_bus, gpio_num_t sck, gpio_num_t miso, gpio_num_t mosi, gpio_num_t cs,
                         gpio_num_t rst) {
  if (sck == GPIO_NUM_NC || miso == GPIO_NUM_NC || mosi == GPIO_NUM_NC || cs == GPIO_NUM_NC) {
    return McpKind::Absent;
  }

  if (rst != GPIO_NUM_NC) {
    pinMode(rst, INPUT_PULLDOWN);        // Assert reset (if MCP2515)
    vTaskDelay(1 / portTICK_PERIOD_MS);  // Wait for reset
    pinMode(rst, INPUT_PULLUP);          // Deassert reset
  }

  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);              // Ensure CS is high to start with
  vTaskDelay(1 / portTICK_PERIOD_MS);  // Wait for chip to settle

  SPISettings settings(100000, MSBFIRST, SPI_MODE0);
  SPIClass spi(spi_bus);
  spi.begin(sck, miso, mosi);

  // Read MCP2515 CANSTAT register
  const uint8_t tx_data[] = {0x03, 0x0E, 0x00};
  uint8_t rx_data[3] = {0};

  spi.beginTransaction(settings);
  digitalWrite(cs, LOW);
  spi.transferBytes(tx_data, rx_data, 3);
  digitalWrite(cs, HIGH);
  spi.endTransaction();

  pinMode(cs, INPUT);  // Set CS back to high impedance
  spi.end();

  McpKind kind;
  if (rx_data[2] == 0x80) {
    kind = McpKind::Mcp2515;
  } else if (rx_data[2] == 0xFF) {
    kind = McpKind::Absent;
  } else {
    kind = McpKind::Mcp2518;
  }

  logging.printf("SPI CAN probe: %s (ret=0x%02X)\n", name_for_mcp_kind(kind), rx_data[2]);
  return kind;
}

#endif  // _MCP_PROBE_H_
