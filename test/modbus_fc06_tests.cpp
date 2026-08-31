#include <gtest/gtest.h>

#include <vector>

#include "../Software/src/inverter/ModbusInverterProtocol.h"

// Regression tests for ModbusInverterProtocol::FC06 (Write Single Register).
//
// A Modbus FC06 response is a byte-for-byte echo of the request:
//     [slaveID][0x06][addrHi][addrLo][valHi][valLo]
// FC06 previously omitted the address and answered with 4 bytes instead of 6.
// Masters that validate the echo reject that; a Delta E6-TL-US will not finish
// its boot handshake without it.
//
// The expectations below are not invented. They are the actual bytes an
// LG RESU10H Prime put on the wire in reply to a Delta E6-TL-US, captured
// 2026-08-26 during an inverter cold start (payload only -- the CRC is added by
// the RTU layer, not by FC06).

namespace {

class Fc06TestInverter : public ModbusInverterProtocol {
 public:
  Fc06TestInverter() : ModbusInverterProtocol(kServerId) {}
  const char* name() override { return "FC06 regression test"; }
  void update_values() override {}

  using ModbusInverterProtocol::FC06;
  using ModbusInverterProtocol::mbPV;

  static constexpr int kServerId = 15;  // 0x0F, the LG RESU's Modbus address
};

std::vector<uint8_t> bytes_of(ModbusMessage& m) {  // eModbus data()/size() are non-const
  return std::vector<uint8_t>(m.data(), m.data() + m.size());
}

// One real write from the captured boot handshake.
struct CapturedWrite {
  uint16_t reg;
  uint16_t value;
};

// The six configuration writes the Delta issues at +3.0..+5.1 s after boot,
// followed by the standby/active pair it sends at +7.2/+7.7 s to a battery
// that has power-cycled. Values are identical across four captured cold starts.
const CapturedWrite kBootWrites[] = {
    {1102, 4100},                             // discharge DC-bus rail, 0.1 V
    {1103, 4200},                             // charge DC-bus rail, 0.1 V
    {1104, 350},  {1106, 350}, {1109, 7000},  // peak discharge nameplate, W
    {1109, 5000},                             // continuous nameplate, W
    {1101, 1},                                // standby
    {1101, 3},                                // active
};

}  // namespace

TEST(ModbusFc06, ResponseEchoesAddressAndValue) {
  Fc06TestInverter inverter;

  ModbusMessage request(Fc06TestInverter::kServerId, 0x06, uint16_t{1102}, uint16_t{4100});
  ModbusMessage response = inverter.FC06(request);

  const std::vector<uint8_t> expected = {0x0F, 0x06, 0x04, 0x4E, 0x10, 0x04};
  EXPECT_EQ(bytes_of(response), expected) << "FC06 must echo the request: [id][06][addrHi][addrLo][valHi][valLo]";
  EXPECT_EQ(response.size(), 6u) << "a 4-byte response means the address was omitted";
}

TEST(ModbusFc06, EchoesEveryWriteOfTheCapturedBootHandshake) {
  Fc06TestInverter inverter;

  for (const CapturedWrite& w : kBootWrites) {
    ModbusMessage request(Fc06TestInverter::kServerId, 0x06, w.reg, w.value);
    ModbusMessage response = inverter.FC06(request);

    EXPECT_EQ(bytes_of(response), bytes_of(request))
        << "reg " << w.reg << " <- " << w.value << ": response is not a byte-for-byte echo";
  }
}

TEST(ModbusFc06, StoresTheWrittenValue) {
  Fc06TestInverter inverter;

  for (const CapturedWrite& w : kBootWrites) {
    ModbusMessage request(Fc06TestInverter::kServerId, 0x06, w.reg, w.value);
    inverter.FC06(request);
  }

  // 1109 is written twice (7000 then 5000); the later value must win.
  EXPECT_EQ(inverter.mbPV[1109], 5000);
  EXPECT_EQ(inverter.mbPV[1102], 4100);
  EXPECT_EQ(inverter.mbPV[1103], 4200);
  // 1101 likewise: standby then active.
  EXPECT_EQ(inverter.mbPV[1101], 3);
}

TEST(ModbusFc06, RejectsOutOfRangeAddress) {
  Fc06TestInverter inverter;

  ModbusMessage request(Fc06TestInverter::kServerId, 0x06, uint16_t{40000}, uint16_t{1});
  ModbusMessage response = inverter.FC06(request);

  ASSERT_GE(response.size(), 3u);
  EXPECT_EQ(response[1], 0x86) << "an exception response sets the high bit of the function code";
  EXPECT_EQ(response[2], ILLEGAL_DATA_ADDRESS);
}
