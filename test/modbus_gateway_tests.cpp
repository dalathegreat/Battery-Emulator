#include <gtest/gtest.h>

#include "../Software/src/communication/modbus_gateway/modbus_gateway.h"

TEST(ModbusGatewayCrc16Tests, ComputesKnownRequestCrc) {
  const uint8_t frame[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x0A};

  EXPECT_EQ(modbus_gateway_crc16(frame, sizeof(frame)), 0xCDC5);
}
