#include <gtest/gtest.h>

#include "../../Software/src/battery/BATTERIES.h"
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

CAN_frame uds_reply(std::initializer_list<uint8_t> bytes) {
  return byd_frame(0x7EF, bytes);
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
  datalayer_extended.bydAtto3.keep_iso_disabled = false;
  datalayer_extended.bydAtto3.iso_command_status = 0;
  datalayer_extended.bydAtto3.iso_measurement_active = false;
}

// Coldest sensor 9 at 8C, hottest sensor 1 at 24C, SOC 99.2%, average 16C.
CAN_frame temperature_frame() {
  return byd_checksummed_frame(0x447, {0x09, 0x30, 0x01, 0x40, 0xE0, 0x03, 0x38});
}

// Discharge 285.5kW (b0:b1 = 0x0B27), charge 131.1kW (b2:b3 = 0x051F).
CAN_frame power_limit_frame() {
  return byd_checksummed_frame(0x345, {0x27, 0x0B, 0x1F, 0x05, 0x00, 0x00, 0x00});
}

// 0x35E b0 bit7 = isolation measurement running: 0x01 disabled, 0x81 re-armed.
CAN_frame iso_measurement_frame(bool active) {
  return byd_checksummed_frame(0x35E, {(uint8_t)(active ? 0x81 : 0x01), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

// 0x344 carries contactor feedback; receiving it is what marks the BMS alive.
CAN_frame contactor_feedback_frame(uint8_t mode) {
  return byd_frame(0x344, {mode, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

}  // namespace

TEST(BydAtto3BalanceApiTests, RejectsUnconfiguredBatteryIndices) {
  user_selected_battery_type = BatteryType::BydAtto3;
  battery = new BydAttoBattery();
  battery2 = new BydAttoBattery();

  EXPECT_TRUE(byd_cell_balance_times_available(0));
  EXPECT_FALSE(byd_cell_balance_times_available(1));
  EXPECT_FALSE(byd_cell_balance_times_available(2));

  user_selected_second_battery = true;
  EXPECT_TRUE(byd_cell_balance_times_available(1));

  delete battery;
  battery = nullptr;
  delete battery2;
  battery2 = nullptr;
  // These are process-wide globals; leaving them set would leak into every later test.
  user_selected_second_battery = false;
  user_selected_battery_type = BatteryType::None;
}

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

// The monitor re-arms ~30s after a contactor open, with the BMS never restarting.
TEST(BydAtto3Tests, ShouldReDisableIsolationMonitorWhenItRearmsWithoutABmsRestart) {
  reset_byd_state();
  set_millis64(10000);
  auto battery = new BydAttoBattery();
  battery->setup();

  // Come alive with the setting off, so the BMS-start edge arms nothing and cannot fire again.
  battery->handle_incoming_can_frame(contactor_feedback_frame(0x84));
  battery->handle_incoming_can_frame(iso_measurement_frame(false));
  battery->update_values();
  EXPECT_EQ(datalayer_extended.bydAtto3.iso_command_status, 0);

  datalayer_extended.bydAtto3.keep_iso_disabled = true;

  // A contactor cycle keeps the BMS alive, so the 0x35E edge is the only report of the re-arm.
  set_millis64(60000);
  battery->handle_incoming_can_frame(contactor_feedback_frame(0x84));
  battery->handle_incoming_can_frame(iso_measurement_frame(true));
  battery->update_values();
  EXPECT_EQ(datalayer_extended.bydAtto3.iso_command_status, 1);
}

class BydAtto3BalanceTimeTest : public testing::Test {
 protected:
  void SetUp() override {
    reset_byd_state();
    datalayer.battery.info.number_of_cells = 0;
    battery = new BydAttoBattery();
    battery->setup();
  }

  void TearDown() override { delete battery; }

  BydAttoBattery* battery = nullptr;
};

TEST_F(BydAtto3BalanceTimeTest, RejectsScanUntilCellCountIsKnown) {
  EXPECT_FALSE(battery->request_cell_balance_times());
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::NOT_READ);
}

TEST_F(BydAtto3BalanceTimeTest, ReadsMatchingDidsAndKeepsZeroDistinctFromUnread) {
  datalayer.battery.info.number_of_cells = 2;

  // DID 0x0004 (completed charges), not 0x000B (sessions entered).
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x04, 0x7B, 0x00, 0xAA, 0xAA}));
  ASSERT_TRUE(battery->request_cell_balance_times());
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::NOT_READ);
  EXPECT_NE(battery->cell_balance_times_json().str().find("\"s\":1"), std::string::npos);

  battery->transmit_can(200);  // Cell 1, DID 0x0040.
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::READING);

  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x41, 0xFF, 0x7F, 0xAA, 0xAA}));
  EXPECT_EQ(battery->cell_balance_times().received_cells, 0);

  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x40, 0xA4, 0x01, 0xAA, 0xAA}));
  EXPECT_EQ(battery->cell_balance_times().hours[0], 420);
  EXPECT_TRUE(battery->cell_balance_times().cell_valid(0));

  battery->transmit_can(400);  // Cell 2, DID 0x0041.
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x41, 0x00, 0x00, 0xAA, 0xAA}));

  const BydCellBalanceTimeData& result = battery->cell_balance_times();
  EXPECT_EQ(result.state, BydCellBalanceTimeState::COMPLETE);
  EXPECT_EQ(result.received_cells, 2);
  EXPECT_EQ(result.hours[1], 0);
  EXPECT_TRUE(result.cell_valid(1));
  EXPECT_EQ(result.charge_cycles, 123);
  EXPECT_TRUE(result.charge_cycles_valid);
  EXPECT_EQ(result.scan_id, 1u);
}

TEST_F(BydAtto3BalanceTimeTest, CountsCompletedChargesNotSessionsEntered) {
  datalayer.battery.info.number_of_cells = 1;

  // 0x000B counts every session entered, so it must not reach the scan result.
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x0B, 0xC7, 0x00, 0xAA, 0xAA}));
  ASSERT_TRUE(battery->request_cell_balance_times());
  battery->transmit_can(200);
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x40, 0xA4, 0x01, 0xAA, 0xAA}));

  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::COMPLETE);
  EXPECT_FALSE(battery->cell_balance_times().charge_cycles_valid);
  EXPECT_EQ(battery->cell_balance_times().charge_cycles, 0);
}

TEST_F(BydAtto3BalanceTimeTest, NegativeReplyProducesUsefulPartialResult) {
  datalayer.battery.info.number_of_cells = 2;
  ASSERT_TRUE(battery->request_cell_balance_times());
  battery->transmit_can(200);

  battery->handle_incoming_can_frame(uds_reply({0x03, 0x7F, 0x22, 0x31, 0x00, 0x00, 0x00, 0x00}));
  battery->transmit_can(400);
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x41, 0xDD, 0x01, 0xAA, 0xAA}));

  const BydCellBalanceTimeData& result = battery->cell_balance_times();
  EXPECT_EQ(result.state, BydCellBalanceTimeState::PARTIAL);
  EXPECT_EQ(result.received_cells, 1);
  EXPECT_FALSE(result.cell_valid(0));
  EXPECT_TRUE(result.cell_valid(1));
  EXPECT_EQ(result.hours[1], 477);
}

TEST_F(BydAtto3BalanceTimeTest, ResponsePendingDoesNotAdvanceTheCellCursor) {
  datalayer.battery.info.number_of_cells = 1;
  ASSERT_TRUE(battery->request_cell_balance_times());
  battery->transmit_can(200);

  battery->handle_incoming_can_frame(uds_reply({0x03, 0x7F, 0x22, 0x78, 0x00, 0x00, 0x00, 0x00}));
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::READING);
  EXPECT_EQ(battery->cell_balance_times().received_cells, 0);

  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x40, 0xA4, 0x01, 0xAA, 0xAA}));
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::COMPLETE);
  EXPECT_EQ(battery->cell_balance_times().hours[0], 420);
}

TEST_F(BydAtto3BalanceTimeTest, ResponsePendingCannotHoldTheScanOpen) {
  datalayer.battery.info.number_of_cells = 1;
  ASSERT_TRUE(battery->request_cell_balance_times());
  battery->transmit_can(200);

  battery->handle_incoming_can_frame(uds_reply({0x03, 0x7F, 0x22, 0x78, 0x00, 0x00, 0x00, 0x00}));
  // Must be past CELL_BALANCE_TIME_SCAN_TIMEOUT_MS, otherwise the scan is still legitimately open.
  battery->transmit_can(90200);

  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::FAILED);
  EXPECT_EQ(battery->cell_balance_times().scan_id, 1u);
}

TEST_F(BydAtto3BalanceTimeTest, DefersResetAndOtherDiagnosticsUntilTheBatteryTaskStartsTheScan) {
  datalayer.battery.info.number_of_cells = 1;
  ASSERT_TRUE(battery->request_cell_balance_times());
  battery->transmit_can(200);
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x40, 0xA4, 0x01, 0xAA, 0xAA}));
  ASSERT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::COMPLETE);

  ASSERT_TRUE(battery->request_cell_balance_times());
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::COMPLETE);
  EXPECT_EQ(battery->cell_balance_times().hours[0], 420);
  EXPECT_NE(battery->cell_balance_times_json().str().find("\"s\":1"), std::string::npos);

  datalayer_extended.bydAtto3.UserRequestIsoRoutineDisable = true;
  battery->update_values();
  EXPECT_TRUE(datalayer_extended.bydAtto3.UserRequestIsoRoutineDisable);

  battery->transmit_can(400);
  EXPECT_EQ(battery->cell_balance_times().state, BydCellBalanceTimeState::READING);
  battery->handle_incoming_can_frame(uds_reply({0x05, 0x62, 0x00, 0x40, 0xA5, 0x01, 0xAA, 0xAA}));
  battery->update_values();
  EXPECT_FALSE(datalayer_extended.bydAtto3.UserRequestIsoRoutineDisable);
}

TEST_F(BydAtto3BalanceTimeTest, TimesOutAfterOneRetry) {
  datalayer.battery.info.number_of_cells = 1;
  ASSERT_TRUE(battery->request_cell_balance_times());
  battery->transmit_can(200);
  battery->transmit_can(800);
  battery->transmit_can(1400);

  const BydCellBalanceTimeData& result = battery->cell_balance_times();
  EXPECT_EQ(result.state, BydCellBalanceTimeState::FAILED);
  EXPECT_EQ(result.expected_cells, 1);
  EXPECT_EQ(result.received_cells, 0);
  EXPECT_EQ(result.scan_id, 1u);
}

// TX frame capture injected by the emulated CAN layer (see emul/can.cpp).
void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

const uint16_t kPackVolts = 426;  // 0x441 reports pack - 1
const uint16_t kLowLink = 12;     // bytes 4-5 = 0x0C 0x00, the floating link
const uint16_t kNo441 = 0xFFFF;

// 0x344 b0 = contactor feedback, b1 = BMS precharge state: 0x00 open, 0x40 moving, 0x41 running.
CAN_frame contactor_state_frame(uint8_t b0, uint8_t b1) {
  return byd_frame(0x344, {b0, b1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

// 0x444 sets battery_voltage and BMS_voltage_available; without it 0x441 only ever reports the low link.
CAN_frame pack_voltage_frame() {
  return byd_checksummed_frame(
      0x444, {(uint8_t)(kPackVolts & 0xFF), (uint8_t)((kPackVolts >> 8) & 0x0F), 0x88, 0x13, 0x64, 0x50, 0x00});
}

void byd_precharge_setup() {
  reset_byd_state();
  clear_transmitted_frames();
  datalayer.system.status.system_status = ACTIVE;
  datalayer.system.status.inverter_allows_contactor_closing = true;
  datalayer.system.info.equipment_stop_active = false;
}

// The RX edge stamps millis(), the ramp reads currentMillis; the test clock has to drive both.
void rx_at(BydAttoBattery* battery, const CAN_frame& frame, uint32_t ms) {
  set_millis64(ms);
  battery->handle_incoming_can_frame(frame);
}

uint16_t link_on_tick(BydAttoBattery* battery, uint32_t ms) {
  clear_transmitted_frames();
  set_millis64(ms);
  battery->transmit_can(ms);
  const std::vector<CAN_frame>& frames = get_transmitted_frames();
  for (size_t i = frames.size(); i > 0; i--) {
    if (frames[i - 1].ID == 0x441) {
      return (uint16_t)((frames[i - 1].data.u8[5] << 8) | frames[i - 1].data.u8[4]);
    }
  }
  return kNo441;
}

// 0x441 is only computed once counter_100ms > 3, so run six 100ms ticks before asserting anything.
uint16_t warmup(BydAttoBattery* battery, uint8_t b0, uint8_t b1, uint32_t start_ms) {
  uint16_t link = kNo441;
  for (uint32_t i = 0; i < 6; i++) {
    const uint32_t ms = start_ms + i * 100;
    rx_at(battery, contactor_state_frame(b0, b1), ms);
    rx_at(battery, pack_voltage_frame(), ms);
    link = link_on_tick(battery, ms);
  }
  return link;
}

}  // namespace

// Rebooting into a live pack must not fake a precharge. The first 0x344 also carries b1 = 0x41, which
// looks like a precharge edge, so b0's main-closed bit has to outrank it.
TEST(BydAtto3PrechargeTests, BootIntoAClosedPackReportsPackVoltageWithoutRamping) {
  byd_precharge_setup();
  auto battery = new BydAttoBattery();
  battery->setup();

  EXPECT_EQ(warmup(battery, 0x84, 0x41, 0), kPackVolts - 1);

  delete battery;
}

// Boot with the pack open is also CONTACTORS_CLOSING, so the wait timer starts when this block first
// runs. Timed from uptime 0 instead, a boot slower than 3s times out at once and reports full pack.
TEST(BydAtto3PrechargeTests, BootWithThePackOpenHoldsTheLowLinkThroughASlowBoot) {
  byd_precharge_setup();
  auto battery = new BydAttoBattery();
  battery->setup();

  EXPECT_EQ(warmup(battery, 0x00, 0x00, 4000), kLowLink);
  EXPECT_EQ(link_on_tick(battery, 6000), kLowLink);

  delete battery;
}

// A real close: b1 goes 0x40 well before b0 reports closed, and the car walks the link up over ~900ms.
// Asserting pack voltage before the BMS has precharged reads as a stuck contactor - P1A3400.
TEST(BydAtto3PrechargeTests, RampsTheLinkOnlyAfterTheBmsStartsPrecharging) {
  byd_precharge_setup();
  auto battery = new BydAttoBattery();
  battery->setup();

  EXPECT_EQ(warmup(battery, 0x00, 0x00, 0), kLowLink);

  rx_at(battery, contactor_state_frame(0x00, 0x40), 600);
  EXPECT_EQ(link_on_tick(battery, 600), kLowLink);
  EXPECT_EQ(link_on_tick(battery, 700), 268);
  EXPECT_EQ(link_on_tick(battery, 1000), 404);
  EXPECT_EQ(link_on_tick(battery, 1500), kPackVolts - 1);

  delete battery;
}

// If the BMS never moves we report the link rather than stall the close, and that has to latch. A late
// b1 edge dropping 0x441 back to 12V is its own plausibility fault.
TEST(BydAtto3PrechargeTests, ALateBmsResponseCannotReverseTheReportedLink) {
  byd_precharge_setup();
  auto battery = new BydAttoBattery();
  battery->setup();

  EXPECT_EQ(warmup(battery, 0x00, 0x00, 0), kLowLink);
  EXPECT_EQ(link_on_tick(battery, 3300), kLowLink);
  EXPECT_EQ(link_on_tick(battery, 3500), kPackVolts - 1);

  rx_at(battery, contactor_state_frame(0x00, 0x40), 3600);
  EXPECT_EQ(link_on_tick(battery, 3700), kPackVolts - 1);
  EXPECT_EQ(link_on_tick(battery, 4000), kPackVolts - 1);

  delete battery;
}
