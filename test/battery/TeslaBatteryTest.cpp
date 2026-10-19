#include <gtest/gtest.h>

#include "../../Software/src/battery/BATTERIES.h"
#include "../../Software/src/battery/TESLA-BATTERY.h"
#include "../../Software/src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../Software/src/datalayer/datalayer.h"
#include "../../Software/src/devboard/utils/events.h"
#include "../../Software/src/devboard/utils/types.h"

// Host tests for the values the safety layer and the inverter actually consume.
// The driver is left as it is: these cases pin today's scaling, including the
// integer division in the discharge-power product.

namespace {

CAN_frame Frame(uint32_t id) {
  CAN_frame frame = {};
  frame.ID = id;
  frame.DLC = 8;
  return frame;
}

// 0x401 brick voltages. 0x2A carries three cells, 0x02 is the short last frame.
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

class TeslaBatteryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    saved_type_ = user_selected_battery_type;
    saved_chemistry_ = user_selected_battery_chemistry;
    saved_chassis_ = user_selected_tesla_GTW_chassisType;
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
    datalayer.battery_settings.user_requests_balancing = saved_balancing_;
    contactor_control_enabled = saved_contactor_control_;
  }

  TeslaBattery* pack_ = nullptr;

  BatteryType saved_type_ = BatteryType::None;
  battery_chemistry_enum saved_chemistry_ = battery_chemistry_enum::NCA;
  uint16_t saved_chassis_ = 0;
  bool saved_balancing_ = false;
  bool saved_contactor_control_ = false;
};

// 32 full frames (mux 0..31) are 96 cells. That is an NCA/NCM pack.
TEST_F(TeslaBatteryTest, NinetySixCellsKeepNcaLimits) {
  ScanFullFrames(pack_, 31);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
  ExpectNca3YLimits();
}

// 36 full frames (mux 0..35) are 108 cells. A 3/Y pack past 101 cells is LFP.
TEST_F(TeslaBatteryTest, HundredEightCellsSelectLfpLimits) {
  ScanFullFrames(pack_, 35);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectLfpLimits();
}

// The short last frame publishes the count on its own: mux 35 holds cell 106.
TEST_F(TeslaBatteryTest, PartialLastFrameSelectsLfpFromOneHundredSixCells) {
  pack_->handle_incoming_can_frame(BrickFrame(35, 0x02));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 106);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectLfpLimits();
}

// Two mux-0 frames and nothing else. The count formula yields 3, which is not LFP.
TEST_F(TeslaBatteryTest, TwoMuxZeroFramesReportThreeCellsAndStayNca) {
  pack_->handle_incoming_can_frame(BrickFrame(0, 0x2A));
  pack_->handle_incoming_can_frame(BrickFrame(0, 0x2A));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 3);
  EXPECT_NE(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectNca3YLimits();
}

// Cell-count autodetection runs only for chassis types above Model S/X.
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

// setup_battery() copies the stored chemistry before the driver runs. Autodetect
// can move a 3/Y pack to LFP. A 96-cell scan does not move it back.
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

// 2001 and 1801 raw counts are 4002 mV and 3602 mV. A scale that drops the 2 mV step fails.
TEST_F(TeslaBatteryTest, BrickMinMaxReachTheDatalayerInMillivolts) {
  pack_->handle_incoming_can_frame(CellMinMax(2001, 1801));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.cell_max_voltage_mV, 4002);
  EXPECT_EQ(datalayer.battery.status.cell_min_voltage_mV, 3602);
}

// 101 and 80 raw are 10.5 C and 0.0 C. A later temperature frame must not wipe the cells.
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

TEST_F(TeslaBatteryTest, PackVoltageAndSignedCurrent) {
  // 37030 centivolts -> 3703 dV. 0xFFD4 is -44 as a signed deciampere.
  pack_->handle_incoming_can_frame(HvBus(37030, 0xFFD4));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.current_dA, -44);

  pack_->handle_incoming_can_frame(HvBus(37030, 225));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.current_dA, 225);
}

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

// The user override starts at 0 and would clamp every reading. Raise it so the
// product itself is what gets published. 430 raw * 0.128 A truncates to a whole
// ampere, then the pack voltage loses its 0.3 V: 3703 dV / 10 = 370.
TEST_F(TeslaBatteryTest, DischargePowerUsesIntegerVolts) {
  datalayer.battery.status.override_discharge_power_W = 1000000;
  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  pack_->handle_incoming_can_frame(DischargeLimit(430));
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 55u * 370u);
}

TEST_F(TeslaBatteryTest, RecognizedFrameRefreshesStillAlive) {
  datalayer.battery.status.CAN_battery_still_alive = 0;
  pack_->handle_incoming_can_frame(Frame(0x001));
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, 0);

  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);
}

// State 4 is CLOSED, state 1 is OPEN, state 5 is WELDED. The bus is live only
// while the BMS says closed, and only when this board is not driving the contactors.
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

TEST_F(TeslaBatteryTest, HvilOpenRaisesInternalOpenFault) {
  pack_->handle_incoming_can_frame(HvilStatus(3));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_INTERNAL_OPEN_FAULT), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvilStatus(0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_INTERNAL_OPEN_FAULT), EVENT_STATE_INACTIVE);
}

// 5..50 dV is the pyro-fuse window. 20 dV is inside it, a normal pack voltage is not.
TEST_F(TeslaBatteryTest, PyroFuseVoltageRaisesBatteryFuse) {
  pack_->handle_incoming_can_frame(HvBus(200, 0));  // 20.0 V
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 20);
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_INACTIVE);
}

// 116 raw is 11.6 V. The check runs only while both pack ends still read 0 V.
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
