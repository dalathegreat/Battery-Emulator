#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <regex>
#include <set>
#include <string>

#include "../Software/src/battery/BATTERIES.h"
#include "../Software/src/datalayer/datalayer.h"
#include "../Software/src/devboard/settings/settings_table.h"

#include "Arduino.h"

// The table is validated by table_valid() at compile time, so these cases cannot be the only
// thing standing between a bad row and a device. What they add is a failure that names the
// row and the rule it broke, instead of one static_assert that says the table is wrong; and
// the last two cases check things a constant expression cannot reach at all - agreement with
// the runtime defaults the loader actually falls back to.

namespace {

const SettingDesc& row(Sid sid) {
  return SETTINGS_TABLE[static_cast<size_t>(sid)];
}

std::string key_of(size_t i) {
  return std::string(SETTINGS_TABLE[i].nvs_key);
}

}  // namespace

TEST(SettingsTableTest, KeysFitTheNvsLimit) {
  for (size_t i = 0; i < SID_COUNT; ++i) {
    const size_t len = strlen(SETTINGS_TABLE[i].nvs_key);
    EXPECT_GT(len, 0u) << "row " << i << " has an empty key";
    EXPECT_LE(len, 15u) << key_of(i) << " is longer than the 15-character NVS key limit";
  }
}

TEST(SettingsTableTest, KeysAreUnique) {
  std::set<std::string> seen;
  for (size_t i = 0; i < SID_COUNT; ++i) {
    EXPECT_TRUE(seen.insert(key_of(i)).second) << key_of(i) << " appears in the table twice";
  }
}

TEST(SettingsTableTest, PlaceholdersAreUnique) {
  std::set<std::string> seen;
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if (SETTINGS_TABLE[i].placeholder == nullptr) {
      continue;
    }
    EXPECT_TRUE(seen.insert(std::string(SETTINGS_TABLE[i].placeholder)).second)
        << "placeholder %" << SETTINGS_TABLE[i].placeholder << "% is claimed by two rows, one of them " << key_of(i);
  }
}

TEST(SettingsTableTest, DefaultsAreWithinTheirRange) {
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if (SETTINGS_TABLE[i].kind == SettingKind::Str) {
      continue;
    }
    EXPECT_LE(SETTINGS_TABLE[i].min, SETTINGS_TABLE[i].max) << key_of(i) << " has an inverted range";
    EXPECT_GE(SETTINGS_TABLE[i].def.i, SETTINGS_TABLE[i].min) << key_of(i) << " defaults below its own minimum";
    EXPECT_LE(SETTINGS_TABLE[i].def.i, SETTINGS_TABLE[i].max) << key_of(i) << " defaults above its own maximum";
  }
}

// The whole table is carried as int32_t, so a U32 row may not describe a value that does not
// fit one - that is the promise the kind comment makes about the pipeline value.
TEST(SettingsTableTest, UnsignedRowsStayWithinInt32) {
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if (SETTINGS_TABLE[i].kind != SettingKind::U32) {
      continue;
    }
    EXPECT_GE(SETTINGS_TABLE[i].min, 0) << key_of(i) << " is stored unsigned but allows a negative value";
    EXPECT_LE(static_cast<int64_t>(SETTINGS_TABLE[i].max), static_cast<int64_t>(INT32_MAX)) << key_of(i);
  }
}

TEST(SettingsTableTest, BoolRowsAreZeroOrOne) {
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if (SETTINGS_TABLE[i].kind != SettingKind::BoolU8) {
      continue;
    }
    EXPECT_EQ(SETTINGS_TABLE[i].min, 0) << key_of(i);
    EXPECT_EQ(SETTINGS_TABLE[i].max, 1) << key_of(i);
  }
}

// The inventory's D1 result: the two signed keys are the only ones on the whole device, and a
// third appearing means someone stored a negative through a getUInt path.
TEST(SettingsTableTest, SignedRowsAreOnlyTheTwoKnownKeys) {
  std::set<std::string> signed_keys;
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if (SETTINGS_TABLE[i].kind == SettingKind::I32) {
      signed_keys.insert(key_of(i));
    }
  }
  EXPECT_EQ(signed_keys, (std::set<std::string>{"CPUTEMPOFFSET", "MINPERCENTAGE"}));
}

TEST(SettingsTableTest, StringRowsCarryADefault) {
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if (SETTINGS_TABLE[i].kind != SettingKind::Str) {
      continue;
    }
    EXPECT_NE(SETTINGS_TABLE[i].def.s, nullptr) << key_of(i) << " is a string row with a null default";
  }
}

// SF_SECRET means the stored value is never rendered back; the field itself is still on the
// page, rendered blank. Enforcing the blank render belongs with the render consumer - what is
// checkable here is that the flag sits on the password-carrying rows and nowhere else.
TEST(SettingsTableTest, SecretFlagMarksExactlyThePasswordRows) {
  std::set<std::string> secrets;
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if ((SETTINGS_TABLE[i].flags & SF_SECRET) != 0) {
      secrets.insert(key_of(i));
      EXPECT_EQ(SETTINGS_TABLE[i].kind, SettingKind::Str) << key_of(i) << " is secret but not stored as a string";
    }
  }
  EXPECT_EQ(secrets, (std::set<std::string>{"APPASSWORD", "HTTPPASS", "MQTTPASSWORD", "PASSWORD"}));
}

// For these rows 0 is the "never stored" sentinel, so the range has to admit it or the row
// would describe a value its own loader treats as normal.
TEST(SettingsTableTest, SkipIfZeroRowsAdmitZero) {
  for (size_t i = 0; i < SID_COUNT; ++i) {
    if ((SETTINGS_TABLE[i].flags & SF_SKIP_IF_ZERO) == 0) {
      continue;
    }
    EXPECT_EQ(SETTINGS_TABLE[i].min, 0) << key_of(i) << " skips on zero but does not allow it";
    EXPECT_EQ(SETTINGS_TABLE[i].def.i, 0) << key_of(i) << " skips on zero but defaults to something else";
  }
}

// Five of the Tesla rows and the two current limits do not default to a literal in the loader:
// it passes the current value of a global, so the table's copy is a second statement of the
// same number. These two cases are the alarm if the two ever drift apart. Nothing in the suite
// writes the Tesla globals, so the comparison does not depend on test order.
TEST(SettingsTableTest, TeslaGatewayDefaultsMatchTheDriverGlobals) {
  EXPECT_EQ(row(Sid::GTWCOUNTRY).def.i, static_cast<int32_t>(user_selected_tesla_GTW_country));
  EXPECT_EQ(row(Sid::GTWRHD).def.i, user_selected_tesla_GTW_rightHandDrive ? 1 : 0);
  EXPECT_EQ(row(Sid::GTWMAPREG).def.i, static_cast<int32_t>(user_selected_tesla_GTW_mapRegion));
  EXPECT_EQ(row(Sid::GTWCHASSIS).def.i, static_cast<int32_t>(user_selected_tesla_GTW_chassisType));
  EXPECT_EQ(row(Sid::GTWPACK).def.i, static_cast<int32_t>(user_selected_tesla_GTW_packEnergy));
}

TEST(SettingsTableTest, CurrentLimitDefaultsMatchTheDatalayerDefaults) {
  // A fresh instance rather than the live datalayer, so a test that moved the limits cannot
  // decide this one.
  const DATALAYER_BATTERY_SETTINGS_TYPE defaults{};
  EXPECT_EQ(row(Sid::MAXCHARGEAMP).def.i, static_cast<int32_t>(defaults.max_user_set_charge_dA));
  EXPECT_EQ(row(Sid::MAXDISCHARGEAMP).def.i, static_cast<int32_t>(defaults.max_user_set_discharge_dA));
}

TEST(SettingsTableTest, EveryRowIsReachableThroughItsSid) {
  EXPECT_EQ(SID_COUNT, 145u) << "a row was added or removed - update this count deliberately";
  EXPECT_EQ(setting_desc(Sid::SSID).nvs_key, std::string("SSID"));
  EXPECT_EQ(setting_desc(static_cast<Sid>(SID_COUNT - 1)).nvs_key, std::string("CTINVERT"));
  EXPECT_TRUE(table_valid());
}

// The table only describes the device for as long as it keeps up with the loader. Someone adding a
// setting edits comm_nvm.cpp; if the row is forgotten, the table silently stops covering that key
// and everything built on it - the boot audit, the one stated default, the range - stops covering
// it too, without anything failing. So the loader's own source is the reference here: every key it
// reads by name must have a row.
//
// Keys reached through a variable rather than a literal are invisible to this, by design: the
// legacy static-IP octets are read that way precisely because they are migration-only and have no
// row.
TEST(SettingsTableTest, EveryKeyTheLoaderReadsHasARow) {
  std::ifstream loader(TEST_SETTINGS_LOADER_SOURCE);
  ASSERT_TRUE(loader.is_open()) << "cannot read " << TEST_SETTINGS_LOADER_SOURCE
                                << " - this test is worthless if it silently reads nothing";
  const std::string source((std::istreambuf_iterator<char>(loader)), std::istreambuf_iterator<char>());

  std::set<std::string> rows;
  for (size_t i = 0; i < SID_COUNT; ++i) {
    rows.insert(std::string(SETTINGS_TABLE[i].nvs_key));
  }

  // settings.getUInt("KEY" ... and the comm-interface helper, which takes the key as an argument
  const std::regex key_read(R"RX((?:settings\.get(?:UInt|Int|Bool|String|IP)|readIf)\("([A-Z0-9_]+)")RX");
  std::set<std::string> read_by_loader;
  for (auto it = std::sregex_iterator(source.begin(), source.end(), key_read); it != std::sregex_iterator(); ++it) {
    read_by_loader.insert((*it)[1].str());
  }

  ASSERT_GT(read_by_loader.size(), 100u) << "only found " << read_by_loader.size()
                                         << " keys in the loader - the scan pattern has stopped matching";
  for (const auto& key : read_by_loader) {
    EXPECT_TRUE(rows.count(key) == 1) << key << " is read at boot but has no row in the settings table";
  }
}
