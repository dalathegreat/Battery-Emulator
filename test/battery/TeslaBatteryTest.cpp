#include <gtest/gtest.h>

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/battery/TESLA-BATTERY.h"
#include "../../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/datalayer/datalayer_extended.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../../Software/src/devboard/utils/types.h"

// Host tests for the Model 3/Y and newer S/X driver (TeslaBattery).
//
// A test does what the CAN task does. It hands the driver one or more frames,
// then calls update_values(). The checks are on the datalayer fields the
// inverter, the voltage limits, and the event list read after that pass.
// Private members are left alone. Where the firmware truncates a value, the
// expected number is the truncated one.
//
// The pack is a local TeslaBattery. It is not stored in the global battery
// pointer: the test listener deletes that pointer before every test, and
// storing this pack there would free it twice.
//
// SetUp starts a Model 3 with contactor control turned off, so the BMS
// contactor state is what marks the DC bus live. A test that needs an S/X
// pack or a stored chemistry changes that setting and calls setup() again.
//
// Some tests send one frame. Others send the frames a pack would have on
// the bus together, and check the picture one update_values() publishes.

namespace {

CAN_frame Frame(uint32_t id) {
  CAN_frame frame = {};
  frame.ID = id;
  frame.DLC = 8;
  return frame;
}

// 0x401 brick voltages. Status 0x2A is a full frame of three cells. Status
// 0x02 is the short last frame and carries one cell. The cell count comes
// from these status bytes, so the voltage words stay zero here.
CAN_frame BrickFrame(uint8_t mux, uint8_t flags) {
  CAN_frame frame = Frame(0x401);
  frame.data.u8[0] = mux;
  frame.data.u8[1] = flags;
  return frame;
}

// A full scan plus the second mux-0 frame that publishes the cell count.
void ScanFullFrames(TeslaBattery* pack, uint8_t last_mux) {
  for (uint8_t mux = 0; mux <= last_mux; mux++) {
    pack->handle_incoming_can_frame(BrickFrame(mux, 0x2A));
  }
  pack->handle_incoming_can_frame(BrickFrame(0, 0x2A));
}

// 0x332 mux 1. Max starts at bit 2 (12 bits, 2 mV). Min is the next 12 bits.
CAN_frame CellMinMax(uint16_t max_raw, uint16_t min_raw) {
  CAN_frame frame = Frame(0x332);
  frame.data.u8[0] = (uint8_t)(((max_raw & 0x3F) << 2) | 0x01);
  frame.data.u8[1] = (uint8_t)((max_raw >> 6) & 0x3F);
  frame.data.u8[2] = (uint8_t)(min_raw & 0xFF);
  frame.data.u8[3] = (uint8_t)((min_raw >> 8) & 0x0F);
  return frame;
}

// 0x332 mux 0. 0.5 C per bit, -40 C offset, stored as deci-C.
CAN_frame BrickTemps(uint8_t max_raw, uint8_t min_raw) {
  CAN_frame frame = Frame(0x332);
  frame.data.u8[2] = max_raw;
  frame.data.u8[3] = min_raw;
  // An average inside [min, max] is how the driver tells the firmware layouts apart.
  frame.data.u8[4] = min_raw;
  return frame;
}

// 0x132. Voltage is centivolts. Current is a signed 16-bit word in deciamperes.
CAN_frame HvBus(uint16_t centivolts, uint16_t current_bits) {
  CAN_frame frame = Frame(0x132);
  frame.data.u8[0] = (uint8_t)(centivolts & 0xFF);
  frame.data.u8[1] = (uint8_t)(centivolts >> 8);
  frame.data.u8[2] = (uint8_t)(current_bits & 0xFF);
  frame.data.u8[3] = (uint8_t)(current_bits >> 8);
  return frame;
}

// 0x292 UI SOC, 10 bits from bit 10, in 0.1 percent.
CAN_frame SocUi(uint16_t raw_tenth_percent) {
  CAN_frame frame = Frame(0x292);
  frame.data.u8[1] = (uint8_t)((raw_tenth_percent & 0x3F) << 2);
  frame.data.u8[2] = (uint8_t)((raw_tenth_percent >> 6) & 0x0F);
  return frame;
}

// 0x2D2 discharge current limit, 14 bits from bit 48, scale 0.128 A.
CAN_frame DischargeLimit(uint16_t raw) {
  CAN_frame frame = Frame(0x2D2);
  frame.data.u8[6] = (uint8_t)(raw & 0xFF);
  frame.data.u8[7] = (uint8_t)((raw >> 8) & 0x3F);
  return frame;
}

// 0x2D2 carries the charge limit in the preceding 14 bits, scale 0.1 A.
// update_values() does not turn that current into charge power.
CAN_frame CurrentLimits(uint16_t charge_raw, uint16_t discharge_raw) {
  CAN_frame frame = DischargeLimit(discharge_raw);
  frame.data.u8[4] = (uint8_t)(charge_raw & 0xFF);
  frame.data.u8[5] = (uint8_t)((charge_raw >> 8) & 0x3F);
  return frame;
}

// 0x401 with the three cell words filled in. The driver divides each raw
// count by 10 and stores millivolts at mux * 3.
CAN_frame BrickVoltages(uint8_t mux, uint8_t flags, uint16_t cell0, uint16_t cell1, uint16_t cell2) {
  CAN_frame frame = BrickFrame(mux, flags);
  frame.data.u8[2] = (uint8_t)(cell0 & 0xFF);
  frame.data.u8[3] = (uint8_t)(cell0 >> 8);
  frame.data.u8[4] = (uint8_t)(cell1 & 0xFF);
  frame.data.u8[5] = (uint8_t)(cell1 >> 8);
  frame.data.u8[6] = (uint8_t)(cell2 & 0xFF);
  frame.data.u8[7] = (uint8_t)(cell2 >> 8);
  return frame;
}

// 0x320 mux 2. CAC-change is bit 2 of byte 3. SOC-change is bit 4.
// CAC-change is an alert, not an amp-hour reading.
CAN_frame AlertMux2(bool cac_change, bool soc_change) {
  CAN_frame frame = Frame(0x320);
  frame.data.u8[0] = 2;
  frame.data.u8[3] = (uint8_t)((cac_change ? 0x04 : 0) | (soc_change ? 0x10 : 0));
  return frame;
}

// 0x212 BMS_status. Contactor state is the low 3 bits of byte 1.
CAN_frame BmsStatus(uint8_t contactor_state) {
  CAN_frame frame = Frame(0x212);
  frame.data.u8[1] = (uint8_t)(contactor_state & 0x07);
  return frame;
}

// 0x20A HVP_contactorState. HVIL status is the low 4 bits of byte 5.
CAN_frame HvilStatus(uint8_t hvil) {
  CAN_frame frame = Frame(0x20A);
  frame.data.u8[5] = (uint8_t)(hvil & 0x0F);
  return frame;
}

// 0x7AA mux 5. 12 V rail is a 12-bit value in 0.1 V, starting at bit 36.
CAN_frame TwelveVolt(uint16_t raw_tenth_volt) {
  CAN_frame frame = Frame(0x7AA);
  frame.data.u8[0] = 5;
  frame.data.u8[4] = (uint8_t)((raw_tenth_volt & 0x0F) << 4);
  frame.data.u8[5] = (uint8_t)(raw_tenth_volt >> 4);
  return frame;
}

// 0x7AA mux 2. Pack-end voltages. Zero means the contactors are open.
CAN_frame PackEnds(uint16_t negative_raw, uint16_t positive_raw) {
  CAN_frame frame = Frame(0x7AA);
  frame.data.u8[0] = 2;
  frame.data.u8[2] = (uint8_t)(negative_raw & 0xFF);
  frame.data.u8[3] = (uint8_t)(negative_raw >> 8);
  frame.data.u8[4] = (uint8_t)(positive_raw & 0xFF);
  frame.data.u8[5] = (uint8_t)(positive_raw >> 8);
  return frame;
}

void ExpectNca3YLimits() {
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4030);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2850);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_mV, 4250);
  EXPECT_EQ(datalayer.battery.info.min_cell_voltage_mV, 2950);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_deviation_mV, 500);
}

void ExpectLfpLimits() {
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 3880);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2968);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_mV, 3650);
  EXPECT_EQ(datalayer.battery.info.min_cell_voltage_mV, 2800);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_deviation_mV, 400);
}

EVENTS_STATE_TYPE EventState(EVENTS_ENUM_TYPE event) {
  return get_event_pointer(event)->state;
}

}  // namespace

// Saves the settings this file changes. setup() reads them and writes the
// pack limits, so a value left behind would leak into the next test file.
class TeslaBatteryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saved_type_ = user_selected_battery_type;
    saved_chemistry_ = user_selected_battery_chemistry;
    saved_chassis_ = user_selected_tesla_GTW_chassisType;
    saved_pack_energy_ = user_selected_tesla_GTW_packEnergy;
    saved_balancing_ = datalayer.battery_settings.user_requests_balancing;
    saved_contactor_control_ = contactor_control_enabled;

    user_selected_battery_type = BatteryType::TeslaModel3Y;
    user_selected_tesla_GTW_chassisType = 2;  // Model 3
    contactor_control_enabled = false;

    pack_ = new TeslaBattery();
    pack_->setup();
  }

  void TearDown() override {
    delete pack_;
    pack_ = nullptr;

    user_selected_battery_type = saved_type_;
    user_selected_battery_chemistry = saved_chemistry_;
    user_selected_tesla_GTW_chassisType = saved_chassis_;
    user_selected_tesla_GTW_packEnergy = saved_pack_energy_;
    datalayer.battery_settings.user_requests_balancing = saved_balancing_;
    contactor_control_enabled = saved_contactor_control_;
  }

  TeslaBattery* pack_ = nullptr;

  BatteryType saved_type_ = BatteryType::None;
  battery_chemistry_enum saved_chemistry_ = battery_chemistry_enum::NCA;
  uint16_t saved_chassis_ = 0;
  uint16_t saved_pack_energy_ = 0;
  bool saved_balancing_ = false;
  bool saved_contactor_control_ = false;
};

// The inverter stops at max_design_voltage_dV and min_design_voltage_dV.
// On a Model 3/Y those limits follow the cell count. 96 cells stay on the
// NCA/NCM window. More than 101 cells select LFP and a lower window, and a
// later 96-cell scan leaves LFP in place. Autodetect runs only when the
// configured chassis is a Model 3 or Model Y.

// 32 full frames (mux 0..31) are 96 cells, the NCA/NCM Model 3/Y size.
TEST_F(TeslaBatteryTest, NinetySixCellsKeepNcaLimits) {
  ScanFullFrames(pack_, 31);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
  ExpectNca3YLimits();
}

// 36 full frames (mux 0..35) are 108 cells. The lower LFP window keeps the
// inverter from charging that pack to the NCA voltage.
TEST_F(TeslaBatteryTest, HundredEightCellsSelectLfpLimits) {
  ScanFullFrames(pack_, 35);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectLfpLimits();
}

// A 106-cell pack reports itself with the short last frame, before a second
// full scan has been seen. Mux 35 of that frame is cell 106, which is LFP.
TEST_F(TeslaBatteryTest, PartialLastFrameSelectsLfpFromOneHundredSixCells) {
  pack_->handle_incoming_can_frame(BrickFrame(35, 0x02));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 106);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectLfpLimits();
}

// Two mux-0 frames and nothing else. The count formula yields 3. That is too
// small to be an LFP pack, so the NCA window stays.
TEST_F(TeslaBatteryTest, TwoMuxZeroFramesReportThreeCellsAndStayNca) {
  pack_->handle_incoming_can_frame(BrickFrame(0, 0x2A));
  pack_->handle_incoming_can_frame(BrickFrame(0, 0x2A));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 3);
  EXPECT_NE(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectNca3YLimits();
}

// An S/X pack keeps the wider 460 V window. A 108-cell scan still publishes
// the cell count, and the chemistry stays off LFP.
TEST_F(TeslaBatteryTest, ModelSChassisIgnoresCellCount) {
  user_selected_battery_type = BatteryType::TeslaModelSX;
  user_selected_tesla_GTW_chassisType = 0;
  pack_->setup();

  ScanFullFrames(pack_, 35);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
  EXPECT_NE(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4600);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2850);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_mV, 4250);
  EXPECT_EQ(datalayer.battery.info.min_cell_voltage_mV, 2950);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_deviation_mV, 500);
}

// A pack whose stored chemistry is already LFP keeps the LFP window. The
// 96-cell scan still updates the cell count.
TEST_F(TeslaBatteryTest, StoredLfpSurvivesANinetySixCellScan) {
  user_selected_battery_chemistry = battery_chemistry_enum::LFP;
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
  pack_->setup();
  ExpectLfpLimits();

  ScanFullFrames(pack_, 31);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectLfpLimits();
}

// Measurements update_values() copies for the inverter. Each test changes
// one reading. The expected value keeps the truncation the firmware does.

// 2001 and 1801 raw counts are 4002 mV and 3602 mV. The cell-deviation check
// compares these millivolts, so a scale that drops the 2 mV step fails.
TEST_F(TeslaBatteryTest, BrickMinMaxReachTheDatalayerInMillivolts) {
  pack_->handle_incoming_can_frame(CellMinMax(2001, 1801));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 4002);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3602);
}

// 101 and 80 raw are 10.5 C and 0.0 C. Temperatures share 0x332 with the
// cell voltages, on mux 0. The cell readings from mux 1 have to still be
// there after the temperature frame.
TEST_F(TeslaBatteryTest, BrickTemperaturesAreDeciCelsiusAndDoNotClearCellVoltages) {
  pack_->handle_incoming_can_frame(CellMinMax(2001, 1801));
  pack_->update_values();

  pack_->handle_incoming_can_frame(BrickTemps(101, 80));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 105);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 0);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 4002);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3602);
}

// 37030 centivolts becomes 3703 dV. The current word is signed: 0xFFD4 is
// -44 dA, a discharge, and 225 stays a positive charge. The sign is what
// tells the inverter which way the pack is going.
TEST_F(TeslaBatteryTest, PackVoltageAndSignedCurrent) {
  pack_->handle_incoming_can_frame(HvBus(37030, 0xFFD4));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.current_dA, -44);

  pack_->handle_incoming_can_frame(HvBus(37030, 225));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.current_dA, 225);
}

// UI SOC is a 10-bit field in 0.1%. real_soc is that value in 0.01%, which
// is what the inverter and the remaining-energy calculation read. The two
// cases sit on either side of 50%, where the field crosses from byte 1 into
// byte 2. This driver hardcodes SOH to 99% and does not read it from SOC.
TEST_F(TeslaBatteryTest, SocBelowFiftyPercent) {
  pack_->handle_incoming_can_frame(SocUi(123));  // 12.3%
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 1230);
}

TEST_F(TeslaBatteryTest, SocAboveFiftyPercent) {
  pack_->handle_incoming_can_frame(SocUi(800));  // 80.0%
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
}

// Discharge power is the BMS current times the pack voltage, in whole amps
// and whole volts. 430 raw * 0.128 A truncates to 55 A, and 3703 dV / 10
// drops the 0.3 V. The user override starts at 0 and would hide that
// product, so the test raises it first.
TEST_F(TeslaBatteryTest, DischargePowerUsesIntegerVolts) {
  datalayer.battery.status.override_discharge_power_W = 1000000;
  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  pack_->handle_incoming_can_frame(DischargeLimit(430));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 55u * 370u);
}

// Faults from the same update pass. The still-alive counter is what decides
// the pack has gone quiet, and only a frame this driver recognizes refills
// it. 0x001 leaves the counter at zero.

TEST_F(TeslaBatteryTest, RecognizedFrameRefreshesStillAlive) {
  datalayer.battery.status.CAN_battery_still_alive = 0;
  pack_->handle_incoming_can_frame(Frame(0x001));
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, 0);

  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);
}

// State 4 is CLOSED, state 1 is OPEN, state 5 is WELDED. The bus is live
// only while the BMS says closed. State 5 also raises a latched welded
// contactor event. The bus is down while that state is present.
TEST_F(TeslaBatteryTest, ContactorClosedMarksTheDcBusLive) {
  pack_->handle_incoming_can_frame(BmsStatus(1));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);

  pack_->handle_incoming_can_frame(BmsStatus(4));
  pack_->update_values();
  EXPECT_TRUE(datalayer.system.status.dc_bus_live);

  pack_->handle_incoming_can_frame(BmsStatus(5));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE_LATCHED);
}

// HVIL status 3 means a high-voltage cable was opened while the pack was in
// use. Any other status clears the fault on the next pass.
TEST_F(TeslaBatteryTest, HvilOpenRaisesInternalOpenFault) {
  pack_->handle_incoming_can_frame(HvilStatus(3));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_INTERNAL_OPEN_FAULT), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvilStatus(0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_INTERNAL_OPEN_FAULT), EVENT_STATE_INACTIVE);
}

// 0.5 V to 5.0 V is the pyro-fuse window. 20.0 V is inside it. A normal
// pack voltage clears the fault on the next pass.
TEST_F(TeslaBatteryTest, PyroFuseVoltageRaisesBatteryFuse) {
  pack_->handle_incoming_can_frame(HvBus(200, 0));  // 20.0 V
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 20);
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_INACTIVE);
}

// 116 raw is 11.6 V, under the 11.7 V contactor-supply limit. The check runs
// while both pack ends still read 0 V, which is how this driver sees open
// contactors. A low reading after an end goes non-zero clears the fault.
TEST_F(TeslaBatteryTest, LowTwelveVoltRaisesOnlyWhilePackEndsAreZero) {
  pack_->handle_incoming_can_frame(PackEnds(0, 0));
  pack_->handle_incoming_can_frame(TwelveVolt(116));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(TwelveVolt(120));  // 12.0 V
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(TwelveVolt(116));
  pack_->handle_incoming_can_frame(PackEnds(1, 0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_INACTIVE);
}

// The frames a Model 3 pack has on the bus, published in one pass. Pack
// energy 1 is the 74 kWh gateway setting, so 80.0% SOC is 59200 Wh. The
// discharge override is raised so the BMS product is what gets published.
// The cap itself is covered by the next test. Switching the gateway setting
// to 100 kWh and publishing again leaves the measurements in place and
// moves the remaining energy.
TEST_F(TeslaBatteryTest, Model3YSnapshotPublishesTheInverterPicture) {
  user_selected_tesla_GTW_packEnergy = 1;
  pack_->setup();
  datalayer.battery.status.override_discharge_power_W = 1000000;

  ScanFullFrames(pack_, 31);
  pack_->handle_incoming_can_frame(HvBus(37030, 0xFFD4));
  pack_->handle_incoming_can_frame(SocUi(800));
  pack_->handle_incoming_can_frame(CellMinMax(2001, 1801));
  pack_->handle_incoming_can_frame(BrickTemps(101, 80));
  pack_->handle_incoming_can_frame(DischargeLimit(430));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.current_dA, -44);
  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 74000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 59200u);
  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 4002);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3602);
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 105);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 0);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 55u * 370u);
  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
  ExpectNca3YLimits();
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);

  user_selected_tesla_GTW_packEnergy = 3;
  pack_->setup();
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 100000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 80000u);
  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  ExpectNca3YLimits();
}

// Discharge power is the BMS product until it passes the user cap, and then
// it is the cap. 55 A at 370 V is 20350 W, so a 5000 W cap is what the
// inverter is allowed. Charge power is the user charge setting. The 1000 A
// charge limit on the same frame does not change it.
TEST_F(TeslaBatteryTest, UserLimitsCapDischargeAndSetChargePower) {
  datalayer.battery.status.override_discharge_power_W = 5000;
  datalayer.battery.status.override_charge_power_W = 4000;
  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  pack_->handle_incoming_can_frame(CurrentLimits(10000, 430));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 4000u);
}

// A full 0x401 frame stores three cells at mux * 3. 37025 raw is 3702 mV.
// The short last frame stores only its first cell, and the cells already
// stored stay where they are.
TEST_F(TeslaBatteryTest, BrickVoltagesLandInTheCellArray) {
  pack_->handle_incoming_can_frame(BrickVoltages(1, 0x2A, 37025, 36009, 35001));
  pack_->handle_incoming_can_frame(BrickVoltages(4, 0x02, 11111, 22222, 33333));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[2], 0);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[3], 3702);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[4], 3600);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[5], 3500);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[6], 0);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[12], 1111);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[13], 0);
  EXPECT_EQ(datalayer.battery.status.cell_voltages_mV[14], 0);
}

// With contactor control on, this board owns dc_bus_live. A BMS CLOSED
// frame leaves a dead bus dead, and a BMS OPEN frame leaves a live bus live.
TEST_F(TeslaBatteryTest, BoardContactorControlKeepsTheDcBus) {
  contactor_control_enabled = true;

  datalayer.system.status.dc_bus_live = false;
  pack_->handle_incoming_can_frame(BmsStatus(4));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);

  datalayer.system.status.dc_bus_live = true;
  pack_->handle_incoming_can_frame(BmsStatus(1));
  pack_->update_values();
  EXPECT_TRUE(datalayer.system.status.dc_bus_live);
}

// State 5 latches the welded-contactor event. The following OPEN frame
// takes the bus down and leaves the latch up. clear_event only clears an
// active event, so the latch survives it.
TEST_F(TeslaBatteryTest, WeldedContactorStaysLatchedAfterTheBmsOpens) {
  pack_->handle_incoming_can_frame(BmsStatus(5));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE_LATCHED);
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);

  pack_->handle_incoming_can_frame(BmsStatus(1));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE_LATCHED);
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);
}

// BMS_a145 raises a latched SOC-recalibration event. BMS_a143 on the same
// mux is only an alert bit: SOH stays at the hardcoded 99%, which is how
// this driver says it has no CAC reading.
TEST_F(TeslaBatteryTest, SocChangeAlertIsLatchedAndCacChangeIsOnlyAnAlert) {
  pack_->handle_incoming_can_frame(AlertMux2(true, false));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_TRUE(datalayer_extended.tesla.BMS_alertMatrixActive[72]);
  EXPECT_FALSE(datalayer_extended.tesla.BMS_alertMatrixActive[74]);
  EXPECT_EQ(EventState(EVENT_BATTERY_SOC_RECALIBRATION), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(AlertMux2(true, true));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_TRUE(datalayer_extended.tesla.BMS_alertMatrixActive[72]);
  EXPECT_TRUE(datalayer_extended.tesla.BMS_alertMatrixActive[74]);
  EXPECT_EQ(EventState(EVENT_BATTERY_SOC_RECALIBRATION), EVENT_STATE_ACTIVE_LATCHED);
}

// A balancing request is the pass that may step outside the chemistry
// window. SOC becomes 99%, the max voltage limits and the charge power come
// from the balancing settings, and the minimum limits stay. Clearing the
// request publishes the CAN SOC, the NCA window, and the user charge power.
TEST_F(TeslaBatteryTest, BalancingRequestReplacesTheSafetyWindow) {
  pack_->handle_incoming_can_frame(SocUi(123));
  datalayer.battery.status.override_charge_power_W = 4000;
  datalayer.battery_settings.balancing_max_pack_voltage_dV = 4100;
  datalayer.battery_settings.balancing_max_cell_voltage_mV = 4300;
  datalayer.battery_settings.balancing_max_deviation_cell_voltage_mV = 600;
  datalayer.battery_settings.balancing_float_power_W = 1500;
  datalayer.battery_settings.user_requests_balancing = true;
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.real_soc, 9900);
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4100);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_mV, 4300);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_deviation_mV, 600);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2850);
  EXPECT_EQ(datalayer.battery.info.min_cell_voltage_mV, 2950);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 1500u);

  datalayer.battery_settings.user_requests_balancing = false;
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.real_soc, 1230);
  ExpectNca3YLimits();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 4000u);
}
