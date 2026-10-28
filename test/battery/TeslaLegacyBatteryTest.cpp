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

// 0x382 with the nominal full pack energy (bit 0, 10 bits) and the nominal energy remaining (bit 11, 11 bits), 0.1 kWh
CAN_frame Nominal382(uint16_t full_x10, uint16_t remaining_x10) {
  const uint32_t raw = (uint32_t)full_x10 | ((uint32_t)remaining_x10 << 11);
  return LegacyBytes(0x382, {(uint8_t)raw, (uint8_t)(raw >> 8), (uint8_t)(raw >> 16), (uint8_t)(raw >> 24)});
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
CAN_frame SocUiNotAvailable() {  // 0x302: SOC min 30.0 %, SOC UI 1023 (not available)
  return LegacyBytes(0x302, {0x2C, 0xFD, 0x0F, 0x00, 0x47, 0x22, 0x07, 0x00});
}
CAN_frame SocAllNotAvailable() {  // 0x302: SOC min and SOC UI both 1023
  return LegacyBytes(0x302, {0xFF, 0xFF, 0x0F, 0x00, 0xEA, 0x42, 0x31, 0x02});
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
    memset(&datalayer_extended.tesla, 0, sizeof(datalayer_extended.tesla));  // no state from earlier tests
    pack_ = new TeslaLegacyBattery();
    pack_->setup();
  }

  void TearDown() override {
    delete pack_;
    pack_ = nullptr;
  }

  TeslaLegacyBattery* pack_ = nullptr;

  // One frame and one update_values() per second (the update runs at 1 Hz)
  void FeedSeconds(int seconds, const CAN_frame& frame) {
    for (int second = 1; second <= seconds; second++) {
      pack_->handle_incoming_can_frame(frame);
      pack_->update_values();
    }
  }
};

// ---- Pack snapshot ----

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

// ---- CAC, nominal energy and the SOH estimate ----

// The block "Calculated values" at the top of the detail page: the rows between its header and the header of the raw
// rows (empty when the page has no such block)
static std::string CalculatedValues(const std::string& page) {
  const std::string start = "<h3>Calculated values</h3>";
  const size_t end = page.find("<h3>Raw values</h3>");
  if (page.compare(0, start.size(), start) != 0 || end == std::string::npos) {
    return std::string();
  }
  return page.substr(start.size(), end - start.size());
}

// 0x7E2 mux 0, 12 bit fields in 0.1 Ah: avg at bit 8 (211.7), min at bit 20 (209.3) and max at bit 32 (213.1).
TEST_F(TeslaLegacyBatteryTest, CacAverageMinMaxFromTheFrame) {
  // Raw rows, N/A until the frame was received
  const std::string na_page = pack_->get_status_renderer().get_status_html().str();
  EXPECT_NE(na_page.find("<h4>CAC Average (0x7E2 page 0): N/A</h4><h4>CAC Min (0x7E2 page 0): N/A</h4>"
                         "<h4>CAC Max (0x7E2 page 0): N/A</h4>"),
            std::string::npos);
  EXPECT_EQ(na_page.find("not received yet"), std::string::npos);
  CAN_frame frame = RealCac();
  pack_->handle_incoming_can_frame(frame);

  EXPECT_TRUE(datalayer_extended.tesla.cac_valid);
  EXPECT_EQ(datalayer_extended.tesla.cac_avg_x10, 2117);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2093);
  EXPECT_EQ(datalayer_extended.tesla.cac_max_x10, 2131);
  EXPECT_EQ(datalayer_extended.tesla.cac_raw_avg_x10[0], 2117);

  std::string page = pack_->get_status_renderer().get_status_html().str();
  // Same rows as the Model 3/Y page (single layout, no brick IDs), the imbalance is only in the calculated values
  EXPECT_NE(page.find("<h4>CAC Average (0x7E2 page 0): 211.7 Ah</h4><h4>CAC Min (0x7E2 page 0): 209.3 Ah</h4>"
                      "<h4>CAC Max (0x7E2 page 0): 213.1 Ah</h4>"),
            std::string::npos);
  EXPECT_NE(CalculatedValues(page).find("<h4>CAC: 209.3 Ah - 213.1 Ah</h4><h4>CAC Imbalance: 3.8 Ah (2%)</h4>"),
            std::string::npos);
  EXPECT_EQ(page.find("CAC Imbalance", page.find("<h3>Raw values</h3>")), std::string::npos);

  // min > avg, or a value outside 20..400 Ah, is not plausible: not used for the calculated values, but shown raw
  frame.data.u8[3] = 0x84;  // min 0x84D = 2125 > avg 2117
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2093);
  EXPECT_EQ(datalayer_extended.tesla.cac_raw_min_x10[0], 2125);
  EXPECT_NE(pack_->get_status_renderer().get_status_html().str().find("<h4>CAC Min (0x7E2 page 0): 212.5 Ah</h4>"),
            std::string::npos);
  frame.data.u8[3] = 0x82;
  frame.data.u8[2] = 0x08;  // min 0x820 = 2080 is fine again, avg bits untouched
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2080);
  frame.data.u8[2] = 0x28;  // min 0x002 = 2 (0.2 Ah)
  frame.data.u8[3] = 0x00;
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2080);
  EXPECT_EQ(datalayer_extended.tesla.cac_raw_min_x10[0], 2);

  // All ones (12 bits, 409.5 Ah) is not available
  memset(frame.data.u8 + 1, 0xFF, 7);
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.cac_min_x10, 2080);
  page = pack_->get_status_renderer().get_status_html().str();
  EXPECT_NE(page.find("<h4>CAC Average (0x7E2 page 0): N/A</h4>"), std::string::npos);
  EXPECT_NE(page.find("<h4>CAC Max (0x7E2 page 0): N/A</h4>"), std::string::npos);
}

// 0x382 bits 0-9: nominal full pack energy, 0.1 kWh. The detail page shows the raw value (N/A until received and for
// 1023), the calculated values the held one: outside 38.0-150.0 kWh (the pack sends 1023 for 5 s) the last plausible value is
// held for 30 s, so the SOH and remaining capacity do not jump during that burst.
TEST_F(TeslaLegacyBatteryTest, NominalFullPackEnergyFrom0x382) {
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  EXPECT_NE(page().find("<h4>Nominal Full Pack Energy (0x382): N/A</h4>"), std::string::npos);

  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 721);
  EXPECT_EQ(datalayer_extended.tesla.nominal_full_held_x10, 721);
  EXPECT_NE(page().find("<h4>Nominal Full Pack Energy (0x382): 72.1 kWh</h4>"), std::string::npos);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x7B, 0x01}));  // 379 = 37.9 kWh
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 379);  // raw, implausible values are shown
  EXPECT_EQ(datalayer_extended.tesla.nominal_full_held_x10, 721);             // held
  EXPECT_NE(page().find("<h4>Nominal Full Pack Energy (0x382): 37.9 kWh</h4>"), std::string::npos);
  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  pack_->update_values();
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 1023);
  EXPECT_EQ(datalayer_extended.tesla.nominal_full_held_x10, 721);
  EXPECT_NE(page().find("<h4>Nominal Full Pack Energy (0x382): N/A</h4>"), std::string::npos);
}

// The last plausible energy is held for 30 s of 1023 (not available) or implausible values, then it is unknown (SOH 99
// %, remaining Wh from the configured capacity). A plausible value ends it at once and a shorter burst restarts the
// count.
TEST_F(TeslaLegacyBatteryTest, NominalFullEnergyIsHeldDuringABurstAndUnknownAfter30Seconds) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));  // 85.0 kWh configured
  pack_->handle_incoming_can_frame(LegacySoc(271));
  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);                 // 72.1 / 85.0
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u);  // nominal energy remaining

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0x03}));  // 1023, not available
  for (int second = 1; second <= 29; second++) {
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482) << second;
    EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u) << second;
    EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known) << second;
  }
  pack_->update_values();  // 30 s
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 23035u);  // 27.1 % of 85.0 kWh
  EXPECT_EQ(datalayer_extended.tesla.nominal_full_held_x10, 0);
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 1023);  // the detail page shows what is received
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);

  pack_->handle_incoming_can_frame(RealNominal());  // back
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u);

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
  auto page = [&]() {  // the SOH is in the calculated values at the top of the detail page
    return CalculatedValues(pack_->get_status_renderer().get_status_html().str());
  };
  auto feed = [&](uint16_t x10) {
    pack_->handle_incoming_can_frame(LegacyBytes(0x382, {(uint8_t)(x10 & 0xFF), (uint8_t)(x10 >> 8)}));
    pack_->update_values();  // the estimate is calculated here
  };
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(RealCac());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 85000u);
  EXPECT_NE(page().find("Calculated SOH: unknown (99% passed to inverter)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);  // as on master, 9900 is sent

  feed(721);
  EXPECT_NE(page().find("Calculated SOH: 84.8% (72.1 kWh / 85.0 kWh)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 8482);
  EXPECT_TRUE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);

  feed(860);  // 86.0 kWh, 101.2 %
  EXPECT_NE(page().find("Calculated SOH: unknown (99% passed to inverter)"), std::string::npos);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_FALSE(datalayer_extended.tesla.soh_estimate_known);
  EXPECT_TRUE(datalayer.battery.status.soh_available);  // as on master, 9900 is sent
  feed(855);                                            // 85.5 kWh, 100.6 %
  EXPECT_NE(page().find("Calculated SOH: 100.0% (85.5 kWh / 85.0 kWh)"), std::string::npos);
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
  EXPECT_EQ(datalayer_extended.tesla.nominal_full_held_x10, 0);
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

// 0x382 bit 11, 11 bits, 0.1 kWh: nominal energy remaining (19.6 kWh in the real frame), shown on the detail page.
// N/A until received, all ones (SNA) is N/A again, the raw field keeps the all ones value.
TEST_F(TeslaLegacyBatteryTest, NominalEnergyRemainingFrom0x382) {
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  EXPECT_NE(page().find("<h4>Nominal Energy Remaining (0x382): N/A</h4>"), std::string::npos);

  pack_->handle_incoming_can_frame(RealNominal());
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_energy_remaining, 196);
  EXPECT_NE(page().find("<h4>Nominal Energy Remaining (0x382): 19.6 kWh</h4>"), std::string::npos);

  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}));
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_energy_remaining, 0x7FF);
  EXPECT_NE(page().find("<h4>Nominal Energy Remaining (0x382): N/A</h4>"), std::string::npos);
}

// A genuine 0 from the pack is shown as 0, N/A is only for "not received yet" and SNA.
TEST_F(TeslaLegacyBatteryTest, DetailPageShowsAGenuineZeroAsZero) {
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  pack_->handle_incoming_can_frame(LegacyBytes(0x382, {0x00, 0x00, 0x00, 0x00}));
  pack_->handle_incoming_can_frame(LegacyBytes(0x7E2, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}));
  EXPECT_NE(page().find("<h4>Nominal Full Pack Energy (0x382): 0.0 kWh</h4>"), std::string::npos);
  EXPECT_NE(page().find("<h4>Nominal Energy Remaining (0x382): 0.0 kWh</h4>"), std::string::npos);
  EXPECT_NE(page().find("<h4>CAC Average (0x7E2 page 0): 0.0 Ah</h4><h4>CAC Min (0x7E2 page 0): 0.0 Ah</h4>"
                        "<h4>CAC Max (0x7E2 page 0): 0.0 Ah</h4>"),
            std::string::npos);
}

// ---- Brick SOC, calculated values and temperature ----

// 0x7E2 page 7: brick SOC max at bit 8 (30.6 %) and min at bit 18 (30.0 %). The detail page shows them as received
// (N/A for 1023 and until received), the calculated values ignore 1023, max > 100 % and min > max.
TEST_F(TeslaLegacyBatteryTest, BrickSocMaxMinFrom0x7E2Page7) {
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  auto cell_rows = [&]() {
    return CalculatedValues(pack_->get_status_renderer().get_status_html().str());
  };
  EXPECT_NE(page().find("<h4>Brick SOC Max (0x7E2 page 7): N/A</h4><h4>Brick SOC Min (0x7E2 page 7): N/A</h4>"),
            std::string::npos);

  CAN_frame frame = RealPage7();
  pack_->handle_incoming_can_frame(frame);
  EXPECT_NE(page().find("<h4>Brick SOC Max (0x7E2 page 7): 30.6%</h4><h4>Brick SOC Min (0x7E2 page 7): 30.0%</h4>"),
            std::string::npos);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 306u);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_min, 300u);
  EXPECT_NE(cell_rows().find("<h4>Brick SOC: 30.0% - 30.6%</h4>"), std::string::npos);

  frame.data.u8[1] = 0xFF;
  frame.data.u8[2] = 0xB3;  // max 1023: not available
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 1023u);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_min, 300u);
  EXPECT_NE(page().find("<h4>Brick SOC Max (0x7E2 page 7): N/A</h4><h4>Brick SOC Min (0x7E2 page 7): 30.0%</h4>"),
            std::string::npos);
  EXPECT_EQ(cell_rows().find("Brick SOC"), std::string::npos);

  pack_->handle_incoming_can_frame(RealPage7());
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 306u);
  frame = RealPage7();
  frame.data.u8[1] = 0x28;  // max 0x128 = 296 < min 300
  pack_->handle_incoming_can_frame(frame);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_max, 296u);  // raw
  EXPECT_NE(page().find("<h4>Brick SOC Max (0x7E2 page 7): 29.6%</h4><h4>Brick SOC Min (0x7E2 page 7): 30.0%</h4>"),
            std::string::npos);
  EXPECT_EQ(cell_rows().find("Brick SOC"), std::string::npos);
}

// The calculated values at the top of the detail page are the Model 3/Y ones, without brick IDs and average SOC (the
// legacy pack reads none), followed by the raw rows.
TEST_F(TeslaLegacyBatteryTest, CalculatedValuesMatchTheModel3YStyle) {
  BatteryHtmlRenderer& renderer = pack_->get_status_renderer();
  const auto calculated = [&]() {
    return CalculatedValues(renderer.get_status_html().str());
  };
  pack_->update_values();
  // Always both capacity rows: "when new" is the configured capacity (the pack does not send it), "current" N/A without
  // 0x382. No CAC and brick SOC rows and no temperature yet.
  EXPECT_EQ(calculated(),
            "<h4>Calculated SOH: unknown (99% passed to inverter)</h4><h4>Total capacity when new: 30.0 kWh</h4>"
            "<h4>Current total capacity: N/A</h4><h4>Temperature min/max: N/A</h4>");

  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(RealNominal());
  pack_->handle_incoming_can_frame(RealCac());
  pack_->handle_incoming_can_frame(RealPage7());
  pack_->update_values();

  EXPECT_EQ(calculated(),
            "<h4>Calculated SOH: 84.8% (72.1 kWh / 85.0 kWh)</h4>"
            "<h4>Total capacity when new: 85.0 kWh</h4><h4>Current total capacity: 72.1 kWh</h4>"
            "<h4>CAC: 209.3 Ah - 213.1 Ah</h4><h4>CAC Imbalance: 3.8 Ah (2%)</h4><h4>Brick SOC: 30.0% - 30.6%</h4>"
            "<h4>Brick SOC Imbalance: 0.6%</h4><h4>Temperature min/max: N/A</h4>");  // no 0x332 yet
  pack_->handle_incoming_can_frame(LegacyBytes(0x332, {0x0A, 0x17, 0xBE, 0x73, 0x08, 0x67, 0x5F, 0x72}));
  pack_->update_values();
  EXPECT_NE(calculated().find("<h4>Temperature min/max: 17.0 &deg;C / 17.5 &deg;C</h4>"), std::string::npos);
}

// Temperatures keep the 0.5 C resolution of the frame: 0x73 is 17.5 C and 0x72 is 17.0 C (they used to truncate to 17).
TEST_F(TeslaLegacyBatteryTest, TemperatureKeepsHalfDegrees) {
  pack_->handle_incoming_can_frame(LegacyBytes(0x332, {0x0A, 0x17, 0xBE, 0x73, 0x08, 0x67, 0x5F, 0x72}));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 175);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 170);
}

// ---- SOC, the SOC unavailable event and the hold ----

// The pack SOC is the UI SOC (0x302 bits 10-19, 27.1 %), not SOC min (bits 0-9, 30.0 %). While the UI SOC is 1023 (not
// available, the pack does that for about 5 s) the last SOC is held, SOC min is not used: no jump from 27.1 to 30.0 %.
TEST_F(TeslaLegacyBatteryTest, PackSocIsTheUiSocAndIsHeldWhenNotAvailable) {
  EXPECT_NE(pack_->get_status_renderer().get_status_html().str().find(
                "<h4>Battery SOC UI (0x302): N/A</h4><h4>Brick SOC Min (0x302): N/A</h4>"),
            std::string::npos);
  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  EXPECT_EQ(datalayer_extended.tesla.battery_soc_ui, 271u);
  auto page = [&]() {
    return pack_->get_status_renderer().get_status_html().str();
  };
  EXPECT_NE(page().find("<h4>Battery SOC UI (0x302): 27.1%</h4><h4>Brick SOC Min (0x302): 30.0%</h4>"),
            std::string::npos);

  // SOC min 300, SOC UI 1023, for 5 s
  for (int second = 0; second < 5; second++) {
    pack_->handle_incoming_can_frame(SocUiNotAvailable());
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.real_soc, 2710);  // held
    EXPECT_NE(page().find("<h4>Battery SOC UI (0x302): N/A</h4><h4>Brick SOC Min (0x302): 30.0%</h4>"),
              std::string::npos);  // live on the detail page
  }
  pack_->handle_incoming_can_frame(SocAllNotAvailable());  // both 1023
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  EXPECT_NE(page().find("<h4>Battery SOC UI (0x302): N/A</h4><h4>Brick SOC Min (0x302): N/A</h4>"), std::string::npos);

  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
}

// Before a valid UI SOC was ever seen SOC min is the initial SOC (0 would zero the discharge limit), a valid UI SOC
// replaces it for good.
TEST_F(TeslaLegacyBatteryTest, SocMinIsOnlyTheInitialSocBeforeAValidUiSoc) {
  pack_->handle_incoming_can_frame(SocUiNotAvailable());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 3000);  // SOC min 30.0 %, UI SOC 1023

  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(SocUiNotAvailable());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
}

// 30 s without a valid UI SOC raise EVENT_SOC_UNAVAILABLE (a WARNING), the SOC is still the last one. A valid UI SOC
// clears it.
TEST_F(TeslaLegacyBatteryTest, SocUnavailableEventAfter30SecondsAndClearedByAValidSoc) {
  init_events();  // Event levels are only assigned here; ctest runs each test in its own process
  clear_event(EVENT_SOC_UNAVAILABLE);
  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();

  FeedSeconds(29, SocUiNotAvailable());
  EXPECT_NE(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_ACTIVE);  // 29 s

  pack_->handle_incoming_can_frame(SocUiNotAvailable());
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

// The held SOC is stale after 30 minutes without a valid UI SOC: cleared (real SOC 0, remaining 0), the warning event
// stays, a valid UI SOC brings it back. The update runs once a second.
TEST_F(TeslaLegacyBatteryTest, HeldSocIsClearedAfter30Minutes) {
  init_events();
  clear_event(EVENT_SOC_UNAVAILABLE);
  pack_->handle_incoming_can_frame(RealNominal());
  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  EXPECT_GT(datalayer.battery.status.remaining_capacity_Wh, 0u);

  FeedSeconds(1799, SocUiNotAvailable());
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);  // 29 min 59 s: still held
  EXPECT_EQ(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(SocUiNotAvailable());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 0);  // 30 min: cleared
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 0u);
  EXPECT_EQ(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(RealSoc());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  EXPECT_GT(datalayer.battery.status.remaining_capacity_Wh, 0u);
  EXPECT_EQ(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_INACTIVE);
}

// The initial SOC min (no valid UI SOC seen yet) is live, so it does not age: it is not cleared while the pack keeps
// sending it, the warning event stays. When it stops too, the last SOC is held for 30 minutes.
TEST_F(TeslaLegacyBatteryTest, InitialSocMinIsNotClearedWhileItIsSent) {
  init_events();
  clear_event(EVENT_SOC_UNAVAILABLE);
  FeedSeconds(1801, SocUiNotAvailable());  // min 30.0
  EXPECT_EQ(datalayer.battery.status.real_soc, 3000);
  EXPECT_EQ(EventState(EVENT_SOC_UNAVAILABLE), EVENT_STATE_ACTIVE);

  FeedSeconds(1799, SocAllNotAvailable());             // both n/a
  EXPECT_EQ(datalayer.battery.status.real_soc, 3000);  // 29 min 59 s: still held
  pack_->handle_incoming_can_frame(SocAllNotAvailable());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 0);  // 30 min: cleared
}

// ---- Remaining capacity ----

// Remaining capacity is the 0x382 nominal energy remaining when plausible (19.6 kWh in the real frame, SOC x full is
// 27.1 % of 72.1 = 19.54 kWh), else SOC x the nominal full energy, else x the configured capacity (27.1 % of 85 kWh).
TEST_F(TeslaLegacyBatteryTest, RemainingCapacityUsesTheNominalEnergyRemaining) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(LegacySoc(271));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 23035u);

  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u);
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 85000u);  // the configured capacity is unchanged
}

// Not plausible: SNA (0x7FF), above the nominal full energy plus 0.1 kWh, 0 and not received. Each falls back to SOC x
// nominal full (19.54 kWh). Without a plausible nominal full energy it is SOC x configured capacity (23.04 kWh).
TEST_F(TeslaLegacyBatteryTest, ImplausibleNominalEnergyRemainingFallsBackToTheSocOfTheFullEnergy) {
  auto remaining_with = [&](uint16_t full_x10, uint16_t remaining_x10) {
    delete pack_;
    pack_ = new TeslaLegacyBattery();
    pack_->handle_incoming_can_frame(LegacyHwId(70));
    pack_->handle_incoming_can_frame(LegacySoc(271));
    pack_->handle_incoming_can_frame(Nominal382(full_x10, remaining_x10));
    pack_->update_values();
    return datalayer.battery.status.remaining_capacity_Wh;
  };
  EXPECT_EQ(remaining_with(721, 196), 19600u);
  EXPECT_EQ(remaining_with(721, 722), 72200u);  // full + 0.1 kWh is the limit
  EXPECT_EQ(remaining_with(721, 723), 19539u);
  EXPECT_EQ(remaining_with(721, 0x7FF), 19539u);  // SNA
  EXPECT_EQ(remaining_with(721, 0), 19539u);
  EXPECT_EQ(remaining_with(1023, 196), 23035u);  // the full energy is not available: SOC x configured capacity
  EXPECT_EQ(remaining_with(300, 196), 23035u);   // 30.0 kWh is not plausible
}

// The last plausible nominal energy remaining is held for 30 s like the nominal full energy, then the fallback
TEST_F(TeslaLegacyBatteryTest, NominalEnergyRemainingIsHeldFor30SecondsThenTheFallback) {
  pack_->handle_incoming_can_frame(LegacyHwId(70));
  pack_->handle_incoming_can_frame(LegacySoc(271));
  pack_->handle_incoming_can_frame(RealNominal());
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u);

  pack_->handle_incoming_can_frame(Nominal382(721, 0x7FF));  // the remaining energy is not available
  for (int second = 1; second <= 29; second++) {
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u) << second;
  }
  pack_->update_values();                                             // 30 s
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19539u);  // SOC x nominal full

  pack_->handle_incoming_can_frame(RealNominal());  // back
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 19600u);

  // The detail page shows what is received, not what is held
  pack_->handle_incoming_can_frame(Nominal382(721, 0x7FF));
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_energy_remaining, 0x7FF);
}
