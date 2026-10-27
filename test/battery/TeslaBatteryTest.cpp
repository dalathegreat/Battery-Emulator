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
// A scan that has to start with no cell count uses its own pack. The full
// scan publishes that count once per pack.

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

// 0x292 SOC min, UI SOC, brick SOC max and average, 10 bits each from bit 0, in 0.1 percent.
CAN_frame SocFields(uint64_t min, uint64_t ui, uint64_t max, uint64_t ave) {
  const uint64_t raw = min | (ui << 10) | (max << 20) | (ave << 30);
  CAN_frame frame = Frame(0x292);
  for (int i = 0; i < 5; i++) {
    frame.data.u8[i] = (uint8_t)(raw >> (8 * i));
  }
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
// Isolation resistance is 10 bits at bit 19, in steps of 10 kOhm. The
// datalayer stores that count times 10, in kOhm. Raw 16 is byte 2 = 128.
CAN_frame BmsStatus(uint8_t contactor_state, uint16_t isolation_raw = 0) {
  CAN_frame frame = Frame(0x212);
  frame.data.u8[1] = (uint8_t)(contactor_state & 0x07);
  frame.data.u8[2] = (uint8_t)((isolation_raw & 0x1F) << 3);
  frame.data.u8[3] = (uint8_t)((isolation_raw >> 5) & 0x1F);
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

void ExpectSxLimits() {
  EXPECT_EQ(datalayer.battery.info.max_design_voltage_dV, 4600);
  EXPECT_EQ(datalayer.battery.info.min_design_voltage_dV, 2850);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_mV, 4250);
  EXPECT_EQ(datalayer.battery.info.min_cell_voltage_mV, 2950);
  EXPECT_EQ(datalayer.battery.info.max_cell_voltage_deviation_mV, 500);
}

// The fixture pack is counted by the first full scan. Later scans that must
// start uncounted get their own object, and this deletes it.
struct LocalPack {
  TeslaBattery* pack;
  LocalPack() : pack(new TeslaBattery()) { pack->setup(); }
  ~LocalPack() { delete pack; }
  LocalPack(const LocalPack&) = delete;
  LocalPack& operator=(const LocalPack&) = delete;
  TeslaBattery* get() const { return pack; }
  TeslaBattery* operator->() const { return pack; }
};

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
// On a Model 3 or Model Y, more than 101 cells selects LFP. Chassis 0 is
// Model S and chassis 1 is Model X: a 108-cell scan still publishes the
// count, and the 460 V window stays. Chassis 3 is Model Y and does select
// LFP. A stored LFP chemistry stays LFP through a 96-cell scan. The 96-cell
// NCA pack is the snapshot below.
//
// The 106-cell pack is the short last frame alone, mux 35. Two mux-0 frames
// and nothing else publish 3 cells, which is too small to leave NCA.
TEST_F(TeslaBatteryTest, CellCountAndChassisSelectTheVoltageWindow) {
  ScanFullFrames(pack_, 35);
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
  ExpectLfpLimits();

  {
    datalayer.battery.info.chemistry = battery_chemistry_enum::NCA;
    LocalPack partial;
    partial->handle_incoming_can_frame(BrickFrame(35, 0x02));
    partial->update_values();

    EXPECT_EQ(datalayer.battery.info.number_of_cells, 106);
    EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
    ExpectLfpLimits();
  }

  {
    datalayer.battery.info.chemistry = battery_chemistry_enum::NCA;
    LocalPack three_cells;
    three_cells->handle_incoming_can_frame(BrickFrame(0, 0x2A));
    three_cells->handle_incoming_can_frame(BrickFrame(0, 0x2A));
    three_cells->update_values();

    EXPECT_EQ(datalayer.battery.info.number_of_cells, 3);
    EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
    ExpectNca3YLimits();
  }

  {
    datalayer.battery.info.chemistry = battery_chemistry_enum::NCA;
    user_selected_battery_type = BatteryType::TeslaModelSX;
    user_selected_tesla_GTW_chassisType = 0;
    LocalPack model_s;
    ScanFullFrames(model_s.get(), 35);
    model_s->update_values();

    EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
    EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
    ExpectSxLimits();
  }

  {
    datalayer.battery.info.chemistry = battery_chemistry_enum::NCA;
    user_selected_battery_type = BatteryType::TeslaModelSX;
    user_selected_tesla_GTW_chassisType = 1;
    LocalPack model_x;
    ScanFullFrames(model_x.get(), 35);
    model_x->update_values();

    EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
    EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
    ExpectSxLimits();
  }

  {
    datalayer.battery.info.chemistry = battery_chemistry_enum::NCA;
    user_selected_battery_type = BatteryType::TeslaModel3Y;
    user_selected_tesla_GTW_chassisType = 3;
    LocalPack model_y;
    ScanFullFrames(model_y.get(), 35);
    model_y->update_values();

    EXPECT_EQ(datalayer.battery.info.number_of_cells, 108);
    EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
    ExpectLfpLimits();
  }

  {
    user_selected_battery_chemistry = battery_chemistry_enum::LFP;
    datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
    user_selected_battery_type = BatteryType::TeslaModel3Y;
    user_selected_tesla_GTW_chassisType = 2;
    LocalPack stored_lfp;
    ExpectLfpLimits();

    ScanFullFrames(stored_lfp.get(), 31);
    stored_lfp->update_values();

    EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
    EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::LFP);
    ExpectLfpLimits();
  }
}

// State 4 is CLOSED, state 1 is OPEN, state 5 is WELDED. With contactor
// control off, the bus is live only while the BMS says closed. State 5
// latches the welded-contactor event, and the following OPEN leaves that
// latch up. With contactor control on, the board owns dc_bus_live: a BMS
// CLOSED frame leaves a dead bus dead, and a BMS OPEN frame leaves a live
// bus live. Welded still latches on that path, and the bus stays as the
// board left it. clear_event does not drop a latch, so that last pass
// starts from a clear event list.
//
// setup() lets the inverter close this pack. The flag starts false.
// Isolation is on this same frame, 10 kOhm per bit. Raw 0 publishes 0 kOhm
// and still marks the reading available. Raw 16 publishes 160 kOhm.
TEST_F(TeslaBatteryTest, ContactorStatePublishesTheDcBusAndAWeldLatch) {
  datalayer.system.status.battery_allows_contactor_closing = false;
  pack_->setup();
  EXPECT_TRUE(datalayer.system.status.battery_allows_contactor_closing);

  pack_->handle_incoming_can_frame(BmsStatus(1));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);
  EXPECT_TRUE(datalayer.battery.status.insulation_resistance_available);
  EXPECT_EQ(datalayer.battery.status.insulation_resistance_kOhm, 0);

  pack_->handle_incoming_can_frame(BmsStatus(4, 16));
  pack_->update_values();
  EXPECT_TRUE(datalayer.system.status.dc_bus_live);
  EXPECT_EQ(datalayer.battery.status.insulation_resistance_kOhm, 160);

  pack_->handle_incoming_can_frame(BmsStatus(5));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE_LATCHED);

  pack_->handle_incoming_can_frame(BmsStatus(1));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE_LATCHED);

  contactor_control_enabled = true;
  datalayer.system.status.dc_bus_live = false;
  pack_->handle_incoming_can_frame(BmsStatus(4));
  pack_->update_values();
  EXPECT_FALSE(datalayer.system.status.dc_bus_live);

  datalayer.system.status.dc_bus_live = true;
  pack_->handle_incoming_can_frame(BmsStatus(1));
  pack_->update_values();
  EXPECT_TRUE(datalayer.system.status.dc_bus_live);

  reset_all_events();
  datalayer.system.status.dc_bus_live = true;
  pack_->handle_incoming_can_frame(BmsStatus(5));
  pack_->update_values();
  EXPECT_TRUE(datalayer.system.status.dc_bus_live);
  EXPECT_EQ(EventState(EVENT_CONTACTOR_WELDED), EVENT_STATE_ACTIVE_LATCHED);
}

// Still-alive, an opened HVIL, the pyro window, and the 12 V rail. 0x001 is
// not a frame this driver counts, so the timer stays at zero until a pack
// frame arrives. HVIL status 3 means a high-voltage cable was opened while
// the pack was in use. Any other status clears that fault.
//
// The pyro window is 5 dV through 50 dV, including both ends. 40 raw is 4 dV
// and stays clear. 50 raw is 5 dV and raises. 500 raw is 50 dV and stays
// raised. 510 raw is 51 dV and clears, and so does a normal pack voltage.
//
// A 12 V reading of 0 is skipped. It does not raise the low-rail event, and
// it does not clear one that is already up. 116 raw is 11.6 V and raises
// while both pack ends are 0. 117 raw is 11.7 V, which is not under the
// limit, so it clears. A non-zero pack end clears the event even while the
// rail is still 11.6 V.
TEST_F(TeslaBatteryTest, PackFaultsPublishTheEventList) {
  datalayer.battery.status.CAN_battery_still_alive = 0;
  pack_->handle_incoming_can_frame(Frame(0x001));
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, 0);

  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);

  pack_->handle_incoming_can_frame(HvilStatus(3));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_INTERNAL_OPEN_FAULT), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvilStatus(0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_INTERNAL_OPEN_FAULT), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(HvBus(40, 0));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 4);
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(HvBus(50, 0));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 5);
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvBus(500, 0));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 50);
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(HvBus(510, 0));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 51);
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(HvBus(37030, 0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_BATTERY_FUSE), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(PackEnds(0, 0));
  pack_->handle_incoming_can_frame(TwelveVolt(0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(TwelveVolt(116));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(TwelveVolt(0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(TwelveVolt(117));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_INACTIVE);

  pack_->handle_incoming_can_frame(TwelveVolt(116));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_ACTIVE);

  pack_->handle_incoming_can_frame(PackEnds(1, 0));
  pack_->update_values();
  EXPECT_EQ(EventState(EVENT_12V_LOW), EVENT_STATE_INACTIVE);
}

// The frames a Model 3 pack has on the bus, published in one pass. 37030
// centivolts is 3703 dV. The current word is signed: 0xFFD4 is -44 dA, and
// 225 stays a positive charge. 80.0% UI SOC is 8000, above the byte boundary
// at 50%. 12.3% is checked when balancing restores the CAN SOC. Pack energy
// 1 is 74 kWh, so 80.0% is 59200 Wh. Cell min/max use the 2 mV step, and the
// temperature frame that follows them has to leave those cells in place.
// Discharge power is whole amps times whole volts: 430 raw truncates to 55 A
// and 3703 dV / 10 drops the 0.3 V. The override starts high enough to show
// that product. The next pass lowers it, and 20350 W publishes as 5000 W.
// Charge power is the user setting. A 1000 A charge limit on the same frame
// does not change it, and before that setting is raised the charge power
// stays 0. Switching the gateway setting to 100 kWh moves the remaining
// energy and leaves the measurements and the cap in place.
TEST_F(TeslaBatteryTest, Model3YSnapshotPublishesTheInverterPicture) {
  user_selected_tesla_GTW_packEnergy = 1;
  pack_->setup();
  datalayer.battery.status.override_discharge_power_W = 1000000;

  ScanFullFrames(pack_, 31);
  pack_->handle_incoming_can_frame(HvBus(37030, 0xFFD4));
  pack_->handle_incoming_can_frame(SocUi(800));
  pack_->handle_incoming_can_frame(CellMinMax(2001, 1801));
  pack_->handle_incoming_can_frame(BrickTemps(101, 80));
  pack_->handle_incoming_can_frame(CurrentLimits(10000, 430));
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
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 0u);
  EXPECT_EQ(datalayer.battery.info.number_of_cells, 96);
  EXPECT_EQ(datalayer.battery.info.chemistry, battery_chemistry_enum::NCA);
  ExpectNca3YLimits();
  EXPECT_EQ(datalayer.battery.status.CAN_battery_still_alive, CAN_STILL_ALIVE);

  pack_->handle_incoming_can_frame(HvBus(37030, 225));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.current_dA, 225);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 55u * 370u);

  datalayer.battery.status.override_discharge_power_W = 5000;
  datalayer.battery.status.override_charge_power_W = 4000;
  pack_->handle_incoming_can_frame(CurrentLimits(10000, 430));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 4000u);

  user_selected_tesla_GTW_packEnergy = 3;
  pack_->setup();
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.info.total_capacity_Wh, 100000u);
  EXPECT_EQ(datalayer.battery.status.remaining_capacity_Wh, 80000u);
  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
  EXPECT_EQ(datalayer.battery.status.voltage_dV, 3703);
  EXPECT_EQ(datalayer.battery.status.max_discharge_power_W, 5000u);
  ExpectNca3YLimits();
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

// BMS_a145 raises a latched SOC-recalibration event. BMS_a143 on the same
// mux is only an alert bit: SOH stays at the hardcoded 99%, which is how
// this driver says it has no CAC reading. A later frame with both bits
// clear drops the matrix bits. clear_event only clears an active event, so
// the latched recalibration stays up.
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

  pack_->handle_incoming_can_frame(AlertMux2(false, false));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.soh_pptt, 9900);
  EXPECT_FALSE(datalayer_extended.tesla.BMS_alertMatrixActive[72]);
  EXPECT_FALSE(datalayer_extended.tesla.BMS_alertMatrixActive[74]);
  EXPECT_EQ(EventState(EVENT_BATTERY_SOC_RECALIBRATION), EVENT_STATE_ACTIVE_LATCHED);
}

// A balancing request is the pass that may step outside the chemistry
// window. SOC becomes 99%, the max voltage limits and the charge power come
// from the balancing settings, and the minimum limits stay. Clearing the
// request publishes the CAN SOC, the NCA window, and the user charge power.
//
// That override sits inside the Model 3/Y chassis check. Model X is chassis
// 1, the last chassis below it, so a request there leaves the CAN SOC, the
// 460 V window, and the user charge power in place.
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

  user_selected_battery_type = BatteryType::TeslaModelSX;
  user_selected_tesla_GTW_chassisType = 1;
  pack_->setup();
  pack_->handle_incoming_can_frame(SocUi(800));
  datalayer.battery.status.override_charge_power_W = 4000;
  datalayer.battery_settings.balancing_max_pack_voltage_dV = 4100;
  datalayer.battery_settings.balancing_max_cell_voltage_mV = 4300;
  datalayer.battery_settings.balancing_max_deviation_cell_voltage_mV = 600;
  datalayer.battery_settings.balancing_float_power_W = 1500;
  datalayer.battery_settings.user_requests_balancing = true;
  pack_->update_values();

  EXPECT_EQ(datalayer.battery.status.real_soc, 8000);
  ExpectSxLimits();
  EXPECT_EQ(datalayer.battery.status.max_charge_power_W, 4000u);
}

// Frame with the given payload (DLC 8)
static CAN_frame FrameWith(uint32_t id, std::initializer_list<uint8_t> bytes) {
  CAN_frame f = Frame(id);
  int i = 0;
  for (uint8_t b : bytes) {
    f.data.u8[i++] = b;
  }
  return f;
}

// 1023 is not available in the 0x292 SOC fields. The pack SOC is the UI SOC, the last SOC is held while it is not
// available (SOC min is not used, no jump from 27.1 to 30.0 %).
TEST_F(TeslaBatteryTest, SocFieldsIgnoreNotAvailableValues) {
  pack_->handle_incoming_can_frame(SocFields(300, 271, 306, 303));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  for (int second = 0; second < 5; second++) {  // a 5 s burst
    pack_->handle_incoming_can_frame(SocFields(300, 1023, 306, 1023));
    pack_->update_values();
    EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
  }

  pack_->handle_incoming_can_frame(SocFields(1023, 1023, 1023, 1023));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(SocFields(310, 281, 306, 303));  // min above max
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2810);
}

// Before a valid UI SOC was ever seen SOC min is the initial SOC (0 would zero the discharge limit), a valid UI SOC
// replaces it for good.
TEST_F(TeslaBatteryTest, SocMinIsOnlyTheInitialSocBeforeAValidUiSoc) {
  pack_->handle_incoming_can_frame(SocFields(300, 1023, 306, 303));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 3000);

  pack_->handle_incoming_can_frame(SocFields(300, 271, 306, 303));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(SocFields(300, 1023, 306, 303));
  pack_->update_values();
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);
}

// 30 s without a valid UI SOC raise EVENT_SOC_UNAVAILABLE (a WARNING), the SOC is still the last one. A valid UI SOC
// clears it.
TEST_F(TeslaBatteryTest, SocUnavailableEventAfter30SecondsAndClearedByAValidSoc) {
  clear_event(EVENT_SOC_UNAVAILABLE);
  pack_->handle_incoming_can_frame(SocFields(300, 271, 306, 303));
  pack_->update_values();

  for (int second = 1; second <= 29; second++) {
    pack_->handle_incoming_can_frame(SocFields(300, 1023, 306, 303));
    pack_->update_values();
  }
  EXPECT_NE(get_event_pointer(EVENT_SOC_UNAVAILABLE)->state, EVENT_STATE_ACTIVE);  // 29 s

  pack_->handle_incoming_can_frame(SocFields(300, 1023, 306, 303));
  pack_->update_values();
  EXPECT_EQ(get_event_pointer(EVENT_SOC_UNAVAILABLE)->state, EVENT_STATE_ACTIVE);  // 30 s
  EXPECT_EQ(get_event_pointer(EVENT_SOC_UNAVAILABLE)->data, 30);
  EXPECT_EQ(get_event_pointer(EVENT_SOC_UNAVAILABLE)->level, EVENT_LEVEL_WARNING);
  EXPECT_EQ(datalayer.battery.status.real_soc, 2710);

  pack_->handle_incoming_can_frame(SocFields(300, 271, 306, 303));
  pack_->update_values();
  EXPECT_EQ(get_event_pointer(EVENT_SOC_UNAVAILABLE)->state, EVENT_STATE_INACTIVE);
}

// Pre-2021 0x352 has no mux, 11 bit fields in 0.1 kWh at bit 0, 11, 22, 33 and 44, buffer at bit 55 (8 bits), fully
// charged at bit 63. The frame is a real one from a 72.1 kWh pack at 27.1 % (ScanMyTesla: full 72.1, remaining 19.6,
// buffer 6.0).
TEST_F(TeslaBatteryTest, NoMuxEnergyStatusDecodesTheDbcFields) {
  const uint8_t real[8] = {0xD1, 0x22, 0x46, 0x30, 0x82, 0x01, 0x00, 0x1E};
  const TeslaBattery::EnergyStatusNoMux e = TeslaBattery::decode_energy_status_no_mux(real);
  EXPECT_EQ(e.nominal_full, 721);
  EXPECT_EQ(e.nominal_remaining, 196);
  EXPECT_EQ(e.expected_remaining, 193);
  EXPECT_EQ(e.ideal_remaining, 193);
  EXPECT_EQ(e.to_charge_complete, 0);
  EXPECT_EQ(e.buffer, 60);
  EXPECT_FALSE(e.fully_charged);

  const uint8_t last_bit[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  const TeslaBattery::EnergyStatusNoMux all = TeslaBattery::decode_energy_status_no_mux(last_bit);
  EXPECT_EQ(all.nominal_full, 2047);
  EXPECT_EQ(all.to_charge_complete, 2047);
  EXPECT_EQ(all.buffer, 255);
  EXPECT_TRUE(all.fully_charged);
}

// The driver takes a frame as no mux format when the two low bits of byte 0 are 2 or 3 and no mux 0 or 1 frame was
// seen. The real frame above has bits 01 in byte 0 (72.1 kWh) so this test uses 72.2 kWh. The values go to the web
// page only.
TEST_F(TeslaBatteryTest, NoMuxEnergyStatusReachesTheExtendedData) {
  pack_->handle_incoming_can_frame(FrameWith(0x352, {0xD2, 0x22, 0x46, 0x30, 0x82, 0x01, 0x00, 0x9E}));
  pack_->update_values();
  EXPECT_FALSE(datalayer_extended.tesla.BMS352_mux);
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_full_pack_energy, 722);
  EXPECT_EQ(datalayer_extended.tesla.battery_nominal_energy_remaining, 196);
  EXPECT_EQ(datalayer_extended.tesla.battery_ideal_energy_remaining, 193);
  EXPECT_EQ(datalayer_extended.tesla.battery_energy_to_charge_complete, 0);
  EXPECT_EQ(datalayer_extended.tesla.battery_energy_buffer, 60);
  EXPECT_TRUE(datalayer_extended.tesla.battery_fully_charged);  // bit 63 is set in byte 7 (0x9E)
}

// 0x352 mux 1: the fully charged flag is bit 15, bit 7 of byte 1. Bit 8, bit 0 of byte 1, is not the flag.
TEST_F(TeslaBatteryTest, FullyChargedIsBit15OfTheMux1Frame) {
  pack_->handle_incoming_can_frame(FrameWith(0x352, {0x01, 0x80, 0x12, 0x01, 0x16, 0x0A, 0x00, 0x00}));
  pack_->update_values();
  EXPECT_TRUE(datalayer_extended.tesla.battery_fully_charged);

  pack_->handle_incoming_can_frame(FrameWith(0x352, {0x01, 0x01, 0x12, 0x01, 0x16, 0x0A, 0x00, 0x00}));
  pack_->update_values();
  EXPECT_FALSE(datalayer_extended.tesla.battery_fully_charged);
}

// Pack temperature is the 0x332 mux 0 thermistor min/max (20.0, 23.5 degC), not the two frame types sharing ID 0x312.
TEST_F(TeslaBatteryTest, PackTempMinMaxComeFromThermistorFrameNotFrom0x312) {
  pack_->update_values();
  EXPECT_FALSE(datalayer_extended.tesla.temperature_seen);  // the web pages show "no data" until 0x332 arrives

  pack_->handle_incoming_can_frame(FrameWith(0x332, {0x04, 0x03, 0x7F, 0x78, 0x7B, 0x00}));
  pack_->handle_incoming_can_frame(FrameWith(0x312, {0xA9, 0x21, 0x48, 0x17, 0x40, 0x8D, 0x00, 0x00}));
  pack_->handle_incoming_can_frame(FrameWith(0x312, {0x00, 0xE5, 0x66, 0x04, 0x2B, 0xE1, 0x13, 0x00}));
  pack_->update_values();

  EXPECT_TRUE(datalayer_extended.tesla.temperature_seen);
  EXPECT_EQ(datalayer.battery.status.temperature_min_dC, 200);
  EXPECT_EQ(datalayer.battery.status.temperature_max_dC, 235);
}
