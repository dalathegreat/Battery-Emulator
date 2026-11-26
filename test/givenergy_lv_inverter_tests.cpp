#include <gtest/gtest.h>

#include <cstring>
#include <deque>
#include <vector>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/devboard/utils/types.h"
#include "../Software/src/inverter/GIVENERGY-LV-FRAMES.h"
#include "../Software/src/inverter/GIVENERGY-LV-RS485.h"
#include "../Software/src/inverter/INVERTERS.h"

// Tests for the GivEnergy LV inverter module: how it frames requests out of
// the serial stream and when it replies. The wire format itself is covered by
// givenergy_lv_frames_tests.cpp.

namespace {

// A serial port the test feeds and reads back.
class FakeSerial : public HardwareSerial {
 public:
  std::deque<uint8_t> rx;
  std::vector<uint8_t> tx;

  int available() override { return static_cast<int>(rx.size()); }
  int read() override {
    if (rx.empty()) {
      return -1;
    }
    const uint8_t byte = rx.front();
    rx.pop_front();
    return byte;
  }
  size_t write(uint8_t byte) override {
    tx.push_back(byte);
    return 1;
  }
  size_t write(const uint8_t* buffer, size_t size) override {
    tx.insert(tx.end(), buffer, buffer + size);
    return size;
  }
  void feed(const std::vector<uint8_t>& bytes) { rx.insert(rx.end(), bytes.begin(), bytes.end()); }
};

std::vector<uint8_t> with_crc(std::vector<uint8_t> frame) {
  const uint16_t crc = givenergy_lv::crc16(frame.data(), frame.size());
  frame.push_back(crc & 0xFF);
  frame.push_back(crc >> 8);
  return frame;
}

const std::vector<uint8_t> kHrPoll = {0x01, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x44, 0x03};

class GivEnergyLvInverter : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    set_millis64(1000);
    // A real pack voltage, so update_values() marks the module ready to reply. Tests that care
    // about the not-ready state override this back to 0 before constructing the inverter.
    datalayer.battery.status.voltage_dV = 520;
    // A normal mid-charge cell voltage, below the taper (see ChargeTaper* tests below): without
    // this, DataLayer's cell_max_voltage_mV default of 3700 mV would taper HR26 to 0 in every
    // test here that doesn't set the aggregate cell voltages itself. cell_min_voltage_mV also
    // defaults to 3700 mV, which is above cell_max here and would leave every test with an
    // unrealistic inverted pack, so give it a matching normal value too.
    datalayer.aggregate.cell_max_voltage_mV = 3300;
    datalayer.aggregate.cell_min_voltage_mV = 3280;
    // The battery allows 80 A each way: a zero limit means the battery side has stopped that
    // direction, which HR20 reports (see *WhenTheBatteryAllowsNo* below).
    datalayer.aggregate.max_charge_current_dA = 800;
    datalayer.aggregate.max_discharge_current_dA = 800;
  }
};

}  // namespace

TEST_F(GivEnergyLvInverter, RepliesAfterTheTurnaround) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  port.feed(kHrPoll);
  inverter.receive();
  EXPECT_TRUE(port.tx.empty()) << "replied before the 75 ms turnaround the G3 needs";
  set_millis64(1075);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[0], 0x01);
  EXPECT_EQ(port.tx[1], 0x03);
  EXPECT_EQ(port.tx[2], 0x38);
}

TEST_F(GivEnergyLvInverter, FindsAPollAfterLineNoise) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  port.feed({0x00, 0xFF, 0x13, 0x01});
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(1075);
  inverter.receive();
  EXPECT_EQ(port.tx.size(), 61u);
}

TEST_F(GivEnergyLvInverter, StillWithholdsTheReply74MillisecondsIn) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(1074);
  inverter.receive();
  EXPECT_TRUE(port.tx.empty()) << "replied before the 75 ms turnaround the G3 needs";
  set_millis64(1075);
  inverter.receive();
  EXPECT_EQ(port.tx.size(), 61u);
}

TEST_F(GivEnergyLvInverter, AssemblesARequestSplitAcrossTwoReceiveCalls) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  port.feed({kHrPoll.begin(), kHrPoll.begin() + 3});
  inverter.receive();
  port.feed({kHrPoll.begin() + 3, kHrPoll.end()});
  inverter.receive();
  set_millis64(1075);
  inverter.receive();
  EXPECT_EQ(port.tx.size(), 61u);
}

TEST_F(GivEnergyLvInverter, IgnoresItsOwnWriteEchoButAnswersALaterRetry) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  const std::vector<uint8_t> write = with_crc({0x01, 0x06, 0x00, 0x02, 0x00, 0x01});
  port.feed(write);
  inverter.receive();
  set_millis64(1075);
  inverter.receive();
  ASSERT_EQ(port.tx, write);

  port.feed(write);  // the transceiver hearing our own reply
  set_millis64(1081);
  inverter.receive();
  set_millis64(1091);
  inverter.receive();
  EXPECT_EQ(port.tx.size(), 8u) << "answered its own echo";

  set_millis64(1300);  // the inverter genuinely writing again
  port.feed(write);
  inverter.receive();
  set_millis64(1375);
  inverter.receive();
  EXPECT_EQ(port.tx.size(), 16u);
}

TEST_F(GivEnergyLvInverter, IgnoresAnEchoThatArrivesInTheSameCallAsTheSend) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  const std::vector<uint8_t> write = with_crc({0x01, 0x06, 0x00, 0x02, 0x00, 0x01});
  port.feed(write);
  inverter.receive();

  set_millis64(1075);
  port.feed(write);  // the echo is already on the wire before we send our reply this call
  inverter.receive();

  set_millis64(1081);
  inverter.receive();
  set_millis64(1091);
  inverter.receive();
  EXPECT_EQ(port.tx.size(), 8u) << "answered its own echo";
}

TEST_F(GivEnergyLvInverter, ReplyUsesTheDatalayerFromTheLastUpdate) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.max_charge_current_dA = 500;  // 50.0 A
  inverter.update_values();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(1075);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[3 + 52], 0x13);  // HR26 = 5000 (50.00 A)
  EXPECT_EQ(port.tx[3 + 53], 0x88);
}

TEST_F(GivEnergyLvInverter, StaysSilentUntilTheBatteryHasReported) {
  FakeSerial port;
  datalayer.battery.status.voltage_dV = 0;  // the battery hasn't decoded a pack voltage yet
  GivEnergyLvRs485Inverter inverter(port);
  inverter.update_values();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(1075);
  inverter.receive();
  EXPECT_TRUE(port.tx.empty()) << "replied before the battery had reported a real voltage";

  datalayer.battery.status.voltage_dV = 520;
  datalayer.aggregate.voltage_dV = 520;
  inverter.update_values();
  port.feed(kHrPoll);
  set_millis64(1100);
  inverter.receive();
  set_millis64(1175);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[3 + 44], 0x14);  // HR22 = 5200 (52.00 V)
  EXPECT_EQ(port.tx[3 + 45], 0x50);
}

TEST_F(GivEnergyLvInverter, DiscardsRequestsHeardBeforeReady) {
  FakeSerial port;
  datalayer.battery.status.voltage_dV = 0;  // the battery hasn't decoded a pack voltage yet
  GivEnergyLvRs485Inverter inverter(port);
  port.feed(kHrPoll);
  inverter.receive();  // parsed and discarded while not ready: never queued as a pending reply

  datalayer.battery.status.voltage_dV = 520;
  inverter.update_values();
  set_millis64(1075);
  inverter.receive();
  set_millis64(1081);
  inverter.receive();
  EXPECT_TRUE(port.tx.empty()) << "answered a request that arrived before the battery was ready";
}

TEST(GivEnergyLvRegistration, IsListedByName) {
  EXPECT_STREQ(name_for_inverter_type(InverterProtocolType::GivEnergyLV485), GivEnergyLvRs485Inverter::Name);
}

TEST_F(GivEnergyLvInverter, LimitsGoToHr26ForChargeAndHr27ForDischarge) {
  datalayer.aggregate.max_charge_current_dA = 300;
  datalayer.aggregate.max_discharge_current_dA = 800;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.charge_limit_cA, 3000);
  EXPECT_EQ(s.discharge_limit_cA, 8000);
  EXPECT_EQ(s.limit_cA, 9000);
}

TEST_F(GivEnergyLvInverter, LimitsAreCappedAt90A) {
  datalayer.aggregate.max_charge_current_dA = 2000;  // a Seplos offering 200 A
  datalayer.aggregate.max_discharge_current_dA = 1500;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.charge_limit_cA, 9000);
  EXPECT_EQ(s.discharge_limit_cA, 9000);
}

TEST_F(GivEnergyLvInverter, ChargeTaperFollowsTheHighestCell) {
  datalayer.aggregate.max_charge_current_dA = 1000;  // 100 A, capped to 9000 before any taper
  datalayer.aggregate.cell_max_voltage_mV = 3400;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 9000);
  datalayer.aggregate.cell_max_voltage_mV = 3450;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 9000);
  datalayer.aggregate.cell_max_voltage_mV = 3500;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 4650);
  datalayer.aggregate.cell_max_voltage_mV = 3549;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 387);
  datalayer.aggregate.cell_max_voltage_mV = 3550;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 300);
  datalayer.aggregate.cell_max_voltage_mV = 3599;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 300);
  datalayer.aggregate.cell_max_voltage_mV = 3600;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 0);
  datalayer.aggregate.cell_max_voltage_mV = 3650;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().charge_limit_cA, 0);
}

TEST_F(GivEnergyLvInverter, ChargeTaperNeverRaisesTheBatteryLimit) {
  datalayer.aggregate.max_charge_current_dA = 20;  // 2.0 A: well under even the untapered limit
  datalayer.aggregate.cell_max_voltage_mV = 3500;  // deep in the taper band, but that's moot here
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.charge_limit_cA, 200);
}

TEST_F(GivEnergyLvInverter, ChargeTaperLeavesDischargeAlone) {
  datalayer.aggregate.cell_max_voltage_mV = 3600;  // taper would zero HR26, but not HR27
  datalayer.aggregate.max_discharge_current_dA = 800;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.discharge_limit_cA, 8000);
}

TEST_F(GivEnergyLvInverter, FaultStopsChargeAndDischarge) {
  datalayer.aggregate.max_charge_current_dA = 500;
  datalayer.aggregate.max_discharge_current_dA = 500;
  datalayer.system.status.system_status = FAULT;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.charge_limit_cA, 0);
  EXPECT_EQ(s.discharge_limit_cA, 0);
  EXPECT_EQ(s.limit_cA, 0);
}

TEST_F(GivEnergyLvInverter, OverVoltageAlarmOnFault) {
  datalayer.system.status.system_status = FAULT;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.alarms, 0x000C);
}

TEST_F(GivEnergyLvInverter, OverVoltageAlarmAtTheCellStop) {
  datalayer.aggregate.cell_max_voltage_mV = 3599;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0004, 0u);
  datalayer.aggregate.cell_max_voltage_mV = 3600;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0004, 0x0004u);
}

TEST_F(GivEnergyLvInverter, OverVoltageAlarmAtTheUserCeiling) {
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 564;
  datalayer.aggregate.voltage_dV = 563;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0004, 0u);
  datalayer.aggregate.voltage_dV = 564;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0004, 0x0004u);

  datalayer.battery_settings.user_set_voltage_limits_active = false;
  datalayer.aggregate.voltage_dV = 570;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0004, 0u);
}

TEST_F(GivEnergyLvInverter, UnderVoltageAlarmAtTheLowCellStop) {
  datalayer.aggregate.cell_min_voltage_mV = 2901;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0008, 0u);
  datalayer.aggregate.cell_min_voltage_mV = 2900;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms & 0x0008, 0x0008u);
}

// The battery's own BMS (or Battery-Emulator's safety layer acting on its voltage limit, the user
// ceiling or SoC) stops charging by zeroing the charge limit. HR26 = 0 alone leaves the DSP a
// ~1 A charge bound until its 30 s "battery full" block, so report it as full (HR20 bit 2) too.
TEST_F(GivEnergyLvInverter, FullAlarmWhenTheBatteryAllowsNoCharge) {
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms, 0u);
  datalayer.aggregate.max_charge_current_dA = 0;
  const auto s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.charge_limit_cA, 0);
  EXPECT_EQ(s.alarms, 0x0004u);
}

// Likewise a zero discharge limit from the battery side is reported as empty (HR20 bit 3).
TEST_F(GivEnergyLvInverter, EmptyAlarmWhenTheBatteryAllowsNoDischarge) {
  datalayer.aggregate.max_discharge_current_dA = 0;
  const auto s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.discharge_limit_cA, 0);
  EXPECT_EQ(s.alarms, 0x0008u);
}

// A limit that is low but not zero (the battery tapering, not stopping) raises nothing.
TEST_F(GivEnergyLvInverter, NoAlarmWhileTheBatteryOnlyReducesItsLimits) {
  datalayer.aggregate.max_charge_current_dA = 30;  // 3 A
  datalayer.aggregate.max_discharge_current_dA = 30;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms, 0u);
}

TEST_F(GivEnergyLvInverter, NoAlarmsInNormalRunning) {
  datalayer.aggregate.cell_max_voltage_mV = 3300;
  datalayer.aggregate.cell_min_voltage_mV = 3280;
  datalayer.aggregate.voltage_dV = 530;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().alarms, 0u);
}

TEST_F(GivEnergyLvInverter, FaultAlarmBitsAppearOnTheWire) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.system.status.system_status = FAULT;
  inverter.update_values();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(1075);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[3 + 40], 0x00);  // HR20 = 0x000C: over- and under-voltage alarms
  EXPECT_EQ(port.tx[3 + 41], 0x0C);
}

TEST_F(GivEnergyLvInverter, StatusBitsFollowTheCurrent) {
  datalayer.aggregate.current_dA = -1;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().status, 0x00CF);
  datalayer.aggregate.current_dA = 50;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().status, 0x00CE);
  datalayer.aggregate.current_dA = 0;
  EXPECT_EQ(GivEnergyLvRs485Inverter::snapshot_from_datalayer().status, 0x00CD);
}

TEST_F(GivEnergyLvInverter, UnitsMatchTheWire) {
  datalayer.aggregate.voltage_dV = 562;
  datalayer.aggregate.current_dA = -123;
  datalayer.aggregate.reported_soc = 9700;
  datalayer.aggregate.reported_total_capacity_Wh = 32000;  // the Fogstar
  datalayer.aggregate.reported_remaining_capacity_Wh = 16000;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.voltage_cV, 5620);
  EXPECT_EQ(s.pack_voltage_mV, 56200);
  EXPECT_EQ(s.current_cA, -1230);
  EXPECT_EQ(s.current_mA, -12300);
  EXPECT_EQ(s.soc_pct, 97);
  EXPECT_EQ(s.block2_soc_pct, 97);
  // The Growatt battery module computes Wh = Ah x measured pack voltage, so converting back
  // uses that same measured voltage (562 dV = 56.2 V), not a fixed nominal one: 32000 Wh * 1000
  // / 562 dV = 56939 cAh.
  EXPECT_EQ(s.capacity_Ah, 569);
  EXPECT_EQ(s.full_capacity_cAh, 56939);
  EXPECT_EQ(s.design_capacity_cAh, 56939);
  EXPECT_EQ(s.remaining_cAh, 28469);
  EXPECT_EQ(s.firmware, 3022);
  EXPECT_STREQ(s.serial, "EM2026G001");

  // A round-numbers pack (33750 Wh at 54.0 V) so the expected cAh is exact.
  datalayer.aggregate.voltage_dV = 540;
  datalayer.aggregate.reported_total_capacity_Wh = 33750;
  const givenergy_lv::Snapshot s2 = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s2.full_capacity_cAh, 62500);
  EXPECT_EQ(s2.capacity_Ah, 625);
}

TEST_F(GivEnergyLvInverter, FullCapacityNoLongerClampsAt16Bits) {
  datalayer.aggregate.voltage_dV = 512;
  datalayer.aggregate.reported_total_capacity_Wh = 64000;  // 625 Ah at 51.2 V: two Fogstar packs
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.capacity_Ah, 1250);
  EXPECT_EQ(s.full_capacity_cAh, 125000u);
}

TEST_F(GivEnergyLvInverter, SubZeroTemperaturesStayNegative) {
  datalayer.aggregate.temperature_max_dC = -30;
  datalayer.aggregate.temperature_min_dC = -52;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.temperature_C, -3);
  const int16_t expected[5] = {-30, -52, -52, -30, -41};
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(s.sensor_temps_dC[i], expected[i]) << "sensor " << i;
  }
  EXPECT_EQ(s.temp_max_dC, -30);
  EXPECT_EQ(s.temp_min_dC, -52);
}

TEST_F(GivEnergyLvInverter, ReportedCellsPassThrough) {
  for (int i = 0; i < 16; i++) {
    datalayer.battery.status.cell_voltages_mV[i] = 3300 + i;
  }
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.cells_mV[0], 3300);
  EXPECT_EQ(s.cells_mV[15], 3315);
  EXPECT_EQ(s.cell_sum_mV, 16 * 3300 + 120);
}

TEST_F(GivEnergyLvInverter, MissingCellsAreFilledFromMinAndMax) {
  datalayer.aggregate.cell_max_voltage_mV = 3400;
  datalayer.aggregate.cell_min_voltage_mV = 3300;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  EXPECT_EQ(s.cells_mV[0], 3400);
  EXPECT_EQ(s.cells_mV[1], 3300);
  for (int i = 2; i < 16; i++) {
    EXPECT_EQ(s.cells_mV[i], 3350) << "cell " << i;
  }
  EXPECT_EQ(s.cell_sum_mV, 3400 + 3300 + 14 * 3350);
}

TEST_F(GivEnergyLvInverter, NoBatteryYetStillGivesInRangeValues) {
  datalayer.aggregate.cell_max_voltage_mV = 0;
  datalayer.aggregate.cell_min_voltage_mV = 0;
  datalayer.aggregate.max_charge_current_dA = 0;  // no battery limits yet either
  datalayer.aggregate.max_discharge_current_dA = 0;
  const givenergy_lv::Snapshot s = GivEnergyLvRs485Inverter::snapshot_from_datalayer();
  for (int i = 0; i < 16; i++) {
    EXPECT_GT(s.cells_mV[i], 2200) << "cell " << i;
    EXPECT_LT(s.cells_mV[i], 3700) << "cell " << i;
  }
  EXPECT_EQ(s.charge_limit_cA, 0);
  EXPECT_EQ(s.discharge_limit_cA, 0);
  EXPECT_EQ(s.cell_count, 16);
}

TEST(GivEnergyLvEvents, LimitsIgnoredIsAWarningAndItsLockoutAnError) {
  init_events();
  reset_all_events();
  EXPECT_STREQ(get_event_level_string(EVENT_INVERTER_LIMITS_IGNORED), "WARNING");
  EXPECT_STREQ(get_event_level_string(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT), "ERROR");
  EXPECT_NE(strstr(get_event_message_string(EVENT_INVERTER_LIMITS_IGNORED).c_str(), "calibration"), nullptr);
  EXPECT_NE(strstr(get_event_message_string(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT).c_str(), "HR29"), nullptr);
}

TEST(GivEnergyLvEvents, TheLockoutPutsTheEmulatorIntoFault) {
  datalayer = DataLayer();
  init_events();
  reset_all_events();
  set_event_latched(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT, 0);
  EXPECT_EQ(datalayer.system.status.system_status, FAULT);
  reset_all_events();
}

// ---- Charge taper latch ----
//
// A replay of a real GivEnergy battery's charge showed its BMS cut HR26 to 3.20 A at the LFP
// knee and held it there through 100%, releasing only once discharge had started. These check
// that update_values() latches the taper the same way, and releases it again once the highest
// cell falls below kTrickleRelease_mV.

namespace {

class GivEnergyLvTaperLatch : public GivEnergyLvInverter {
 protected:
  void SetUp() override {
    GivEnergyLvInverter::SetUp();
    init_events();
    reset_all_events();
    datalayer.aggregate.max_charge_current_dA = 800;  // 80 A: well above the trickle
  }
  void TearDown() override { reset_all_events(); }
};

}  // namespace

TEST_F(GivEnergyLvTaperLatch, LatchesAtTheTrickleAndReleasesOffTheTop) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.cell_max_voltage_mV = 3550;  // the taper reaches the trickle: latch
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 300);

  datalayer.aggregate.cell_max_voltage_mV = 3460;  // relaxes; unlatched, the taper alone would
                                                   // already give 8130, above the 80 A battery
                                                   // limit that would otherwise cap it to 8000
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 300) << "should still be latched to the trickle";

  datalayer.aggregate.cell_max_voltage_mV = 3410;  // still at or above the release threshold
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 300);

  datalayer.aggregate.cell_max_voltage_mV = 3390;  // below kTrickleRelease_mV: released
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 8000);  // back to the battery limit
}

TEST_F(GivEnergyLvTaperLatch, DoesNotLatchBeforeTheTrickle) {
  FakeSerial port;
  datalayer.aggregate.max_charge_current_dA = 1000;  // 100 A, capped to 9000 before any taper
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.cell_max_voltage_mV = 3520;  // short of kTaperEnd_mV: no latch yet
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 2910);

  datalayer.aggregate.cell_max_voltage_mV = 3460;  // relaxes further, never having reached it
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 8130) << "latched without ever reaching the trickle";
}

TEST_F(GivEnergyLvTaperLatch, StopStillWinsWhileLatched) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.cell_max_voltage_mV = 3550;  // engage the latch
  inverter.update_values();
  ASSERT_EQ(inverter.snapshot().charge_limit_cA, 300);

  datalayer.aggregate.cell_max_voltage_mV = 3600;  // the cell stop still wins over the latch
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 0);
}

TEST_F(GivEnergyLvTaperLatch, BatteryLimitLowerThanTheTrickleStillWins) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.cell_max_voltage_mV = 3550;  // engage the latch
  inverter.update_values();
  ASSERT_EQ(inverter.snapshot().charge_limit_cA, 300);

  datalayer.aggregate.max_charge_current_dA = 20;  // 2.0 A: below even the trickle
  inverter.update_values();                        // still latched, the cell hasn't moved
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 200);
}

TEST_F(GivEnergyLvTaperLatch, HoldsTheLatchStateWhileDataIsStale) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.cell_max_voltage_mV = 3550;  // engage the latch
  inverter.update_values();
  ASSERT_EQ(inverter.snapshot().charge_limit_cA, 300);

  datalayer.battery.status.CAN_battery_still_alive = 0;  // stale, before safety.cpp raises the event
  datalayer.aggregate.cell_max_voltage_mV = 3390;        // would release the latch, but it's stale
  inverter.update_values();
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 300) << "advanced the latch on stale data";

  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  // live again
  inverter.update_values();  // the cell is already below the release threshold
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 8000);
}

TEST_F(GivEnergyLvTaperLatch, WireReplyCarriesTheLatchedTrickle) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.cell_max_voltage_mV = 3550;  // engage the latch
  inverter.update_values();
  datalayer.aggregate.cell_max_voltage_mV = 3460;  // relaxed, but still latched
  inverter.update_values();

  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(millis64() + 75);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[3 + 52], 0x01);  // HR26 = 300 (3.00 A), not the unlatched ~8130
  EXPECT_EQ(port.tx[3 + 53], 0x2C);
}

TEST_F(GivEnergyLvTaperLatch, AFreshInstanceStartsUnlatched) {
  FakeSerial port1;
  datalayer.aggregate.max_charge_current_dA = 1000;  // 100 A, capped to 9000 before any taper
  {
    GivEnergyLvRs485Inverter inverter(port1);
    datalayer.aggregate.cell_max_voltage_mV = 3550;  // engage the latch
    inverter.update_values();
    datalayer.aggregate.cell_max_voltage_mV = 3460;  // relaxed, but still latched
    inverter.update_values();
    ASSERT_EQ(inverter.snapshot().charge_limit_cA, 300);
  }

  // A restart: a fresh instance sees the same relaxed cell with no memory of the old latch.
  FakeSerial port2;
  GivEnergyLvRs485Inverter fresh(port2);
  fresh.update_values();  // cell_max_voltage_mV is still 3460 from above
  EXPECT_EQ(fresh.snapshot().charge_limit_cA, 8130) << "a restart should not inherit the latch";
}

// ---- Calibration guard ----
//
// During a GivEnergy battery calibration the inverter holds both current limits at 8 A or more,
// whatever HR26 and HR27 say. The guard spots that from the current, ends the calibration by
// reporting HR20 "empty" then "full", and stops answering if the inverter still won't obey.

namespace {

using Guard = GivEnergyLvRs485Inverter::Guard;

class GivEnergyLvGuard : public GivEnergyLvInverter {
 protected:
  void SetUp() override {
    GivEnergyLvInverter::SetUp();
    init_events();
    reset_all_events();
    datalayer.aggregate.voltage_dV = 520;
    datalayer.aggregate.reported_soc = 5000;
    datalayer.aggregate.max_charge_current_dA = 800;  // 80 A
    datalayer.aggregate.max_discharge_current_dA = 800;
  }
  void TearDown() override { reset_all_events(); }

  // One core_loop second at a time: the inverter polls (and gets its reply, if any) near the
  // end of the second, then update_values() runs. The polls keep EVENT_MODBUS_INVERTER_MISSING,
  // an ERROR, from putting the emulator into FAULT.
  static void run(GivEnergyLvRs485Inverter& inverter, FakeSerial& port, int seconds) {
    for (int i = 0; i < seconds; i++) {
      const uint64_t start = millis64();
      set_millis64(start + 900);
      port.feed(kHrPoll);
      inverter.receive();
      set_millis64(start + 1000);
      inverter.receive();
      inverter.update_values();
    }
  }

  static const EVENTS_STRUCT_TYPE& event(EVENTS_ENUM_TYPE e) { return *get_event_pointer(e); }

  // Trips trigger A (5 A advertised, 8 A flowing) and returns once the guard has.
  static void trip(GivEnergyLvRs485Inverter& inverter, FakeSerial& port) {
    datalayer.aggregate.max_charge_current_dA = 50;
    datalayer.aggregate.current_dA = 80;
    run(inverter, port, 1);   // HR26 goes to 5 A; the inverter has obeyed the old 80 A until now
    run(inverter, port, 61);  // the condition holds from the first of these to the last
    ASSERT_EQ(inverter.guard(), Guard::EndingBoth);
  }
};

}  // namespace

TEST_F(GivEnergyLvGuard, TripsWhenChargeIgnoresALowHr26) {
  FakeSerial port;
  datalayer.aggregate.max_charge_current_dA = 50;  // HR26 = 5 A
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 71;  // 7.1 A: more than 2 A over
  inverter.update_values();             // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);               // the condition starts here
  run(inverter, port, 59);
  EXPECT_EQ(inverter.guard(), Guard::Normal) << "tripped a second early";
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).state, EVENT_STATE_INACTIVE);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).state, EVENT_STATE_ACTIVE);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 1);  // trigger A
}

TEST_F(GivEnergyLvGuard, AChargeDipRestartsTheWindow) {
  FakeSerial port;
  datalayer.aggregate.max_charge_current_dA = 50;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 80;
  inverter.update_values();  // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);    // the condition starts here
  run(inverter, port, 40);
  datalayer.aggregate.current_dA = 70;  // exactly 2 A over: not more than 2 A
  run(inverter, port, 1);
  datalayer.aggregate.current_dA = 80;
  run(inverter, port, 60);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

TEST_F(GivEnergyLvGuard, TripsWhenDischargeIgnoresALowHr27) {
  FakeSerial port;
  datalayer.aggregate.max_discharge_current_dA = 0;  // HR27 = 0
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -80;  // 8 A discharge: the calibration's 8 A floor
  inverter.update_values();              // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);                // the condition starts here
  run(inverter, port, 59);
  EXPECT_EQ(inverter.guard(), Guard::Normal) << "tripped a second early";
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 2);  // trigger B
}

TEST_F(GivEnergyLvGuard, ADischargeDipRestartsTheWindow) {
  FakeSerial port;
  datalayer.aggregate.max_discharge_current_dA = 0;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -80;
  inverter.update_values();  // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);    // the condition starts here
  run(inverter, port, 30);
  datalayer.aggregate.current_dA = -40;  // 4 A: the inverter's own 2 A floor plus the 2 A margin
  run(inverter, port, 1);
  datalayer.aggregate.current_dA = -80;
  run(inverter, port, 60);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

TEST_F(GivEnergyLvGuard, TripsWhenDischargeCarriesOnBelowTheFloor) {
  FakeSerial port;
  datalayer.aggregate.reported_soc = 200;  // 2%: below any reserve the inverter allows
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -51;  // 5.1 A discharge, well inside HR27 = 80 A
  inverter.update_values();              // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);                // the condition starts here
  run(inverter, port, 59);
  EXPECT_EQ(inverter.guard(), Guard::Normal) << "tripped a second early";
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 3);  // trigger C
}

TEST_F(GivEnergyLvGuard, AFloorDipRestartsTheWindow) {
  FakeSerial port;
  datalayer.aggregate.reported_soc = 100;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -300;
  inverter.update_values();  // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);    // the condition starts here
  run(inverter, port, 50);
  datalayer.aggregate.current_dA = -50;  // 5 A: not over 5 A
  run(inverter, port, 1);
  datalayer.aggregate.current_dA = -300;
  run(inverter, port, 60);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

TEST_F(GivEnergyLvGuard, TheFloorTriggerNeedsTwoPercentOrLess) {
  FakeSerial port;
  datalayer.aggregate.reported_soc = 300;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -300;
  inverter.update_values();  // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);    // the condition starts here
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, TripsWhenChargeCarriesOnAfterFull) {
  FakeSerial port;
  // The user's charge-voltage ceiling raises HR20 bit 2 without touching HR26, so only
  // trigger D can see this one.
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 540;
  datalayer.aggregate.voltage_dV = 540;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 21;  // 2.1 A: about the 180 W a calibration lets through
  inverter.update_values();             // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);               // the condition starts here
  run(inverter, port, 29);
  EXPECT_EQ(inverter.guard(), Guard::Normal) << "tripped a second early";
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 4);  // trigger D
}

TEST_F(GivEnergyLvGuard, AnAfterFullDipRestartsTheWindow) {
  FakeSerial port;
  datalayer.battery_settings.user_set_voltage_limits_active = true;
  datalayer.battery_settings.max_user_set_charge_voltage_dV = 540;
  datalayer.aggregate.voltage_dV = 540;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 35;
  inverter.update_values();  // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);    // the condition starts here
  run(inverter, port, 20);
  datalayer.aggregate.current_dA = 20;  // 2 A: not over 2 A
  run(inverter, port, 1);
  datalayer.aggregate.current_dA = 35;
  run(inverter, port, 30);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

TEST_F(GivEnergyLvGuard, CountsOnlyOnceTheBatteryHasReported) {
  FakeSerial port;
  datalayer.battery.status.voltage_dV = 0;
  datalayer.aggregate.max_charge_current_dA = 50;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 80;
  inverter.update_values();  // ready: the guard skips this one, nothing was advertised before it
  run(inverter, port, 1);    // the condition starts here
  run(inverter, port, 120);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  datalayer.battery.status.voltage_dV = 520;
  run(inverter, port, 1);  // ready: skipped, since what went before was never advertised
  run(inverter, port, 60);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

TEST_F(GivEnergyLvGuard, EndsTheCalibrationWithEmptyThenFull) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  datalayer.aggregate.current_dA = 0;  // the inverter backs off
  for (int s = 0; s < 10; s++) {
    SCOPED_TRACE(s);
    EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
    EXPECT_EQ(inverter.snapshot().alarms, 0x000C);
    EXPECT_EQ(inverter.snapshot().charge_limit_cA, 0);
    EXPECT_EQ(inverter.snapshot().discharge_limit_cA, 0);
    run(inverter, port, 1);
  }
  for (int s = 10; s < 20; s++) {
    SCOPED_TRACE(s);
    EXPECT_EQ(inverter.guard(), Guard::EndingFull);
    EXPECT_EQ(inverter.snapshot().alarms, 0x0004);
    EXPECT_EQ(inverter.snapshot().charge_limit_cA, 0);
    EXPECT_EQ(inverter.snapshot().discharge_limit_cA, 0);
    run(inverter, port, 1);
  }
  EXPECT_EQ(inverter.guard(), Guard::Verify);
  EXPECT_EQ(inverter.snapshot().alarms, 0u);
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 500);
  EXPECT_EQ(inverter.snapshot().discharge_limit_cA, 8000);
}

TEST_F(GivEnergyLvGuard, TheEndingBitsGoOutOnTheWire) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  port.tx.clear();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(millis64() + 75);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[3 + 40], 0x00);  // HR20 = 0x000C: empty and full
  EXPECT_EQ(port.tx[3 + 41], 0x0C);
  EXPECT_EQ(port.tx[3 + 52], 0x00);  // HR26 = 0
  EXPECT_EQ(port.tx[3 + 53], 0x00);
  EXPECT_EQ(port.tx[3 + 54], 0x00);  // HR27 = 0
  EXPECT_EQ(port.tx[3 + 55], 0x00);

  run(inverter, port, 10);
  port.tx.clear();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(millis64() + 75);
  inverter.receive();
  ASSERT_EQ(port.tx.size(), 61u);
  EXPECT_EQ(port.tx[3 + 41], 0x04);  // HR20 = 0x0004: full alone
}

TEST_F(GivEnergyLvGuard, GoesBackToNormalWhenTheInverterObeys) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 20);
  ASSERT_EQ(inverter.guard(), Guard::Verify);
  datalayer.aggregate.current_dA = 60;  // 6 A against HR26 = 5 A: within the 2 A margin
  run(inverter, port, 59);
  EXPECT_EQ(inverter.guard(), Guard::Verify);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).state, EVENT_STATE_ACTIVE);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT).state, EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.system.status.system_status, ACTIVE);
}

TEST_F(GivEnergyLvGuard, LocksOutWhenTheInverterKeepsOverriding) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  run(inverter, port, 20);  // the current stays at 8 A against HR26 = 5 A throughout
  ASSERT_EQ(inverter.guard(), Guard::Verify);
  run(inverter, port, 20);
  EXPECT_EQ(inverter.guard(), Guard::Verify) << "locked out a second early";
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::Lockout);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT).state, EVENT_STATE_ACTIVE_LATCHED);
  EXPECT_EQ(datalayer.system.status.system_status, FAULT);

  port.tx.clear();
  port.feed(kHrPoll);
  inverter.receive();
  set_millis64(millis64() + 75);
  inverter.receive();
  EXPECT_TRUE(port.tx.empty()) << "answered the inverter while locked out";
  EXPECT_TRUE(port.rx.empty()) << "left the poll in the receive buffer";

  datalayer.aggregate.current_dA = 0;  // the inverter gives up on the battery, but keeps polling
  run(inverter, port, 3600);
  EXPECT_EQ(inverter.guard(), Guard::Lockout);
  EXPECT_TRUE(port.tx.empty()) << "the lockout didn't latch";
}

TEST_F(GivEnergyLvGuard, LockoutStopsAReplyAlreadyQueued) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  run(inverter, port, 40);
  ASSERT_EQ(inverter.guard(), Guard::Verify);
  port.tx.clear();
  set_millis64(millis64() + 999);
  port.feed(kHrPoll);
  inverter.receive();  // queued, to go out after the turnaround
  set_millis64(millis64() + 1);
  inverter.update_values();
  ASSERT_EQ(inverter.guard(), Guard::Lockout);
  set_millis64(millis64() + 80);  // past the turnaround, when the queued reply would have gone
  inverter.receive();
  EXPECT_TRUE(port.tx.empty()) << "sent a reply queued before the lockout";
}

TEST_F(GivEnergyLvGuard, NoTripOnTheTrickleCharge) {
  FakeSerial port;
  datalayer.aggregate.cell_max_voltage_mV = 3560;  // taper at 3 A
  GivEnergyLvRs485Inverter inverter(port);
  ASSERT_EQ(inverter.snapshot().charge_limit_cA, 300);
  datalayer.aggregate.current_dA = 30;
  inverter.update_values();
  run(inverter, port, 600);
  datalayer.aggregate.current_dA = 50;  // 5 A: still within the 2 A margin
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, NoTripAtTheCeilingWhileTheChargeDecays) {
  FakeSerial port;
  datalayer.aggregate.cell_max_voltage_mV = 3600;  // HR26 = 0 and HR20 bit 2
  GivEnergyLvRs485Inverter inverter(port);
  ASSERT_EQ(inverter.snapshot().charge_limit_cA, 0);
  ASSERT_EQ(inverter.snapshot().alarms & 0x0004, 0x0004);
  for (int s = 0; s <= 20; s++) {
    datalayer.aggregate.current_dA = static_cast<int16_t>(300 - 15 * s);  // 30 A down to 0 in 20 s
    run(inverter, port, 1);
  }
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, NoTripOnADischargeStoppedAtTheReserve) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.reported_soc = 500;
  datalayer.aggregate.current_dA = -200;
  run(inverter, port, 600);
  datalayer.aggregate.reported_soc = 400;
  run(inverter, port, 5);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, NoTripOnAHeavyDischargeWithinHr27) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -700;
  run(inverter, port, 3600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

// The battery zeroing its charge limit now raises HR20 bit 2, which arms trigger D. An inverter
// that obeys, with the charge current decaying to nothing inside 20 s, must not trip it.
TEST_F(GivEnergyLvGuard, NoTripWhenTheBatteryStopsChargeAndTheChargeDecays) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 500;
  run(inverter, port, 5);
  datalayer.aggregate.max_charge_current_dA = 0;  // the BMS stops charging
  for (int amps_dA : {400, 300, 200, 100, 50, 20, 0}) {
    datalayer.aggregate.current_dA = amps_dA;
    run(inverter, port, 3);
  }
  run(inverter, port, 60);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, NoTripOnDischargeWhileReportingFull) {
  FakeSerial port;
  datalayer.aggregate.cell_max_voltage_mV = 3600;  // HR20 bit 2, as a real battery sent it
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -200;
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, AnOverrideLateInTheCheckStillGetsItsWindow) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 20);
  ASSERT_EQ(inverter.guard(), Guard::Verify);
  run(inverter, port, 50);
  datalayer.aggregate.current_dA = 80;  // back over HR26 = 5 A from 51 s into the check
  run(inverter, port, 20);
  EXPECT_EQ(inverter.guard(), Guard::Verify) << "gave the all-clear with a condition still counting";
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::Lockout);
}

TEST_F(GivEnergyLvGuard, NoTripOnTheInvertersOwnDischargeFloor) {
  FakeSerial port;
  datalayer.aggregate.max_discharge_current_dA = 0;  // HR27 = 0 still lets ~2 A through
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = -23;
  run(inverter, port, 3600);
  datalayer.aggregate.current_dA = -40;  // 4 A: not over max(HR27, 2 A) + 2 A
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  datalayer.aggregate.current_dA = -41;
  run(inverter, port, 61);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

// A zero charge limit from the battery side now always comes with HR20 bit 2, and with bit 2 the
// DSP drops its charge bound to 0 at once (HR26 = 0 alone would still let ~1 A through). So up
// to 2 A is tolerated indefinitely, but more than that for 30 s is the inverter ignoring "full":
// trigger D, before trigger A's 60 s.
TEST_F(GivEnergyLvGuard, AZeroChargeLimitIsReportedAsFullForTheGuard) {
  FakeSerial port;
  datalayer.aggregate.max_charge_current_dA = 0;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 20;  // 2.0 A: not over the 2 A margin
  run(inverter, port, 3600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  datalayer.aggregate.current_dA = 21;
  run(inverter, port, 30);
  EXPECT_EQ(inverter.guard(), Guard::Normal) << "tripped a second early";
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 4);  // trigger D
}

TEST_F(GivEnergyLvGuard, AfterFullWinsOverTheChargeLimitAtTheCeiling) {
  FakeSerial port;
  datalayer.aggregate.cell_max_voltage_mV = 3600;  // HR26 = 0 and HR20 bit 2: A and D both hold
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 35;
  inverter.update_values();
  run(inverter, port, 1);
  run(inverter, port, 29);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 4);  // D, at 30 s
}

TEST_F(GivEnergyLvGuard, IgnoresStaleCurrentWhileTheBatteryIsMissing) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  datalayer.aggregate.current_dA = 300;  // the last frame before the pack went quiet: 30 A charge
  run(inverter, port, 5);
  // The safety layer's view 60 s later: the counter has run out and the missing event (an
  // ERROR) has put the emulator into FAULT, so HR26 = HR27 = 0 and HR20 bits 2 and 3 go out.
  datalayer.battery.status.CAN_battery_still_alive = 0;
  set_event(EVENT_CAN_BATTERY_MISSING, 0);
  ASSERT_EQ(datalayer.system.status.system_status, FAULT);
  run(inverter, port, 3600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).state, EVENT_STATE_INACTIVE);

  // The pack comes back: nothing carried over from while it was missing.
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  clear_event(EVENT_CAN_BATTERY_MISSING);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, IgnoresStaleCurrentOnceTheCounterRunsOut) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  set_event(EVENT_DUMMY_ERROR, 0);                       // FAULT: HR26 = 0 and HR20 bit 2, as trigger D wants
  datalayer.battery.status.CAN_battery_still_alive = 0;  // stale, before safety.cpp raises the event
  datalayer.aggregate.current_dA = 300;
  run(inverter, port, 600);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}

TEST_F(GivEnergyLvGuard, AFaultDoesNotHideACalibration) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  set_event(EVENT_DUMMY_ERROR, 0);  // some other fault: HR26 = HR27 = 0, HR20 bits 2 and 3
  ASSERT_EQ(datalayer.system.status.system_status, FAULT);
  datalayer.aggregate.current_dA = 30;  // the ~180 W a calibration still lets through
  inverter.update_values();
  run(inverter, port, 1);
  run(inverter, port, 29);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1);
  ASSERT_EQ(inverter.guard(), Guard::EndingBoth);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).data, 4);
  EXPECT_EQ(inverter.snapshot().alarms, 0x000C);
  run(inverter, port, 10);
  ASSERT_EQ(inverter.guard(), Guard::EndingFull);
  EXPECT_EQ(inverter.snapshot().alarms, 0x0004) << "the fault's bit 3 would stop the calibration reaching full";
}

TEST_F(GivEnergyLvGuard, ALockoutSurvivesClearingTheEvents) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  run(inverter, port, 41);
  ASSERT_EQ(inverter.guard(), Guard::Lockout);
  reset_all_events();  // "Clear events" on the web page
  run(inverter, port, 1);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT).state, EVENT_STATE_ACTIVE_LATCHED);
  EXPECT_EQ(datalayer.system.status.system_status, FAULT);
  run(inverter, port, 60);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT).occurences, 1u);
}

TEST_F(GivEnergyLvGuard, ARepeatWithinTheHourLocksOut) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 80);
  ASSERT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 1800);
  datalayer.aggregate.current_dA = 80;  // at it again
  run(inverter, port, 61);
  EXPECT_EQ(inverter.guard(), Guard::Lockout);
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT).state, EVENT_STATE_ACTIVE_LATCHED);
}

TEST_F(GivEnergyLvGuard, ARepeatAfterTheHourEndsItAgain) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 80);
  ASSERT_EQ(inverter.guard(), Guard::Normal);
  run(inverter, port, 3600);
  datalayer.aggregate.current_dA = 80;
  run(inverter, port, 61);
  EXPECT_EQ(inverter.guard(), Guard::EndingBoth);
}

TEST_F(GivEnergyLvGuard, LockoutZeroesTheLimitsEvenWithoutFault) {
  FakeSerial port;
  datalayer.battery_settings.user_requests_forced_charging_recovery_mode = true;  // holds ACTIVE
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  run(inverter, port, 41);
  ASSERT_EQ(inverter.guard(), Guard::Lockout);
  ASSERT_EQ(datalayer.system.status.system_status, ACTIVE);
  EXPECT_EQ(inverter.snapshot().limit_cA, 0);
  EXPECT_EQ(inverter.snapshot().charge_limit_cA, 0);
  EXPECT_EQ(inverter.snapshot().discharge_limit_cA, 0);
  EXPECT_EQ(inverter.snapshot().alarms, 0x000C);
}

TEST_F(GivEnergyLvGuard, VerifyWaitsForLiveBatteryData) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  run(inverter, port, 20);  // the current stays at 8 A against HR26 = 5 A throughout
  ASSERT_EQ(inverter.guard(), Guard::Verify);
  datalayer.battery.status.CAN_battery_still_alive = 0;
  set_event(EVENT_CAN_BATTERY_MISSING, 0);
  run(inverter, port, 90);
  EXPECT_EQ(inverter.guard(), Guard::Verify) << "gave the all-clear with nothing measured";
  EXPECT_EQ(event(EVENT_INVERTER_LIMITS_IGNORED).state, EVENT_STATE_ACTIVE);

  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  clear_event(EVENT_CAN_BATTERY_MISSING);
  run(inverter, port, 20);
  EXPECT_EQ(inverter.guard(), Guard::Verify);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::Lockout);
}

TEST_F(GivEnergyLvGuard, VerifyMeasuresAFullMinuteOnceTheDataIsBack) {
  FakeSerial port;
  GivEnergyLvRs485Inverter inverter(port);
  trip(inverter, port);
  datalayer.aggregate.current_dA = 0;
  run(inverter, port, 20);
  ASSERT_EQ(inverter.guard(), Guard::Verify);
  datalayer.battery.status.CAN_battery_still_alive = 0;
  run(inverter, port, 90);
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  run(inverter, port, 59);
  EXPECT_EQ(inverter.guard(), Guard::Verify);
  run(inverter, port, 1);
  EXPECT_EQ(inverter.guard(), Guard::Normal);
}
