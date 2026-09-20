#include <gtest/gtest.h>

#include <vector>

#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/utils/types.h"
#include "../Software/src/inverter/SOL-ARK-LV-CAN.h"

// Regression tests for the two over-current bits the Sol-Ark LV protocol
// publishes in 0x359: byte 0 bit 7 (discharge over-current protection) and
// byte 1 bit 0 (charge over-current protection).
//
// datalayer.h documents the current sign as "Positive value = Battery
// Charging / Negative value = Battery Discharging", so the discharge check
// must compare against the negative of max_discharge_current_dA and the
// charge check against the positive of max_charge_current_dA. Both compared
// against the wrong sign, which is what these tests pin down. Same bug and
// same two bits as the Pylon LV fix in #2920; SOL-ARK-LV-CAN.cpp was copied
// from PYLON-LV-CAN.cpp and kept the "See Pylon protocol for example
// integration" comment, but not the fix.

void clear_transmitted_frames();
const std::vector<CAN_frame>& get_transmitted_frames();

namespace {

constexpr uint8_t kDischargeOverCurrentBit = 0x80;  // 0x359 byte 0, bit 7
constexpr uint8_t kChargeOverCurrentBit = 0x01;     // 0x359 byte 1, bit 0

// The margin SOL-ARK-LV-CAN.cpp adds on top of each limit before it calls the
// current an over-current, in dA.
constexpr int16_t kMarginDA = 50;

class SolArkLvOverCurrent : public ::testing::Test {
 protected:
  void SetUp() override {
    datalayer = DataLayer();
    // Keep the temperature and under-voltage bits in byte 0 clear so the only
    // thing that can set a bit under test is the current comparison.
    datalayer.battery.status.temperature_min_dC = 200;
    datalayer.battery.status.temperature_max_dC = 300;
    datalayer.battery.info.min_design_voltage_dV = 400;
    datalayer.battery.info.max_design_voltage_dV = 580;
    datalayer.battery.status.voltage_dV = 520;
    clear_transmitted_frames();
  }

  // Runs one publish cycle and returns the 0x359 frame that went out.
  CAN_frame publish() {
    SolArkLvInverter inverter;
    inverter.update_values();
    inverter.transmit_can(INTERVAL_1_S);
    for (const CAN_frame& frame : get_transmitted_frames()) {
      if (frame.ID == 0x359) {
        return frame;
      }
    }
    ADD_FAILURE() << "no 0x359 frame was transmitted";
    return CAN_frame{};
  }
};

}  // namespace

// A full pack reports max_charge_current_dA == 0 while it keeps supplying the
// house. The charge over-current bit must stay clear: the pack is discharging,
// which cannot be a charge over-current no matter what the charge limit is.
TEST_F(SolArkLvOverCurrent, FullPackDischargingDoesNotClaimChargeOverCurrent) {
  datalayer.battery.status.max_charge_current_dA = 0;
  datalayer.battery.status.max_discharge_current_dA = 1000;
  datalayer.battery.status.reported_current_dA = -300;  // 30.0 A out of the pack

  EXPECT_EQ(publish().data.u8[1] & kChargeOverCurrentBit, 0)
      << "a discharging pack must not raise the charge over-current bit";
}

// An idle pack draws no current at all. Neither bit may be set.
TEST_F(SolArkLvOverCurrent, IdlePackRaisesNoOverCurrentBits) {
  datalayer.battery.status.max_charge_current_dA = 0;
  datalayer.battery.status.max_discharge_current_dA = 0;
  datalayer.battery.status.reported_current_dA = 0;

  CAN_frame frame = publish();
  EXPECT_EQ(frame.data.u8[0] & kDischargeOverCurrentBit, 0);
  EXPECT_EQ(frame.data.u8[1] & kChargeOverCurrentBit, 0);
}

TEST_F(SolArkLvOverCurrent, ChargingPastTheChargeLimitRaisesChargeOverCurrent) {
  datalayer.battery.status.max_charge_current_dA = 500;
  datalayer.battery.status.max_discharge_current_dA = 500;
  datalayer.battery.status.reported_current_dA = 500 + kMarginDA + 10;  // charging past the limit

  EXPECT_EQ(publish().data.u8[1] & kChargeOverCurrentBit, kChargeOverCurrentBit);
}

TEST_F(SolArkLvOverCurrent, DischargingPastTheDischargeLimitRaisesDischargeOverCurrent) {
  datalayer.battery.status.max_charge_current_dA = 500;
  datalayer.battery.status.max_discharge_current_dA = 500;
  datalayer.battery.status.reported_current_dA = -(500 + kMarginDA + 10);  // discharging past the limit

  EXPECT_EQ(publish().data.u8[0] & kDischargeOverCurrentBit, kDischargeOverCurrentBit);
}

// Charging hard is not a discharge over-current, and discharging hard is not a
// charge over-current. These are the two cross-checks the wrong signs failed.
TEST_F(SolArkLvOverCurrent, HeavyChargingDoesNotClaimDischargeOverCurrent) {
  datalayer.battery.status.max_charge_current_dA = 2000;
  datalayer.battery.status.max_discharge_current_dA = 500;
  datalayer.battery.status.reported_current_dA = 1500;  // well inside the charge limit

  EXPECT_EQ(publish().data.u8[0] & kDischargeOverCurrentBit, 0);
}

TEST_F(SolArkLvOverCurrent, HeavyDischargingDoesNotClaimChargeOverCurrent) {
  datalayer.battery.status.max_charge_current_dA = 500;
  datalayer.battery.status.max_discharge_current_dA = 2000;
  datalayer.battery.status.reported_current_dA = -1500;  // well inside the discharge limit

  EXPECT_EQ(publish().data.u8[1] & kChargeOverCurrentBit, 0);
}

// Right at the limit is not yet over it: the margin exists so that a pack
// sitting exactly on its own limit is not reported as protecting itself.
TEST_F(SolArkLvOverCurrent, CurrentExactlyAtEitherLimitRaisesNothing) {
  datalayer.battery.status.max_charge_current_dA = 500;
  datalayer.battery.status.max_discharge_current_dA = 500;

  datalayer.battery.status.reported_current_dA = 500;
  EXPECT_EQ(publish().data.u8[1] & kChargeOverCurrentBit, 0);

  clear_transmitted_frames();
  datalayer.battery.status.reported_current_dA = -500;
  EXPECT_EQ(publish().data.u8[0] & kDischargeOverCurrentBit, 0);
}
