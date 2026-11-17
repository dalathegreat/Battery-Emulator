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

  // A poll of MonitoringData through the real read handler, as a GEN24 makes it
  void inverter_reads_monitoring() {
    ModbusMessage request(21, 0x03, uint16_t{400}, uint16_t{30});
    FC03(request);
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
    datalayer.aggregate.current_dA = 0;
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
  EXPECT_EQ(inverter.mbPV[428], 0x8000);  // TAmbient, not available
  EXPECT_EQ(inverter.mbPV[429], 9500);    // SOH

  datalayer.aggregate.reported_soc = 10000;
  datalayer.aggregate.current_dA = -50;
  inverter.update_values();
  EXPECT_EQ(inverter.u32(401), 0x86u);  // link OK, full and discharging
}

TEST_F(FroniusGenericStorageTest, NameplateCarriesTheTemperatureRangeAndNoCalibration) {
  inverter.update_values();

  EXPECT_EQ(inverter.u32(300), 0u);              // BatteryCapabilityFlags
  EXPECT_EQ((int16_t)inverter.mbPV[319], -250);  // TCellMin
  EXPECT_EQ((int16_t)inverter.mbPV[320], 500);   // TCellMax
  EXPECT_EQ(inverter.u32(321), 1u);              // CalMode, NoCalibrationRoutine
  EXPECT_EQ(inverter.mbPV[333], 1);              // NumberParallelStacks

  // A setting, read at boot: the flags are written once, with the rest of the static data
  user_selected_fronius_iso_check = true;
  inverter.update_values();
  EXPECT_EQ(inverter.u32(300), 0u);
  inverter.handle_static_data();
  EXPECT_EQ(inverter.u32(300), 4u);  // InverterIsolationCheck
}

// Unsigned in the map, but some batteries count discharged energy downwards
TEST_F(FroniusGenericStorageTest, EnergyCountersAreSentWithoutTheirSign) {
  datalayer.aggregate.total_charged_battery_Wh = 1234567;
  datalayer.aggregate.total_discharged_battery_Wh = -1111111;
  inverter.update_values();

  EXPECT_EQ(inverter.u32(420), 0u);  // TotalEnergyCharged, high half
  EXPECT_EQ(inverter.u32(422), 1234567u);
  EXPECT_EQ(inverter.u32(424), 0u);  // TotalEnergyDischarged, high half
  EXPECT_EQ(inverter.u32(426), 1111111u);

  datalayer.aggregate.total_charged_battery_Wh = 0;
  datalayer.aggregate.total_discharged_battery_Wh = 0;
}

// The inverter is set up before the battery, so these are still empty when the static data is
// written, and they move at runtime as packs join or the capacity is learnt
TEST_F(FroniusGenericStorageTest, NameplateFollowsTheBatteryOnceItHasReported) {
  datalayer.aggregate.reported_total_capacity_Wh = 0;
  datalayer.aggregate.max_design_voltage_dV = 0;
  datalayer.aggregate.min_design_voltage_dV = 0;
  inverter.handle_static_data();
  EXPECT_EQ(inverter.u32(302), 0u);

  datalayer.aggregate.reported_total_capacity_Wh = 24000;
  datalayer.aggregate.max_design_voltage_dV = 4040;
  datalayer.aggregate.min_design_voltage_dV = 2600;
  inverter.update_values();
  EXPECT_EQ(inverter.u32(302), 24000u);  // NominalCapacity
  EXPECT_EQ(inverter.mbPV[308], 4040);   // MaxUDC
  EXPECT_EQ(inverter.mbPV[309], 2600);   // MinUDC

  datalayer.aggregate.reported_total_capacity_Wh = 48000;  // 2nd pack joined
  inverter.update_values();
  EXPECT_EQ(inverter.u32(302), 48000u);
}

using FGS = FroniusGenericStorageInverter;

TEST_F(FroniusGenericStorageTest, ContactorsStayOpenUntilTheInverterAsksForTheBattery) {
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);

  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);
}

// How a GEN24 brings the battery up: INACTIVE, which closes the contactors, then ACTIVE (asked for
// as 2, answered with 3), which leaves them as they are
TEST_F(FroniusGenericStorageTest, InactiveConnectsTheBatteryAndActiveKeepsIt) {
  inverter.write_op_mode(FGS::REQUEST_INACTIVE);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);  // Precharging, not there yet
  EXPECT_EQ(inverter.mbPV[412], 0);                   // UDC_Bus

  datalayer.system.status.dc_bus_live = true;
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_INACTIVE);
  EXPECT_EQ(inverter.mbPV[412], 3800);

  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_ACTIVE);
}

TEST_F(FroniusGenericStorageTest, ActiveStraightFromStandbyConnectsTheBatteryToo) {
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);

  datalayer.system.status.dc_bus_live = true;
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_ACTIVE);
}

// The state goes to STANDBY first. The contactors follow once the inverter has read it twice, and
// no sooner than 3 s. Opened before it has seen STANDBY, the GEN24 finds its DC link gone while it
// still counts on the battery: GEN24-1250
TEST_F(FroniusGenericStorageTest, StandbyIsReportedBeforeTheContactorsOpen) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_INACTIVE);
  inverter.update_values();
  inverter.inverter_reads_monitoring();  // Reads of the state before the change do not count
  inverter.inverter_reads_monitoring();

  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[412], 3800);  // Still on the DC link, and said so

  set_millis64(1000 + 5000);  // Long enough, but not read yet
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());

  inverter.inverter_reads_monitoring();
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());  // Read once: no poll yet to act on it

  inverter.inverter_reads_monitoring();
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[412], 0);
}

TEST_F(FroniusGenericStorageTest, StandbySeenAtOnceStillWaitsTheMinimum) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();
  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  inverter.inverter_reads_monitoring();
  inverter.inverter_reads_monitoring();

  set_millis64(1000 + 2999);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());

  set_millis64(1000 + 3000);
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, StandbyWaitsForTheCurrentToGo) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();

  datalayer.aggregate.current_dA = -150;  // Still discharging at 15 A
  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  inverter.inverter_reads_monitoring();
  inverter.inverter_reads_monitoring();
  set_millis64(1000 + 3000);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());

  datalayer.aggregate.current_dA = -5;
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, StandbyOpensAfterTheWaitEvenUnderCurrent) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();

  datalayer.aggregate.current_dA = 150;
  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  inverter.inverter_reads_monitoring();
  inverter.inverter_reads_monitoring();
  set_millis64(1000 + 9999);
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());

  set_millis64(1000 + 10000);
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
}

// An inverter that never reads the state gets the contactors opened all the same, after a minute
TEST_F(FroniusGenericStorageTest, StandbyNeverReadOpensAfterAMinute) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();
  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();

  set_millis64(1000 + 59999);
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());

  set_millis64(1000 + 60000);
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());
}

// Asked for again before the hand-over is over: the contactors never open
TEST_F(FroniusGenericStorageTest, RequestDuringTheHandOverKeepsTheBatteryConnected) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();
  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  inverter.inverter_reads_monitoring();
  inverter.inverter_reads_monitoring();

  set_millis64(1000 + 2000);
  inverter.write_op_mode(FGS::REQUEST_INACTIVE);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_INACTIVE);

  set_millis64(1000 + 5000);
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, SilentInverterLosesItsRequest) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  inverter.update_values();
  ASSERT_EQ(inverter.mbPV[400], FGS::STATE_ACTIVE);

  set_millis64(1000 + 60001);  // Not a single request for a minute
  inverter.update_values();
  // No longer ACTIVE (FAULT by now: the watchdog has raised "inverter missing"), and handed over
  // like any STANDBY. A silent inverter reads nothing, so that takes another minute
  EXPECT_NE(inverter.mbPV[400], FGS::STATE_ACTIVE);
  EXPECT_TRUE(inverter.allows_contactor_closing());
  set_millis64(1000 + 60001 + 60000);
  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());

  inverter.inverter_talks();  // Back again, and the last opMode still stands
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
}

// The workaround decides over the contactors only. The state still answers the request
TEST_F(FroniusGenericStorageTest, AlwaysClosedWorkaroundOnlyKeepsTheContactorsClosed) {
  user_selected_inverter_contactor_mode = inverter_contactor_mode_enum::AlwaysClosed;
  datalayer.system.status.dc_bus_live = true;

  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);  // Nothing asked yet

  inverter.write_op_mode(FGS::REQUEST_INACTIVE);
  inverter.update_values();
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_INACTIVE);

  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  set_millis64(1000 + 10000);
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_STANDBY);
  EXPECT_EQ(inverter.mbPV[412], 3800);  // On the DC link all the same, and said so
}

TEST_F(FroniusGenericStorageTest, LockWorkaroundKeepsTheContactorsClosedAfterTheFirstRequest) {
  user_selected_inverter_contactor_mode = inverter_contactor_mode_enum::LockAfterFirstClose;

  inverter.update_values();
  EXPECT_FALSE(inverter.allows_contactor_closing());

  inverter.write_op_mode(FGS::REQUEST_INACTIVE);
  inverter.update_values();
  inverter.write_op_mode(FGS::REQUEST_STANDBY);
  inverter.update_values();
  set_millis64(1000 + 10000);
  inverter.inverter_talks();
  inverter.update_values();
  EXPECT_TRUE(inverter.allows_contactor_closing());
}

TEST_F(FroniusGenericStorageTest, EmulatorFaultIsReportedWhateverTheInverterAsked) {
  datalayer.system.status.dc_bus_live = true;
  inverter.write_op_mode(FGS::REQUEST_ACTIVE);
  datalayer.system.status.system_status = FAULT;
  inverter.update_values();

  EXPECT_EQ(inverter.mbPV[400], FGS::STATE_FAULT);
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
  EXPECT_EQ(inverter.u32(1002), 0u);  // No timestamp to give it yet, so the inverter takes it as empty

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

// Raised before the inverter has sent its time: held in the buffers and back-dated once it has,
// including a fault that is over by then
TEST_F(FroniusGenericStorageTest, FaultsBeforeTheClockAreBackDatedOnceItIsKnown) {
  set_event(EVENT_BATTERY_OVERHEAT, 512, 1);
  inverter.update_values();
  clear_event(EVENT_BATTERY_OVERHEAT, 1);  // Over before the clock comes
  inverter.update_values();

  set_millis64(1000 + 20000);
  set_event(EVENT_CELL_CRITICAL_OVER_VOLTAGE, 4250, 1);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1000), 0u);  // Neither stamped yet
  EXPECT_EQ(inverter.u32(1002), 0u);
  EXPECT_EQ(inverter.u32(1012), 0u);

  set_millis64(1000 + 50000);
  inverter.write_utc(1794299900);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1002), 1794299900u - 50);  // Fault_0: the overheat, 50 s ago
  EXPECT_EQ(inverter.mbPV[1005], 27);
  EXPECT_EQ(inverter.u32(1012), 1794299900u - 30);  // Fault_1: the cell overvoltage, 30 s ago
  EXPECT_EQ(inverter.mbPV[1015], 24);

  // From here on a new fault gets the time at once
  set_millis64(1000 + 60000);
  set_event(EVENT_HVIL_FAILURE, 0, 1);
  inverter.update_values();
  EXPECT_EQ(inverter.u32(1022), 1794299900u + 10);
  EXPECT_EQ(inverter.mbPV[1025], 47);

  clear_event(EVENT_CELL_CRITICAL_OVER_VOLTAGE, 1);
  clear_event(EVENT_HVIL_FAILURE, 1);
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
