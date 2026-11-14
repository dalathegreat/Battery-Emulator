#include <gtest/gtest.h>

#include <string>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/events.h"
#include "../Software/src/inverter/FRONIUS-GENERIC-STORAGE.h"
#include "../Software/src/inverter/INVERTERS.h"

/* Fronius Generic Storage, register map version 2. These pin the register positions the inverter
   reads (they all moved from version 1, which the BYD-Modbus driver keeps speaking), the contactor
   decision taken from opMode, and the translation of emulator errors into fault buffer entries. */
namespace {

class FroniusProbe : public FroniusGenericStorageInverter {
 public:
  using FroniusGenericStorageInverter::handle_static_data;
  using FroniusGenericStorageInverter::mbPV;

  // A request from the inverter, as any of the FC workers registers it
  void inverter_talks() { notify_inverter_communication(); }

  void write_op_mode(uint16_t mode) {
    inverter_talks();
    mbPV[500] = mode;
  }

  void write_utc(uint32_t epoch_s) {
    mbPV[503] = 0;
    mbPV[504] = 0;
    mbPV[505] = epoch_s >> 16;
    mbPV[506] = epoch_s & 0xFFFF;
  }

  uint32_t u32(uint16_t reg) { return ((uint32_t)mbPV[reg] << 16) | mbPV[reg + 1]; }

  std::string text(uint16_t reg, uint8_t registers) {
    std::string out;
    for (uint8_t i = 0; i < registers; i++) {
      for (char c : {(char)(mbPV[reg + i] >> 8), (char)(mbPV[reg + i] & 0xFF)}) {
        if (c == 0) {
          return out;
        }
        out += c;
      }
    }
    return out;
  }
};

class FroniusGenericStorageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_millis64(1000);
    init_events();  // Event levels are only assigned here, and only error events become faults
    user_selected_inverter_contactor_mode = inverter_contactor_mode_enum::NoWorkaround;
    user_selected_fronius_iso_check = false;
    user_selected_primo_gen24 = false;
    user_selected_accept_inverter_reboot = false;
    inverter_modbus_utc_epoch_s = 0;
    inverter_modbus_watchdog_timeout_s = MODBUS_INV_WATCHDOG_DEFAULT_S;
    inverter_modbus_watchdog_changed = false;
    datalayer.system.status.system_status = ACTIVE;
    datalayer.system.status.dc_bus_live = false;
    datalayer.aggregate.voltage_dV = 3800;
    inverter.handle_static_data();
  }

  void TearDown() override {
    user_selected_inverter_contactor_mode = inverter_contactor_mode_enum::NoWorkaround;
    user_selected_fronius_iso_check = false;
    inverter_modbus_utc_epoch_s = 0;
    inverter_modbus_watchdog_timeout_s = MODBUS_INV_WATCHDOG_DEFAULT_S;
    inverter_modbus_watchdog_changed = false;
    set_millis64(0);
  }

  FroniusProbe inverter;
};

}  // namespace

TEST_F(FroniusGenericStorageTest, IdentifiesAsItselfOnTheVersion2Map) {
  EXPECT_EQ(inverter.u32(100), 0x53490001u);  // InterfaceID
  EXPECT_EQ(inverter.text(102, 16), "BatteryEmulator");
  EXPECT_EQ(inverter.text(150, 16), "BE3C8427C5A1F0");  // From the MAC address of test/emul/esp_mac.h
  EXPECT_EQ(inverter.mbPV[166], 2);                     // RegMapVrMajor
  EXPECT_EQ(inverter.mbPV[167], 0);
  EXPECT_EQ(inverter.text(218, 4), "BYD2");  // MsgPrefix
  EXPECT_EQ(inverter.mbPV[2000], 0);         // UPD_TYPE
}

TEST_F(FroniusGenericStorageTest, CapacityAndLimitsAreNotCappedTo16Bit) {
  datalayer.aggregate.reported_total_capacity_Wh = 82000;
  datalayer.aggregate.reported_remaining_capacity_Wh = 41000;
  datalayer.aggregate.max_charge_power_W = 70000;
  datalayer.aggregate.max_discharge_power_W = 90000;

  inverter.update_values();

  EXPECT_EQ(inverter.u32(302), 82000u);  // NominalCapacity
  EXPECT_EQ(inverter.u32(404), 82000u);  // CurrTotalCapacity
  EXPECT_EQ(inverter.u32(406), 41000u);  // CurrRestCapacity
  EXPECT_EQ(inverter.u32(408), 90000u);  // CurrMaxDischargePwr
  EXPECT_EQ(inverter.u32(410), 70000u);  // CurrMaxChargePwr
}

TEST_F(FroniusGenericStorageTest, MonitoringValuesLandOnTheirRegisters) {
  datalayer.aggregate.reported_soc = 6700;
  datalayer.aggregate.current_dA = 100;  // Charging
  datalayer.aggregate.active_power_W = 3800;
  datalayer.aggregate.temperature_min_dC = 180;
  datalayer.aggregate.temperature_max_dC = 210;
  datalayer.aggregate.soh_pptt = 9500;

  inverter.update_values();

  EXPECT_EQ(inverter.u32(401), 0x81u);           // flags: link OK and charging
  EXPECT_EQ(inverter.mbPV[403], 6700);           // SOC
  EXPECT_EQ(inverter.mbPV[415], 3800);           // UDC_internal
  EXPECT_EQ((int32_t)inverter.u32(416), -3800);  // PDC_internal, charging is negative
  EXPECT_EQ(inverter.mbPV[418], 180);
  EXPECT_EQ(inverter.mbPV[419], 210);
  EXPECT_EQ(inverter.mbPV[428], 210);   // TAmbient, the warmest cell
  EXPECT_EQ(inverter.mbPV[429], 9500);  // SOH

  datalayer.aggregate.reported_soc = 10000;
  datalayer.aggregate.current_dA = -50;
  inverter.update_values();
  EXPECT_EQ(inverter.u32(401), 0x82u);  // link OK and discharging. No other bit, full or not
}

TEST_F(FroniusGenericStorageTest, NameplateCarriesTheTemperatureRangeAndNoCalibration) {
  inverter.update_values();

  EXPECT_EQ(inverter.u32(300), 0u);              // BatteryCapabilityFlags
  EXPECT_EQ((int16_t)inverter.mbPV[319], -250);  // TCellMin
  EXPECT_EQ((int16_t)inverter.mbPV[320], 500);   // TCellMax
  EXPECT_EQ(inverter.u32(321), 1u);              // CalMode, NoCalibrationRoutine
  EXPECT_EQ(inverter.mbPV[333], 1);              // NumberParallelStacks

  user_selected_fronius_iso_check = true;
  inverter.update_values();
  EXPECT_EQ(inverter.u32(300), 4u);  // InverterIsolationCheck
}

TEST_F(FroniusGenericStorageTest, ContactorsStayOpenUntilTheInverterAsksForTheBattery) {
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_STANDBY);

  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_STANDBY);
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());

  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_ACTIVE);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  // Asked for, not yet there: precharge is still running
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_STANDBY);
  EXPECT_EQ(inverter.mbPV[412], 0);  // UDC_Bus

  datalayer.system.status.dc_bus_live = true;
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_ACTIVE);
  EXPECT_EQ(inverter.mbPV[412], 3800);
}

// What a GEN24 does first with a battery. It has to find the battery connected and ACTIVE after
// that: answered with INACTIVE it reports GEN24-1250 "DC connection between inverter and battery
// interrupted" and falls back to STANDBY.
TEST_F(FroniusGenericStorageTest, InactiveRequestBringsTheBatteryUpAsActive) {
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_INACTIVE);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_STANDBY);  // Not there yet

  datalayer.system.status.dc_bus_live = true;
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_ACTIVE);
  EXPECT_EQ(inverter.mbPV[412], 3800);  // UDC_Bus
}

TEST_F(FroniusGenericStorageTest, DarkstartRequestClosesTheContactorsToo) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_DARKSTART);
  inverter.update_values();

  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_ACTIVE);
}

TEST_F(FroniusGenericStorageTest, StandbyRequestOpensOnceTheCurrentHasGone) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_ACTIVE);
  inverter.update_values();

  datalayer.aggregate.current_dA = -150;  // Still discharging at 15 A
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_STANDBY);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_ACTIVE);

  datalayer.aggregate.current_dA = -5;
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_STANDBY);
}

TEST_F(FroniusGenericStorageTest, StandbyRequestOpensAfterTheWaitEvenUnderCurrent) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_ACTIVE);
  inverter.update_values();

  datalayer.aggregate.current_dA = 150;
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_STANDBY);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());

  set_millis64(1000 + 10000);
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, SilentInverterLosesItsRequest) {
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_ACTIVE);
  inverter.update_values();
  ASSERT_TRUE(inverter.allows_contactor_closing());

  set_millis64(1000 + 60001);  // Not a single request for a minute
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());

  inverter.inverter_talks();  // Back again, and the last opMode still stands
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, AlwaysClosedWorkaroundBehavesLikeBydModbus) {
  user_selected_inverter_contactor_mode = inverter_contactor_mode_enum::AlwaysClosed;
  datalayer.system.status.dc_bus_live = true;

  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_ACTIVE);

  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_STANDBY);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_ACTIVE);
  EXPECT_EQ(inverter.mbPV[412], 3800);
}

TEST_F(FroniusGenericStorageTest, LockWorkaroundKeepsTheContactorsClosedAfterTheFirstRequest) {
  user_selected_inverter_contactor_mode = inverter_contactor_mode_enum::LockAfterFirstClose;

  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());

  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_ACTIVE);
  inverter.update_values();
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_STANDBY);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, EmulatorFaultIsReportedWhateverTheInverterAsked) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FroniusGenericStorageInverter::MODE_ACTIVE);
  datalayer.system.status.system_status = FAULT;
  inverter.update_values();

  EXPECT_EQ(inverter.mbPV[400], FroniusGenericStorageInverter::MODE_FAULT);
}

TEST_F(FroniusGenericStorageTest, ControlDataIsReadFromThe500Block) {
  inverter.mbPV[502] = 120;  // WatchDogTimeout
  inverter.write_utc(1794299900);
  inverter.update_values();

  EXPECT_EQ(inverter_modbus_watchdog_timeout_s, 120u);
  EXPECT_TRUE(inverter_modbus_watchdog_changed);
  EXPECT_EQ(inverter_modbus_utc_epoch_s, 1794299900u);
}

TEST_F(FroniusGenericStorageTest, InverterDetailsAreDecoded) {
  inverter.mbPV[3000] = ('1' << 8) | '.';
  inverter.mbPV[3001] = ('3' << 8) | '8';
  inverter.mbPV[3010] = ('G' << 8) | 'E';
  inverter.mbPV[3011] = ('N' << 8) | '<';  // Markup must not reach the web page
  inverter.update_values();

  EXPECT_STREQ(inverter.inverter_firmware(), "1.38");
  EXPECT_STREQ(inverter.inverter_name(), "GEN?");
}

TEST_F(FroniusGenericStorageTest, ErrorEventBecomesAFaultBufferEntryOnceTheClockIsKnown) {
  set_event(EVENT_BATTERY_OVERHEAT, 512, 2);  // Pack 2 at 51.2 degC
  ASSERT_EQ(get_event_pointer(EVENT_BATTERY2_OVERHEAT)->level, EVENT_LEVEL_ERROR);

  inverter.update_values();
  EXPECT_EQ(inverter.u32(1002), 0u);  // No timestamp to give it yet

  inverter.write_utc(1794299900);
  inverter.update_values();
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

// The BYD-Modbus driver shares the ControlData handling and must keep reading it where version 1
// of the map has it
TEST(BydModbusControlBlockTest, ControlDataIsStillReadFromThe400Block) {
  class Probe : public BydModbusInverter {
   public:
    using BydModbusInverter::mbPV;
  };
  inverter_modbus_watchdog_timeout_s = MODBUS_INV_WATCHDOG_DEFAULT_S;
  inverter_modbus_watchdog_changed = false;
  Probe byd;

  byd.mbPV[502] = 90;  // Not a register of version 1
  byd.update_values();
  EXPECT_EQ(inverter_modbus_watchdog_timeout_s, MODBUS_INV_WATCHDOG_DEFAULT_S);

  byd.mbPV[402] = 120;  // WatchDogTimeout
  byd.update_values();
  EXPECT_EQ(inverter_modbus_watchdog_timeout_s, 120u);

  inverter_modbus_watchdog_timeout_s = MODBUS_INV_WATCHDOG_DEFAULT_S;
  inverter_modbus_watchdog_changed = false;
}
