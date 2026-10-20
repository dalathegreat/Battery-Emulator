#include <gtest/gtest.h>

#include "../../Software/src/battery/TESLA-LEGACY-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../../Software/src/devboard/utils/types.h"

// Host tests for the 2012-2020 Model S/X driver (TeslaLegacyBattery).
//
// A test sends 0x7E2 the way the CAN task does, then calls update_values().
// The only CAC this driver stores is the minimum, and only from mux 0, in
// 0.1 Ah. Maximum and average are not decoded. The stored minimum is
// private, so the check is the SOH the rest of the firmware reads:
// raw * 10000 / 2316. Raw 2316 is a new pack, 231.6 Ah.
//
// With no frame the minimum stays at its power-on value, which is that same
// new-pack number, so SOH reads 100.00%. update_values() writes SOH before
// it checks isolation. Isolation starts at 0 and raises its own event on
// this pass; these tests leave that event alone and read SOH anyway.
//
// The pack is local and is not stored in the global battery pointer. The
// test listener deletes that pointer before every test.

namespace {

CAN_frame CacMin(uint8_t mux, uint16_t raw_tenth_ah) {
  CAN_frame frame = {};
  frame.ID = 0x7E2;
  frame.DLC = 8;
  frame.data.u8[0] = mux;
  // Low 4 bits sit in the high nibble of byte 2. High 8 bits are byte 3.
  frame.data.u8[2] = (uint8_t)((raw_tenth_ah & 0x0F) << 4);
  frame.data.u8[3] = (uint8_t)((raw_tenth_ah >> 4) & 0xFF);
  return frame;
}

// The same division update_values() applies, so a test can show the expected
// SOH beside the value the datalayer actually got.
uint16_t SohFromRaw(uint16_t raw_tenth_ah) {
  return (uint16_t)((raw_tenth_ah * 10000u) / 2316u);
}

CAN_frame LegacyFrame(uint32_t id) {
  CAN_frame frame = {};
  frame.ID = id;
  frame.DLC = 8;
  return frame;
}

// 0x102. The driver stores whole volts and whole amps, then update_values()
// adds one decimal. 37030 raw is 370.30 V and becomes 3700 dV.
CAN_frame LegacyHvBus(uint16_t raw_centivolts, uint16_t current_bits) {
  CAN_frame frame = LegacyFrame(0x102);
  frame.data.u8[0] = (uint8_t)(raw_centivolts & 0xFF);
  frame.data.u8[1] = (uint8_t)(raw_centivolts >> 8);
  frame.data.u8[2] = (uint8_t)(current_bits & 0xFF);
  frame.data.u8[3] = (uint8_t)(current_bits >> 8);
  return frame;
}

// 0x332 holds min and max cell voltage and both temperatures in one frame.
// Voltages are 2 mV per bit. Temperatures are 0.5 C per bit with a -40 C offset.
CAN_frame LegacyBrickMinMax(uint16_t max_raw, uint16_t min_raw, uint8_t max_temp_raw, uint8_t min_temp_raw) {
  CAN_frame frame = LegacyFrame(0x332);
  frame.data.u8[0] = (uint8_t)(max_raw & 0xFF);
  frame.data.u8[1] = (uint8_t)((max_raw >> 8) & 0x0F);
  frame.data.u8[3] = max_temp_raw;
  frame.data.u8[4] = (uint8_t)(min_raw & 0xFF);
  frame.data.u8[5] = (uint8_t)((min_raw >> 8) & 0x0F);
  frame.data.u8[7] = min_temp_raw;
  return frame;
}

// 0x302 UI SOC, 10 bits from bit 0, in 0.1 percent.
CAN_frame LegacySoc(uint16_t raw_tenth_percent) {
  CAN_frame frame = LegacyFrame(0x302);
  frame.data.u8[0] = (uint8_t)(raw_tenth_percent & 0xFF);
  frame.data.u8[1] = (uint8_t)((raw_tenth_percent >> 8) & 0x03);
  return frame;
}

// 0x202 charge limit is 0.1 A. Discharge limit is 0.128 A. Both are 14 bits.
CAN_frame LegacyCurrentLimits(uint16_t charge_raw, uint16_t discharge_raw) {
  CAN_frame frame = LegacyFrame(0x202);
  frame.data.u8[4] = (uint8_t)(charge_raw & 0xFF);
  frame.data.u8[5] = (uint8_t)((charge_raw >> 8) & 0x3F);
  frame.data.u8[6] = (uint8_t)(discharge_raw & 0xFF);
  frame.data.u8[7] = (uint8_t)((discharge_raw >> 8) & 0x3F);
  return frame;
}

// 0x5D2 mux 0x0A. The hardware id is byte 4 plus byte 5, and it selects the
// pack size in update_values().
CAN_frame LegacyHwId(uint8_t id) {
  CAN_frame frame = LegacyFrame(0x5D2);
  frame.data.u8[0] = 0x0A;
  frame.data.u8[4] = id;
  return frame;
}

// 0x212. BMS state is the high nibble of byte 2. Isolation is byte 3 times 20.
CAN_frame LegacyStatus(uint8_t bms_state, uint8_t isolation_byte) {
  CAN_frame frame = LegacyFrame(0x212);
  frame.data.u8[2] = (uint8_t)((bms_state & 0x0F) << 4);
  frame.data.u8[3] = isolation_byte;
  return frame;
}

EVENTS_STATE_TYPE EventState(EVENTS_ENUM_TYPE event) {
  return get_event_pointer(event)->state;
}

}  // namespace

class TeslaLegacyBatteryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    pack_ = new TeslaLegacyBattery();
    pack_->setup();
  }

  void TearDown() override {
    delete pack_;
    pack_ = nullptr;
  }

  // One CAC frame, then the publish pass. Mux 0 is the minimum. Any other
  // mux is where a later maximum or average would arrive, and today it is ignored.
  void PublishCac(uint8_t mux, uint16_t raw_tenth_ah) {
    pack_->handle_incoming_can_frame(CacMin(mux, raw_tenth_ah));
    pack_->update_values();
  }

  TeslaLegacyBattery* pack_ = nullptr;
};

// BMS_CAC_min starts at 23160000, which is the new-pack reading. A missing frame
// therefore looks like 100.00% SOH rather than an unknown pack.
TEST_F(TeslaLegacyBatteryTest, MissingCacReadsAsANewPack) {
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000);
}

// Raw 2316 is 231.6 Ah. Byte 2 is 0xC0 and byte 3 is 0x90, so both halves of the field are in use.
TEST_F(TeslaLegacyBatteryTest, NewPackCacIsOneHundredPercent) {
  PublishCac(0, 2316);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000);
  EXPECT_EQ(SohFromRaw(2316), 10000);
}

// 200.0 Ah is not a round percentage of 231.6. 20000000 / 2316 truncates to 8635.
TEST_F(TeslaLegacyBatteryTest, TwoHundredAmpHoursTruncatesStateOfHealth) {
  PublishCac(0, 2000);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
  EXPECT_EQ(SohFromRaw(2000), 8635);
}

// Raw 171 uses the low nibble (byte 2 = 0xB0, byte 3 = 0x0A). Dropping that nibble still
// passes the 200.0 Ah case, whose low nibble is zero.
TEST_F(TeslaLegacyBatteryTest, LowNibbleIsPartOfTheCac) {
  PublishCac(0, 171);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 738);
  EXPECT_EQ(SohFromRaw(171), 738);
}

// A later max or average will be some other mux. Today any mux but 0 leaves the minimum alone.
TEST_F(TeslaLegacyBatteryTest, OtherMuxesLeaveTheMinimumAlone) {
  PublishCac(0, 2000);
  ASSERT_EQ(datalayer.battery.status.soh_pptt, 8635);

  PublishCac(1, 2316);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
}

// The frames an old S/X pack has on the bus, published in one pass.
// Voltage and current are kept as whole volts and whole amps: 370.30 V
// becomes 3700 dV, and -4.4 A becomes -40 dA. 10.5 C becomes 10.0 C.
// Hardware id 74 is the 75 kWh pack, so 80.0% SOC is 60000 Wh. Id 79 is
// the 100 kWh pack and moves the remaining energy on the next pass.
// Charge and discharge power are voltage times the BMS current limits.
// This driver has no user power cap.
TEST_F(TeslaLegacyBatteryTest, LegacyPackSnapshotPublishesTheInverterPicture) {
  pack_->handle_incoming_can_frame(LegacyHvBus(37030, 0xFFD4));
  pack_->handle_incoming_can_frame(LegacyBrickMinMax(2001, 1801, 101, 80));
  pack_->handle_incoming_can_frame(LegacySoc(800));
  pack_->handle_incoming_can_frame(LegacyCurrentLimits(1301, 430));
  pack_->handle_incoming_can_frame(LegacyHwId(74));
  PublishCac(0, 2000);

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3700);
  EXPECT_EQ(datalayer.battery.status.current_dA, -40);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 4002);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3602);
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 100);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 0);
  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 75000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 60000u);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 48100u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 20350u);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);

  pack_->handle_incoming_can_frame(LegacyHwId(79));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 100000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 80000u);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
}

// Isolation starts at 0, so the first publish raises the isolation event.
// Byte 3 times 20 is the resistance: 100 clears the event, 80 raises it,
// and neither pass changes the SOH already published. BMS state is applied
// on the 1 second transmit. State 8 raises a welded contactor, state 7
// raises an open contactor, and the welded event stays up.
TEST_F(TeslaLegacyBatteryTest, IsolationAndBmsStateReachTheEventList) {
  PublishCac(0, 2000);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(LegacyStatus(0, 5));  // 100, the first healthy reading
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);

  pack_->handle_incoming_can_frame(LegacyStatus(0, 4));  // 80
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_ACTIVE);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);

  pack_->handle_incoming_can_frame(LegacyStatus(8, 5));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_INACTIVE);
  pack_->transmit_can(INTERVAL_1_S);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(LegacyStatus(7, 5));
  pack_->transmit_can(2 * INTERVAL_1_S);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_OPEN), EVENT_STATE_ACTIVE);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8635);
}
