#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>

#include "../../Software/src/battery/TESLA-LEGACY-BATTERY.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/datalayer/datalayer_extended.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../../Software/src/devboard/utils/types.h"

// Host tests for the 2012-2020 Model S/X driver (TeslaLegacyBattery).
//
// A test sends frames the way the CAN task does, then calls update_values().
//
// Isolation starts at 0 and raises its own event on the first pass. The snapshot below checks that event, the
// whole-amp current, and an unknown hardware id.
//
// The pack is local and is not stored in the global battery pointer. The
// test listener deletes that pointer before every test.

namespace {

CAN_frame LegacyFrame(uint32_t id) {
  CAN_frame frame = {};
  frame.ID = id;
  frame.DLC = 8;
  return frame;
}

CAN_frame LegacyBytes(uint32_t id, std::initializer_list<uint8_t> bytes) {
  CAN_frame frame = LegacyFrame(id);
  std::copy(bytes.begin(), bytes.end(), frame.data.u8);
  return frame;
}

// Frames from a real pack
CAN_frame RealCac() {  // 0x7E2 page 0: avg 211.7 Ah, min 209.3 Ah, max 213.1 Ah
  return LegacyBytes(0x7E2, {0x00, 0x45, 0xD8, 0x82, 0x53, 0xB8, 0x82, 0xDF});
}
CAN_frame RealPage7() {  // 0x7E2 page 7: brick SOC max 30.6 %, min 30.0 %
  return LegacyBytes(0x7E2, {0x07, 0x32, 0xB1, 0xA4, 0xF4, 0xD3, 0x3A, 0x49});
}
CAN_frame RealNominal() {  // 0x382: nominal full pack energy 72.1 kWh
  return LegacyBytes(0x382, {0xD1, 0x22, 0x46, 0x30, 0x82, 0x01, 0x00, 0x1E});
}
CAN_frame RealSoc() {  // 0x302: SOC min 30.0 %, SOC UI 27.1 %
  return LegacyBytes(0x302, {0x2C, 0x3D, 0x14, 0x00, 0xEA, 0x42, 0x31, 0x02});
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

// 0x302 SOC min (10 bits from bit 0) and UI SOC (10 bits from bit 10), both set to the same value, in 0.1 percent.
CAN_frame LegacySoc(uint16_t raw_tenth_percent) {
  CAN_frame frame = LegacyFrame(0x302);
  frame.data.u8[0] = (uint8_t)(raw_tenth_percent & 0xFF);
  frame.data.u8[1] = (uint8_t)(((raw_tenth_percent >> 8) & 0x03) | ((raw_tenth_percent & 0x3F) << 2));
  frame.data.u8[2] = (uint8_t)(raw_tenth_percent >> 6);
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
    memset(&datalayer_extended.tesla, 0, sizeof(datalayer_extended.tesla));  // no state from earlier tests
  }

  void TearDown() override {
    delete pack_;
    pack_ = nullptr;
  }

  TeslaLegacyBattery* pack_ = nullptr;
};

// The frames an old S/X pack has on the bus, published in one pass.
// Voltage and current are kept as whole volts and whole amps: 370.30 V
// becomes 3700 dV, and -4.4 A becomes -40 dA. A charge of 22.5 A becomes
// 220 dA. Power stays on the BMS current limits, not the measured current.
// 10.5 C becomes 10.0 C. Hardware id 0 means the id has not been read: the
// default pack size stays, and the unknown-id event stays down. Id 74 is
// the 75 kWh pack, so 80.0% SOC is 60000 Wh. Id 79 is the 100 kWh pack.
// Id 1 is not a known size. It raises the unknown-id event and leaves the
// 100 kWh capacity in place.
// Charge and discharge power are voltage times the BMS current limits.
// This driver has no user power cap. Isolation starts at 0 and raises on
// the first pass that still has no isolation frame. A later healthy reading
// clears it, and a low reading raises it again. BMS state is applied on the
// one-second transmit: welded, then open, and the welded event stays up.
TEST_F(TeslaLegacyBatteryTest, LegacyPackSnapshotPublishesTheInverterPicture) {
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 30000u);
  EXPECT_EQ(EventState(EVENT_BATTERY_VALUE_UNAVAILABLE), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(LegacyHvBus(37030, 0xFFD4));
  pack_->handle_incoming_can_frame(LegacyBrickMinMax(2001, 1801, 101, 80));
  pack_->handle_incoming_can_frame(LegacySoc(800));
  pack_->handle_incoming_can_frame(LegacyCurrentLimits(1301, 430));
  pack_->handle_incoming_can_frame(LegacyHwId(74));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3700);
  EXPECT_EQ(datalayer.battery.status.current_dA, -40);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 4002);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3602);
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 105);  // raw 101 is 10.5 C
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 0);
  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 75000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 60000u);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 48100u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 20350u);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);
  // Isolation is still 0, so this same pass raises the event.
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(LegacyHvBus(37030, 225));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3700);
  EXPECT_EQ(datalayer.battery.status.current_dA, 220);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 48100u);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 20350u);

  pack_->handle_incoming_can_frame(LegacyHwId(79));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 100000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 80000u);

  pack_->handle_incoming_can_frame(LegacyHwId(1));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 100000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 80000u);
  EXPECT_EQ(EventState(EVENT_BATTERY_VALUE_UNAVAILABLE), EVENT_STATE_ACTIVE);

  // Byte 3 times 20 is the resistance. 100 is the first healthy reading and
  // clears the event. 80 raises it again.
  pack_->handle_incoming_can_frame(LegacyStatus(0, 5));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(LegacyStatus(0, 4));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_ACTIVE);

  // BMS state is applied on the 1 second transmit. State 8 raises a welded
  // contactor, state 7 raises an open contactor, and the welded event stays up.
  pack_->handle_incoming_can_frame(LegacyStatus(8, 5));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_ISOLATION), EVENT_STATE_INACTIVE);
  pack_->transmit_can(INTERVAL_1_S);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(LegacyStatus(7, 5));
  pack_->transmit_can(2 * INTERVAL_1_S);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_OPEN), EVENT_STATE_ACTIVE);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE);
}

// 0x7E2 mux 0, 12 bit fields in 0.1 Ah: avg at bit 8 (211.7), min at bit 20 (209.3) and max at bit 32 (213.1).
TEST_F(TeslaLegacyBatteryTest, CacAverageMinMaxFromTheFrame) {
  EXPECT_NE(pack_->get_status_renderer().get_status_html().str().find("<h4>CAC: not received yet"), std::string::npos);
  CAN_frame frame = RealCac();
  pack_->handle_incoming_can_frame(frame);

  EXPECT_TRUE(datalayer_extended.tesla.cac_valid);
  EXPECT_EQ(datalayer_extended.tesla.cac_avg_x10, 2117);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2093);
  EXPECT_EQ(datalayer_extended.tesla.cac_max_x10, 2131);

  std::string page = pack_->get_status_renderer().get_status_html().str();
  // Same rows and order as the Model 3/Y page (no brick IDs here), with the imbalance of 3.8 Ah, 2 % of the average
  EXPECT_NE(page.find("<h4>CAC Average: 211.7 Ah</h4><h4>CAC Min: 209.3 Ah</h4><h4>CAC Max: 213.1 Ah</h4>"
                      "<h4>CAC Imbalance: 3.8 Ah (2%)</h4>"),
            std::string::npos);

  // min > avg, or a value outside 20..400 Ah, is not plausible and is not taken over
  frame.data.u8[3] = 0x84;  // min 0x84D = 2125 > avg 2117
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2093);
  frame.data.u8[3] = 0x82;
  frame.data.u8[2] = 0x08;  // min 0x820 = 2080 is fine again, avg bits untouched
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2080);
  frame.data.u8[2] = 0x28;  // min 0x022 = 34 (3.4 Ah)
  frame.data.u8[3] = 0x00;
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2080);
}

// 0x382 bits 0-9: nominal full pack energy, 0.1 kWh. Outside 38.0-150.0 kWh (the pack sends 1023 for 5 s) the last
// plausible value is held for 30 s, so the SOH and remaining capacity do not jump during that burst.
TEST_F(TeslaLegacyBatteryTest, NominalFullPackEnergyFrom0x382) {
  EXPECT_NE(pack_->get_status_renderer().get_status_html().str().find("Energy: not received yet"), std::string::npos);

  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 721);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x7B, 0x01}));  // 379 = 37.9 kWh
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 721);
  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 721);
}

// The last plausible energy is held for 30 s of 1023 (not available) or implausible values, then it is unknown (SOH 99
// %, remaining Wh from the configured capacity). A plausible value ends it at once and a shorter burst restarts the
// count.
TEST_F(TeslaLegacyBatteryTest, NominalFullEnergyIsHeldDuringABurstAndUnknownAfter30Seconds) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));  // 85.0 kWh configured
  pack_->handle_incoming_can_frame(LegacySoc(271));
  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);  // 72.1 / 85.0
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19539u);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0x03}));  // 1023, not available
  for (int second = 1; second <= 29; second++) {
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482) << second;
    EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19539u) << second;
    EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known) << second;
  }
  pack_->update_values();  // 30 s
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 23035u);  // 27.1 % of 85.0 kWh
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 0);
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);

  pack_->handle_incoming_can_frame(RealNominal());  // back
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19539u);

  for (int burst = 0; burst < 3; burst++) {  // 20 s bursts do not add up
    pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0x03}));
    for (int second = 0; second < 20; second++) {
      pack_->update_values();
    }
    pack_->handle_incoming_can_frame(RealNominal());
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);
  }
}

// The SOH sent to the inverter is the estimate: 0x382 nominal full energy / configured capacity (id 70 is 85 kWh): 72.1
// kWh is 84.8 %. Unknown (99 % to the inverter) without the energy or above 101 %, 100 % up to 101 %. CAC is ignored.
TEST_F(TeslaLegacyBatteryTest, SohEstimateIsNominalEnergyOverConfiguredCapacity) {
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  auto feed = [&](uint16_t x10) {
    pack_->handle_incoming_can_frame(LegacyBytes(0x382, {(uint8_t)(x10 & 0xFF), (uint8_t)(x10 >> 8)}));
    pack_->update_values();  // the estimate is calculated here
  };
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(RealCac());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 85000u);
  EXPECT_NE(page().find("Estimate SOH: unknown (99% passed to inverter)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);  // as on master, 9900 is sent

  feed(721);
  EXPECT_NE(page().find("Estimate SOH: 84.8% (72.1 kWh / 85.0 kWh)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);
  EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);

  feed(860);  // 86.0 kWh, 101.2 %
  EXPECT_NE(page().find("Estimate SOH: unknown (99% passed to inverter)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);  // as on master, 9900 is sent
  feed(855);                                            // 85.5 kWh, 100.6 %
  EXPECT_NE(page().find("Estimate SOH: 100.0% (85.5 kWh / 85.0 kWh)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000);
  feed(1023);  // not available: the last plausible energy stays
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 10000);
  EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);
}

// An energy outside 38.0 to 150.0 kWh is not decoded, so the SOH stays unknown (99 % to the inverter) until a plausible
// one arrives, never a tiny SOH.
TEST_F(TeslaLegacyBatteryTest, SohEstimateStaysUnknownForAnImplausibleNominalEnergy) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x14, 0x00}));  // 2.0 kWh
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 0);
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);  // as on master, 9900 is sent
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x7B, 0x01}));  // 37.9 kWh
  pack_->update_values();
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);  // as on master, 9900 is sent
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x7C, 0x01}));  // 38.0 kWh of 85.0 kWh
  pack_->update_values();
  EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 4470);
}

// The reference is the capacity configured for the hardware ID, not the real capacity when new (known limitation): a
// new pack of 81 kWh on an 85 kWh configuration shows 95 % instead of 100 %.
TEST_F(TeslaLegacyBatteryTest, SohReferenceIsTheConfiguredCapacityNotTheRealCapacityWhenNew) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x2A, 0x03}));  // 810 = 81.0 kWh
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 85000u);
  EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9529);
}

// 0x382 bit 11, 11 bits, 0.1 kWh: nominal energy remaining (19.6 kWh in the real frame), shown on the detail page only.
// No data until received, all ones (SNA) is no data again.
TEST_F(TeslaLegacyBatteryTest, NominalEnergyRemainingFrom0x382) {
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  EXPECT_NE(page().find("Nominal Energy Remaining: no data"), std::string::npos);

  pack_->handle_incoming_can_frame(RealNominal());
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_energy_remaining, 196);
  EXPECT_NE(page().find("<h4>Nominal Energy Remaining: 19.6 kWh</h4>"), std::string::npos);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_energy_remaining, 0);
  EXPECT_NE(page().find("Nominal Energy Remaining: no data"), std::string::npos);
}

// 0x7E2 page 7: brick SOC max at bit 8 (30.6 %) and min at bit 18 (30.0 %). 1023, max > 100 % and min > max: rejected.
TEST_F(TeslaLegacyBatteryTest, BrickSocMaxMinFrom0x7E2Page7) {
  CAN_frame frame = RealPage7();
  pack_->handle_incoming_can_frame(frame);
  EXPECT_NE(pack_->get_status_renderer().get_status_html().str().find("Max: 30.6%</h4><h4>Brick SOC Min: 30.0%</h4>"),
            std::string::npos);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 306u);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_min, 300u);

  frame.data.u8[1] = 0xFF;
  frame.data.u8[2] = 0xB3;  // max 1023: not available
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 0u);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_min, 0u);

  pack_->handle_incoming_can_frame(RealPage7());
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 306u);
  frame = RealPage7();
  frame.data.u8[1] = 0x28;  // max 0x128 = 296 < min 300
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 0u);
}

// Main page rows come from the Model 3/Y hooks, without brick IDs and average SOC (the legacy pack reads none).
TEST_F(TeslaLegacyBatteryTest, MainPageRowsMatchTheModel3YStyle) {
  BatteryHtmlRenderer& renderer = pack_->get_status_renderer();
  pack_->update_values();
  EXPECT_EQ(renderer.get_main_view_soh_rows().str(), "<h4>Estimate SOH: unknown (99% passed to inverter)</h4>");
  EXPECT_TRUE(renderer.get_main_view_capacity_rows().str().empty());
  EXPECT_TRUE(renderer.get_main_view_cell_rows().str().empty());

  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(RealNominal());
  pack_->handle_incoming_can_frame(RealCac());
  pack_->handle_incoming_can_frame(RealPage7());
  pack_->update_values();

  EXPECT_EQ(renderer.get_main_view_soh_rows().str(), "<h4>Estimate SOH: 84.8% (72.1 kWh / 85.0 kWh)</h4>");
  EXPECT_EQ(renderer.get_main_view_capacity_rows().str(), "<h4>Current capacity (reported by pack): 72.1 kWh</h4>");
  EXPECT_EQ(renderer.get_main_view_cell_rows().str(),
            "<h4>CAC: 209.3 Ah - 213.1 Ah</h4><h4>Brick SOC: 30.0% - 30.6%</h4>");
  EXPECT_FALSE(renderer.has_temperature_data());  // no 0x332 yet: the main card shows "no data"
  pack_->handle_incoming_can_frame(LegacyBytes(0x332, {0x0A, 0x17, 0xBE, 0x73, 0x08, 0x67, 0x5F, 0x72}));
  EXPECT_TRUE(renderer.has_temperature_data());
}

// Temperatures keep the 0.5 C resolution of the frame: 0x73 is 17.5 C and 0x72 is 17.0 C (they used to truncate to 17).
TEST_F(TeslaLegacyBatteryTest, TemperatureKeepsHalfDegrees) {
  pack_->handle_incoming_can_frame(LegacyBytes(0x332, {0x0A, 0x17, 0xBE, 0x73, 0x08, 0x67, 0x5F, 0x72}));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 175);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 170);
}

// The pack SOC is the UI SOC (0x302 bits 10-19, 27.1 %), not SOC min (bits 0-9, 30.0 %). While the UI SOC is 1023 (not
// available, the pack does that for about 5 s) the last SOC is held, SOC min is not used: no jump from 27.1 to 30.0 %.
TEST_F(TeslaLegacyBatteryTest, PackSocIsTheUiSocAndIsHeldWhenNotAvailable) {
  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_ui, 271u);
  EXPECT_NE(pack_->get_status_renderer().get_status_html().str().find("<h4>Battery SOC UI: 27.1"), std::string::npos);

  // SOC min 300, SOC UI 1023, for 5 s
  for (int second = 0; second < 5; second++) {
    pack_->handle_incoming_can_frame(LegacyBytes(0x302, {0x2C, 0xFD, 0x0F, 0x00, 0x47, 0x22, 0x07, 0x00}));
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  }
  pack_->handle_incoming_can_frame(LegacyBytes(0x302, {0xFF, 0xFF, 0x0F, 0x00, 0xEA, 0x42, 0x31, 0x02}));  // both 1023
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
}

// Before a valid UI SOC was ever seen SOC min is the initial SOC (0 would zero the discharge limit), a valid UI SOC
// replaces it for good.
TEST_F(TeslaLegacyBatteryTest, SocMinIsOnlyTheInitialSocBeforeAValidUiSoc) {
  pack_->handle_incoming_can_frame(LegacyBytes(0x302, {0x2C, 0xFD, 0x0F, 0x00, 0x47, 0x22, 0x07, 0x00}));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 3000);  // SOC min 30.0 %, UI SOC 1023

  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(LegacyBytes(0x302, {0x2C, 0xFD, 0x0F, 0x00, 0x47, 0x22, 0x07, 0x00}));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
}

// 30 s without a valid UI SOC raise EVENT_SOC_UNAVAILABLE (a WARNING), the SOC is still the last one. A valid UI SOC
// clears it.
TEST_F(TeslaLegacyBatteryTest, SocUnavailableEventAfter30SecondsAndClearedByAValidSoc) {
  clear_event(EVENT_SOC_UNAVAILABLE);
  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();

  for (int second = 1; second <= 29; second++) {
    pack_->handle_incoming_can_frame(LegacyBytes(0x302, {0x2C, 0xFD, 0x0F, 0x00, 0x47, 0x22, 0x07, 0x00}));
    pack_->update_values();
  }
  EXPECT_NE(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_ACTIVE);  // 29 s

  pack_->handle_incoming_can_frame(LegacyBytes(0x302, {0x2C, 0xFD, 0x0F, 0x00, 0x47, 0x22, 0x07, 0x00}));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_ACTIVE);  // 30 s
  EXPECT_EQ(get_event_pointer(EVENT_SOC_UNAVAILABLE)->data, 30);
  EXPECT_EQ(get_event_pointer(EVENT_SOC_UNAVAILABLE)->level, EVENT_LEVEL_WARNING);
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_INACTIVE);
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
}

// Remaining capacity is SOC x the 0x382 nominal full energy when readable (27.1 % of 72.1 kWh), else x the configured
// capacity (27.1 % of 85 kWh).
TEST_F(TeslaLegacyBatteryTest, RemainingCapacityUsesTheNominalFullEnergy) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(LegacySoc(271));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 23035u);

  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19539u);
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 85000u);  // the configured capacity is unchanged

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0x03}));  // 1023, not available
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19539u);
}
