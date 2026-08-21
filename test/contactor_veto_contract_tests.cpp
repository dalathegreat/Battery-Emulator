#include <gtest/gtest.h>

#include <cctype>
#include <string>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/hal/hal.h"
#include "../Software/src/devboard/utils/events.h"

/* The contactor-veto contract, per driver.
 *
 * battery_allows_contactor_closing is each battery's veto over contactor
 * closure, and the closing gate honors it again (it was lost in 1645c5b3).
 * Restoring the gate means every driver must either drive the veto from real
 * BMS state or grant it deliberately - a driver that never writes the flag
 * leaves it at its false default and can never close contactors.
 *
 * These tests run EVERY supported battery driver through construction and a
 * few benign update ticks on the host, and assert the flag lands where that
 * driver's declared behavior says. A driver that grants at setup or under
 * benign conditions must show true - a never-writer fails here, which is the
 * point. A handshake- or CAN-gated driver must show false - granting with no
 * vehicle data would be the unsafe direction for it.
 *
 * A new driver fails until a row is added below, which forces its author to
 * decide the veto question explicitly. What this cannot distinguish is a
 * correctly-withholding driver from a never-writer wrongly declared as
 * withholding - that judgment lives in the veto census and, eventually, the
 * provides_contactor_veto capability declaration.
 */

namespace {

// Drivers that must WITHHOLD permission under benign host conditions (no
// vehicle CAN, default datalayer): their grant comes from decoded vehicle
// state or a scheduled CAN path that plain update_values() ticks never run.
// Everyone else must have granted by the time setup + a few ticks are done.
bool expected_to_withhold(BatteryType type) {
  switch (type) {
    case BatteryType::BmwIX:       // grants 2 s after boot, from transmit_can()
    case BatteryType::BydAtto3:    // grants from system state in transmit_can()
    case BatteryType::Chademo:     // grants only in CHADEMO_EVSE_START, after the vehicle handshake
    case BatteryType::Meb:         // grants only in HV-active BMS modes
    case BatteryType::VAGMqbEvo:   // MEB subclass, same veto
    case BatteryType::MgGen1:      // grants from decoded BMS state frames
    case BatteryType::NissanLeaf:  // grants once battery CAN is alive
    case BatteryType::VolvoSpa:    // grants from system state in transmit_can()
    case BatteryType::VolvoSpaHybrid:
      return true;
    default:
      return false;
  }
}

class ContactorVetoContract : public ::testing::TestWithParam<BatteryType> {};

TEST_P(ContactorVetoContract, DeclaredVetoBehaviorHolds) {
  const BatteryType type = GetParam();

  datalayer.system.status.system_status = ACTIVE;
  user_selected_battery_type = type;
  setup_battery();
  ASSERT_NE(battery, nullptr) << "create_battery() returned nothing for " << name_for_battery_type(type);

  for (int i = 0; i < 3; ++i) {
    battery->update_values();
  }

  if (expected_to_withhold(type)) {
    EXPECT_FALSE(datalayer.system.status.battery_allows_contactor_closing)
        << name_for_battery_type(type)
        << " granted contactor permission with no vehicle data - its veto is supposed to wait for the vehicle";
  } else {
    EXPECT_TRUE(datalayer.system.status.battery_allows_contactor_closing)
        << name_for_battery_type(type)
        << " never granted contactor permission - under the restored gate this driver cannot close contactors";
  }
}

std::string test_name_for(const ::testing::TestParamInfo<BatteryType>& info) {
  const char* raw = name_for_battery_type(info.param);
  std::string name = raw ? raw : "";
  std::string out;
  for (char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
      out += c;
    }
  }
  return out.empty() ? std::to_string(static_cast<int>(info.param)) : out;
}

// supported_battery_types() spans the raw enum range, including None and the
// two unassigned values; only entries with a name are constructible drivers.
std::vector<BatteryType> constructible_battery_types() {
  std::vector<BatteryType> out;
  for (BatteryType type : supported_battery_types()) {
    if (type != BatteryType::None && name_for_battery_type(type) != nullptr) {
      out.push_back(type);
    }
  }
  return out;
}

INSTANTIATE_TEST_SUITE_P(AllBatteryDrivers, ContactorVetoContract, ::testing::ValuesIn(constructible_battery_types()),
                         test_name_for);

}  // namespace
