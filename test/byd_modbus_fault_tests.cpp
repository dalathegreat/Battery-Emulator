#include <gtest/gtest.h>

#include <Arduino.h>  // set_millis64(), from test/emul

#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/devboard/utils/system_clock.h"
#include "../Software/src/inverter/BYD-MODBUS.h"

/* Emulator errors, and a few warnings, become FaultBuffers entries (1000-1099), which a Fronius
   inverter reads every 20 s and lists in its event log with the texts of its BYD2 table. Each entry
   is dated by the ESP32's clock, so one raised before the inverter has sent its time is back-dated
   once it has. */
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
    init_events();  // Event levels are only assigned here
    clock_valid = false;
    // The watchdog value of a talking inverter, so that one minute in it does not go missing
    inverter.mbPV[401] = 0x00FF;
  }

  void TearDown() override {
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
  EXPECT_EQ(inverter.mbPV[1005], 27);          // Type, BYD2-27 "Charge Over Temperature 3"
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
  EXPECT_EQ(inverter.mbPV[1015], 24);               // BYD2-24 "Cell Over Voltage 3"

  // From here on a new fault gets the time at once
  set_millis64(1000 + 60000);
  set_event(EVENT_HVIL_FAILURE, 0, 1);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1022), 1794299900u + 10);
  EXPECT_EQ(inverter.mbPV[1025], 49);  // BYD2-49 "Faulty wiring of the battery supply line"
}

// The fault entries take their time from the ESP32's clock, not from the UTC registers
TEST_F(BydModbusFaultTest, FaultsAreStampedFromTheClock) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_millis64(1000 + 5000);
  set_event(EVENT_HVIL_FAILURE, 0, 1);
  inverter.update_values();

  EXPECT_EQ(inverter.u32(1002), 1794299905u);
  EXPECT_EQ(inverter.mbPV[1005], 49);
}

TEST_F(BydModbusFaultTest, EquipmentStopIsReportedAsTheMainSwitchTripped) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_EQUIPMENT_STOP, 1);
  inverter.update_values();

  EXPECT_EQ(inverter.u32(1002), 1794299900u);
  EXPECT_EQ(inverter.mbPV[1005], 45);  // BYD2-45 "Battery main switch has tripped"
  EXPECT_EQ(inverter.mbPV[1006], 0);   // An error, so a Fault2
  EXPECT_EQ(inverter.mbPV[1009], 0x01FF);
}

TEST_F(BydModbusFaultTest, ThermalRunawayOfPack3IsReportedAsOverTemperature) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_THERMAL_RUNAWAY, 0, 3);
  inverter.update_values();

  EXPECT_EQ(inverter.mbPV[1005], 27);      // BYD2-27 "The maximum cell temperature is more than 70..."
  EXPECT_EQ(inverter.mbPV[1009], 0x03FF);  // Pack 3
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

// A warning or info event in the map goes in as a Fault1, one not in the map not at all
TEST_F(BydModbusFaultTest, WarningsAndInfosGoInAsFault1) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_BATTERY_TEMP_DEVIATION_HIGH, 120, 1);  // Not in the map
  ASSERT_EQ(get_event_pointer(EVENT_BATTERY_TEMP_DEVIATION_HIGH)->level, EVENT_LEVEL_WARNING);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[1004], 0);  // No entry

  set_event(EVENT_BATTERY_OVERVOLTAGE, 4100, 1);
  ASSERT_EQ(get_event_pointer(EVENT_BATTERY_OVERVOLTAGE)->level, EVENT_LEVEL_WARNING);
  set_event(EVENT_CHARGE_LIMIT_EXCEEDED, 0, 2);
  ASSERT_EQ(get_event_pointer(EVENT_CHARGE_LIMIT_EXCEEDED_BAT2)->level, EVENT_LEVEL_INFO);
  inverter.update_values();

  // In the order of the map
  EXPECT_EQ(inverter.u32(1002), 1794299900u);
  EXPECT_EQ(inverter.mbPV[1005], 18);  // BYD2-18 "Charge Over Current"
  EXPECT_EQ(inverter.mbPV[1006], 1);   // Severity, Fault1
  EXPECT_EQ(inverter.mbPV[1009], 0x02FF);
  EXPECT_EQ(inverter.mbPV[1015], 24);  // BYD2-24 "Cell Over Voltage 3"
  EXPECT_EQ(inverter.mbPV[1016], 1);
  EXPECT_EQ(inverter.mbPV[1017], 4100);
  EXPECT_EQ(inverter.mbPV[1024], 0);  // Still nothing for the temperature deviation
}

TEST_F(BydModbusFaultTest, OpenContactorIsReportedAsTheMainSwitchTripped) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_CONTACTOR_OPEN, 0, 1);
  inverter.update_values();

  EXPECT_EQ(inverter.mbPV[1005], 45);  // BYD2-45 "Battery main switch has tripped"
  EXPECT_EQ(inverter.mbPV[1006], 1);   // A warning, so a Fault1
}

TEST_F(BydModbusFaultTest, ColdPackAndWeldedContactorAreReported) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_BATTERY_COLD, 0, 2);
  set_event(EVENT_CONTACTOR_WELDED, 0, 1);
  inverter.update_values();

  // In the order of the map
  EXPECT_EQ(inverter.mbPV[1005], 29);  // BYD2-29 "Charge Under Temperature 3"
  EXPECT_EQ(inverter.mbPV[1006], 1);   // An info event, so a Fault1
  EXPECT_EQ(inverter.mbPV[1009], 0x02FF);
  EXPECT_EQ(inverter.mbPV[1015], 47);  // BYD2-47 "Internal hardware failure at the battery control unit (BCU)"
  EXPECT_EQ(inverter.mbPV[1016], 1);
}

// The main pack gone missing is a communication error of the BCU, a secondary one that of one of
// its modules
TEST_F(BydModbusFaultTest, AMissingPackIsReportedByWhichOneItIs) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_CAN_BATTERY3_MISSING, 0);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[1004], 1);
  EXPECT_EQ(inverter.mbPV[1005], 0);       // BYD2-0 "The BCU cannot communicate with one of the battery modules"
  EXPECT_EQ(inverter.mbPV[1006], 1);       // A warning, so a Fault1
  EXPECT_EQ(inverter.mbPV[1009], 0x03FF);  // Pack 3

  set_event(EVENT_CAN_BATTERY_MISSING, 0);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[1015], 1);  // BYD2-1 "BCU communication error"
  EXPECT_EQ(inverter.mbPV[1016], 0);  // An error, so a Fault2
  EXPECT_EQ(inverter.mbPV[1019], 0x01FF);
}

TEST_F(BydModbusFaultTest, TheInverterGoneQuietIsACommunicationError) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  inverter.update_values();

  EXPECT_EQ(inverter.mbPV[1005], 46);  // BYD2-46 "Communication error between inverter and battery"
  EXPECT_EQ(inverter.mbPV[1006], 0);   // An error, so a Fault2
}

// The cause of each boot is raised as an event. An unexpected one is reported, as a restart
TEST_F(BydModbusFaultTest, AnUnexpectedRestartIsReported) {
  ASSERT_TRUE(clock_set_utc(1794299900));
  set_event(EVENT_RESET_TASK_WDT, 6);  // ESP_RST_TASK_WDT
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[1004], 1);
  EXPECT_EQ(inverter.mbPV[1005], 46);  // BYD2-46 "A restart process has been initiated"
  EXPECT_EQ(inverter.mbPV[1006], 1);   // A warning, so a Fault1
  EXPECT_EQ(inverter.mbPV[1007], 6);

  set_event(EVENT_RESET_CPU_LOCKUP, 15);  // The last of the runs in the map
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[1015], 46);

  // A restart that was asked for is not reported
  set_event(EVENT_RESET_SW, 3);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[1024], 0);
}
