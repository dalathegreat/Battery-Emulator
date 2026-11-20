#include <gtest/gtest.h>

#include <Arduino.h>  // set_millis64(), from test/emul

#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/devboard/utils/system_clock.h"
#include "../Software/src/inverter/BYD-MODBUS.h"
#include "../Software/src/inverter/INVERTERS.h"

/* Emulator errors become FaultBuffers entries (1000-1099), which a Fronius inverter reads every 20 s
   and lists in its event log with the texts of its BYD1 table. Each entry is dated by the ESP32's
   clock, so one raised before the inverter has sent its time is back-dated once it has. */
namespace {

class BydModbusFaultProbe : public BydModbusInverter {
 public:
  using BydModbusInverter::handle_static_data;
  using BydModbusInverter::mbPV;

  // UTC as the inverter writes it: Unix seconds in a uint64, high word first
  void write_utc(uint32_t epoch_s) {
    mbPV[403] = 0;
    mbPV[404] = 0;
    mbPV[405] = epoch_s >> 16;
    mbPV[406] = epoch_s & 0xFFFF;
  }

  uint32_t u32(uint16_t reg) { return ((uint32_t)mbPV[reg] << 16) | mbPV[reg + 1]; }
};

class BydModbusFaultTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_millis64(1000);
    init_events();  // Event levels are only assigned here, and only error events become faults
    inverter_modbus_utc_epoch_s = 0;
    clock_valid = false;
  }

  void TearDown() override {
    inverter_modbus_utc_epoch_s = 0;
    clock_valid = false;
    set_millis64(0);
  }

  BydModbusFaultProbe inverter;
};

}  // namespace

// Created with the static data, before the Modbus task starts: a register the map does not hold
// would be inserted by the first read, from that task
TEST_F(BydModbusFaultTest, FaultBuffersExistBeforeTheInverterReadsThem) {
  inverter.handle_static_data();

  for (uint16_t reg = 1000; reg < 1100; reg++) {
    ASSERT_EQ(inverter.mbPV.count(reg), 1u) << reg;
    EXPECT_EQ(inverter.mbPV[reg], 0) << reg;
  }
}

TEST_F(BydModbusFaultTest, ErrorEventBecomesAFaultBufferEntryOnceTheClockIsKnown) {
  set_event(EVENT_BATTERY_OVERHEAT, 512, 2);  // Pack 2 at 51.2 degC
  ASSERT_EQ(get_event_pointer(EVENT_BATTERY2_OVERHEAT)->level, EVENT_LEVEL_ERROR);

  inverter.update_values();
  EXPECT_EQ(inverter.u32(1002), 0u);  // No timestamp to give it yet, so the inverter takes it as empty

  inverter.write_utc(1794299900);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1000), 0u);           // Fault_0_Timestamp, high half
  EXPECT_EQ(inverter.u32(1002), 1794299900u);  // Fault_0_Timestamp, low half
  EXPECT_EQ(inverter.mbPV[1004], 1);           // Number
  EXPECT_EQ(inverter.mbPV[1005], 27);          // Type, BYD1-27 "Charging overtemperature 3"
  EXPECT_EQ(inverter.mbPV[1006], 0);           // Severity, Fault2
  EXPECT_EQ(inverter.mbPV[1007], 512);
  EXPECT_EQ(inverter.mbPV[1009], 0x02FF);  // Pack 2

  // Still active: reported once, not on every pass
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1012), 0u);

  // Gone and back: a new entry in the next slot, stamped with the time that has passed
  clear_event(EVENT_BATTERY_OVERHEAT, 2);
  inverter.update_values();
  set_millis64(1000 + 5000);
  set_event(EVENT_BATTERY_OVERHEAT, 520, 2);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1012), 1794299905u);
  EXPECT_EQ(inverter.mbPV[1014], 2);
}

// Raised before the inverter has sent its time: held in the buffers and back-dated once it has,
// including a fault that is over by then
TEST_F(BydModbusFaultTest, FaultsBeforeTheClockAreBackDatedOnceItIsKnown) {
  set_event(EVENT_BATTERY_OVERHEAT, 512, 1);
  inverter.update_values();
  clear_event(EVENT_BATTERY_OVERHEAT, 1);  // Over before the clock comes
  inverter.update_values();

  set_millis64(1000 + 20000);
  set_event(EVENT_CELL_CRITICAL_OVER_VOLTAGE, 4250, 1);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1002), 0u);  // Neither stamped yet
  EXPECT_EQ(inverter.u32(1012), 0u);

  set_millis64(1000 + 50000);
  inverter.write_utc(1794299900);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1002), 1794299900u - 50);  // Fault_0: the overheat, 50 s ago
  EXPECT_EQ(inverter.mbPV[1005], 27);
  EXPECT_EQ(inverter.u32(1012), 1794299900u - 30);  // Fault_1: the cell overvoltage, 30 s ago
  EXPECT_EQ(inverter.mbPV[1015], 24);               // BYD1-24 "Cell overvoltage 3"

  // From here on a new fault gets the time at once
  set_millis64(1000 + 60000);
  set_event(EVENT_HVIL_FAILURE, 0, 1);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1022), 1794299900u + 10);
  EXPECT_EQ(inverter.mbPV[1025], 47);  // BYD1-47 "Hardware fault at the battery control unit (BCU)"
}

// The fault entries take their time from the ESP32's clock, not from the UTC registers
TEST_F(BydModbusFaultTest, FaultsAreStampedFromTheClock) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_millis64(1000 + 5000);
  set_event(EVENT_HVIL_FAILURE, 0, 1);
  inverter.update_values();

  EXPECT_EQ(inverter.u32(1002), 1794299905u);
  EXPECT_EQ(inverter.mbPV[1005], 47);
}

// The two that read differently in BYD1 than in the table of the newer BYD batteries
TEST_F(BydModbusFaultTest, ThermalRunawayIsSystemOvertemperatureAndEquipmentStopTheMainSwitch) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_THERMAL_RUNAWAY, 0, 3);
  set_event(EVENT_EQUIPMENT_STOP, 1);
  inverter.update_values();

  EXPECT_EQ(inverter.mbPV[1005], 53);      // BYD1-53 "System overtemperature"
  EXPECT_EQ(inverter.mbPV[1009], 0x03FF);  // Pack 3
  EXPECT_EQ(inverter.mbPV[1015], 45);      // BYD1-45 "Battery main switch"
  EXPECT_EQ(inverter.mbPV[1019], 0x01FF);
}

// Ten entries, so the eleventh fault takes the place of the first
TEST_F(BydModbusFaultTest, TheEleventhFaultTakesTheOldestEntry) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  for (uint32_t i = 0; i < 11; i++) {
    set_millis64(1000 + i * 1000);
    set_event(EVENT_HVIL_FAILURE, 0, 1);
    inverter.update_values();
    clear_event(EVENT_HVIL_FAILURE, 1);
    inverter.update_values();
  }

  EXPECT_EQ(inverter.mbPV[1004], 11);  // Fault_0 holds the eleventh now
  EXPECT_EQ(inverter.u32(1002), 1794299910u);
  EXPECT_EQ(inverter.mbPV[1014], 2);  // Fault_1 still the second
  EXPECT_EQ(inverter.mbPV[1094], 10);
}

// Warnings stay out of the inverter's event log
TEST_F(BydModbusFaultTest, WarningsAreNotReported) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_BATTERY_ISOLATION, 120, 1);
  ASSERT_EQ(get_event_pointer(EVENT_BATTERY_ISOLATION)->level, EVENT_LEVEL_WARNING);
  inverter.update_values();

  EXPECT_EQ(inverter.u32(1002), 0u);
  EXPECT_EQ(inverter.mbPV[1004], 0);
}
