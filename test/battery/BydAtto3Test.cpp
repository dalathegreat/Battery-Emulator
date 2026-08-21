#include <gtest/gtest.h>

#include "../../Software/src/battery/BYD-ATTO-3-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/datalayer/datalayer_extended.h"

#include "Arduino.h"

// File-scope in the .cpp, no header. Lets tests build frames the driver will accept.
extern uint8_t computeBydChecksum(const uint8_t* u8);

namespace {

CAN_frame byd_frame(uint32_t id, std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = {};
  frame.DLC = 8;
  frame.ID = id;
  uint8_t i = 0;
  for (uint8_t b : bytes) {
    if (i >= 8) {
      break;
    }
    frame.data.u8[i++] = b;
  }
  return frame;
}

// Builds a frame from the first 7 bytes, computing byte 7 the way the BMS does.
CAN_frame byd_checksummed_frame(uint32_t id, std::initializer_list<uint8_t> first7bytes) {
  CAN_frame frame = byd_frame(id, first7bytes);
  frame.data.u8[7] = computeBydChecksum(frame.data.u8);
  return frame;
}

// Same with a bad checksum. Payload must differ from the previous decode, or acceptance and
// rejection look identical.
CAN_frame byd_corrupt_frame(uint32_t id, std::initializer_list<uint8_t> first7bytes) {
  CAN_frame frame = byd_frame(id, first7bytes);
  frame.data.u8[7] = (uint8_t)(computeBydChecksum(frame.data.u8) ^ 0xFF);
  return frame;
}

// Clears the shared global datalayer between tests, so none sees values left by another.
void reset_byd_state() {
  datalayer.battery.status = DATALAYER_BATTERY_STATUS_TYPE{};
  datalayer.battery.settings.max_user_set_charge_dA = 300;
  datalayer_extended.bydAtto3.chargePower = 0;
  datalayer_extended.bydAtto3.dischargePower = 0;
  datalayer_extended.bydAtto3.SOC_polled = 0;
  datalayer_extended.bydAtto3.SOC_highprec = 0;
  datalayer_extended.bydAtto3.BMS_min_temp_module_number = 0;
  datalayer_extended.bydAtto3.BMS_max_temp_module_number = 0;
  datalayer_extended.bydAtto3.pack_voltage_dV = 0;
}

// Coldest sensor 9 at 8C, hottest sensor 1 at 24C, SOC 99.2%, average 16C.
CAN_frame temperature_frame() {
  return byd_checksummed_frame(0x447, {0x09, 0x30, 0x01, 0x40, 0xE0, 0x03, 0x38});
}

// Discharge 285.5kW (b0:b1 = 0x0B27), charge 131.1kW (b2:b3 = 0x051F).
CAN_frame power_limit_frame() {
  return byd_checksummed_frame(0x345, {0x27, 0x0B, 0x1F, 0x05, 0x00, 0x00, 0x00});
}

}  // namespace

TEST(BydAtto3Tests, ShouldDecode0x345PowerLimitsAndRejectBadChecksum) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  battery->handle_incoming_can_frame(power_limit_frame());
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.dischargePower, 2855);
  EXPECT_EQ(datalayer_extended.bydAtto3.chargePower, 1311);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);

  // 100 / 200 with a bad checksum: a gate failing open would show those instead of 2855 / 1311.
  battery->handle_incoming_can_frame(byd_corrupt_frame(0x345, {0x64, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.dischargePower, 2855);
  EXPECT_EQ(datalayer_extended.bydAtto3.chargePower, 1311);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1);

  // Same payload, valid checksum: accepted, so the checksum caused the rejection above.
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x345, {0x64, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.dischargePower, 100);
  EXPECT_EQ(datalayer_extended.bydAtto3.chargePower, 200);
}

TEST(BydAtto3Tests, ShouldDecode0x447TemperaturesSensorNumbersAndHighPrecisionSoc) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  battery->handle_incoming_can_frame(temperature_frame());
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.BMS_min_temp_module_number, 9);
  EXPECT_EQ(datalayer_extended.bydAtto3.BMS_max_temp_module_number, 1);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 80);   // 8C
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 240);  // 24C
  EXPECT_EQ(datalayer.battery.status.real_soc, 9920);           // 99.2%, scaled by 100
  EXPECT_EQ(datalayer_extended.bydAtto3.SOC_highprec, 992);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);

  // Sensor 1 at 40C / sensor 2 at 56C / SOC 25.6%, all materially different, with a bad checksum.
  battery->handle_incoming_can_frame(byd_corrupt_frame(0x447, {0x01, 0x50, 0x02, 0x68, 0x00, 0x01, 0x50}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.BMS_min_temp_module_number, 9);
  EXPECT_EQ(datalayer_extended.bydAtto3.BMS_max_temp_module_number, 1);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 80);
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 240);
  EXPECT_EQ(datalayer.battery.status.real_soc, 9920);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1);
}

TEST(BydAtto3Tests, ShouldDecode0x444WholePercentSocAndRejectBadChecksum) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  // b0/b1 = whole-volt voltage (420V), b2:b3 = current (offset 5000, here 0A), b4 = SOH (93%),
  // b5 = whole-percent SOC (31%)
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x444, {0xA4, 0x01, 0x88, 0x13, 0x5D, 0x1F, 0x00}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.SOC_polled, 31);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9300);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);

  // SOH 50% / SOC 80%, both materially different, with a bad checksum.
  battery->handle_incoming_can_frame(byd_corrupt_frame(0x444, {0xA4, 0x01, 0x88, 0x13, 0x32, 0x50, 0x00}));
  battery->update_values();

  EXPECT_EQ(datalayer_extended.bydAtto3.SOC_polled, 31);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9300);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 1);
}

TEST(BydAtto3Tests, ShouldPrefer0x438VoltageWhenItAgreesWith0x444) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  // Atto payload: 0x444 says 420V whole-volt, 0x438 says 4203 dV. Real captures peak at 8 dV apart.
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x444, {0xA4, 0x01, 0x88, 0x13, 0x5D, 0x1F, 0x00}));
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x438, {0x55, 0x55, 0x01, 0xD6, 0x47, 0x6B, 0x10}));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4203);
  EXPECT_EQ(datalayer_extended.bydAtto3.pack_voltage_dV, 4203);
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);
}

TEST(BydAtto3Tests, ShouldFallBackTo0x444WhenPackDoesNotCarryVoltageIn0x438) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  // Real 142S VM7 payloads. b5:b6 there is not voltage and decodes to 36352 dV (3635.2V), which
  // used to win over 0x444 and trip EVENT_BATTERY_OVERVOLTAGE.
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x444, {0xD2, 0x01, 0x88, 0x13, 0x64, 0x1C, 0x4E}));
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x438, {0x55, 0x55, 0x05, 0xFF, 0x00, 0x00, 0x8E}));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4660);
  // The detail page must show the resolved voltage too, not the rejected 0x438 value.
  EXPECT_EQ(datalayer_extended.bydAtto3.pack_voltage_dV, 4660);
  // A rejected pack family is not a bus error.
  EXPECT_EQ(datalayer.battery.status.CAN_error_counter, 0);
}

TEST(BydAtto3Tests, ShouldIgnore0x438UntilAReference0x444HasArrived) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  // No 0x444 yet, so there is nothing to cross-check against and 0x438 must not be adopted.
  const uint16_t initial_voltage_dV = datalayer.battery.status.voltage_dV;
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x438, {0x55, 0x55, 0x05, 0xFF, 0x00, 0x00, 0x8E}));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, initial_voltage_dV);

  // Once the reference arrives, the same 0x438 is judged and rejected.
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x444, {0xD2, 0x01, 0x88, 0x13, 0x64, 0x1C, 0x4E}));
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x438, {0x55, 0x55, 0x05, 0xFF, 0x00, 0x00, 0x8E}));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4660);
}

TEST(BydAtto3Tests, ShouldStopTrusting0x438OnceItDivergesFrom0x444) {
  reset_byd_state();
  auto battery = new BydAttoBattery();
  battery->setup();

  battery->handle_incoming_can_frame(byd_checksummed_frame(0x444, {0xA4, 0x01, 0x88, 0x13, 0x5D, 0x1F, 0x00}));
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x438, {0x55, 0x55, 0x01, 0xD6, 0x47, 0x6B, 0x10}));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4203);

  // 0x438 now reads 4260 dV against 420V, 60 dV apart and past the tolerance. No stale value latches.
  battery->handle_incoming_can_frame(byd_checksummed_frame(0x438, {0x55, 0x55, 0x01, 0xD6, 0x47, 0xA4, 0x10}));
  battery->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4200);
  EXPECT_EQ(datalayer_extended.bydAtto3.pack_voltage_dV, 4200);
}
