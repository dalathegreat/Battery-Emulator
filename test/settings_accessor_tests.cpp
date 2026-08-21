#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

#include "../Software/src/communication/nvm/comm_nvm.h"
#include "../Software/src/devboard/settings/settings_accessors.h"
#include "../Software/src/devboard/utils/events.h"

// The accessor layer's one claim is that a call site can no longer disagree
// with the table: the row picks the C++ type, the store call and the default.
// The compile-time half of that claim is checked right here by static_assert;
// the runtime half - that what setting_save writes carries the on-flash type
// tag the table declares, for every row - is walked over the whole table
// below, on the same Preferences emulation the store tests run on.

namespace {

class SettingsAccessorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    emul_nvs_reset();
    reset_all_events();
  }
};

const SettingDesc& row(Sid sid) {
  return SETTINGS_TABLE[static_cast<size_t>(sid)];
}

// The kind -> NVS tag mapping, stated independently of both the accessors and
// comm_nvm.cpp's preference_type_for, so this file agreeing with the firmware
// means two separate statements of the mapping agree.
PreferenceType expected_tag(SettingKind kind) {
  switch (kind) {
    case SettingKind::BoolU8:
      return PT_U8;
    case SettingKind::U32:
      return PT_U32;
    case SettingKind::I32:
      return PT_I32;
    case SettingKind::Str:
      return PT_STR;
  }
  return PT_INVALID;
}

// Save every row's current value (its default, on a clean device) through its
// own accessor - one fold over the whole Sid range, so a row added tomorrow is
// covered without anyone editing this file.
template <Sid S>
void save_default(BatteryEmulatorSettingsStore& store) {
  setting_save<S>(store, setting_get<S>(store));
}

template <size_t... I>
void save_all_defaults(BatteryEmulatorSettingsStore& store, std::index_sequence<I...>) {
  (save_default<static_cast<Sid>(I)>(store), ...);
}

}  // namespace

// The row's kind decides the accessor's C++ type at compile time.
static_assert(std::is_same_v<setting_value_t<Sid::WEBAUTH>, bool>);
static_assert(std::is_same_v<setting_value_t<Sid::MQTTPORT>, uint32_t>);
static_assert(std::is_same_v<setting_value_t<Sid::MINPERCENTAGE>, int32_t>);
static_assert(std::is_same_v<setting_value_t<Sid::HOSTNAME>, String>);

// --- Defaults --------------------------------------------------------------

// On a device that has never stored the key, the accessor returns the row's
// own default - the same fallback the loader passes today.
TEST_F(SettingsAccessorTest, ReadsTheRowDefaultWhenNothingIsStored) {
  BatteryEmulatorSettingsStore store;

  EXPECT_EQ(setting_get<Sid::MQTTPORT>(store), 1883u);
  EXPECT_EQ(setting_get<Sid::HTTPUSER>(store), String("admin"));
  EXPECT_TRUE(setting_get<Sid::WIFIAPENABLED>(store));
  EXPECT_TRUE(setting_get<Sid::GTWRHD>(store));
  EXPECT_EQ(setting_get<Sid::MINPERCENTAGE>(store), 0);
}

// --- Round-trip ------------------------------------------------------------

TEST_F(SettingsAccessorTest, RoundTripsEachKind) {
  BatteryEmulatorSettingsStore store;

  setting_save<Sid::WEBAUTH>(store, true);
  setting_save<Sid::MQTTPORT>(store, 8883u);
  setting_save<Sid::MINPERCENTAGE>(store, -50);
  setting_save<Sid::HOSTNAME>(store, String("emulator"));

  EXPECT_TRUE(setting_get<Sid::WEBAUTH>(store));
  EXPECT_EQ(setting_get<Sid::MQTTPORT>(store), 8883u);
  EXPECT_EQ(setting_get<Sid::MINPERCENTAGE>(store), -50);
  EXPECT_EQ(setting_get<Sid::HOSTNAME>(store), String("emulator"));
}

// --- Type agreement with the table, over every row --------------------------

// This is the executable form of "cannot disagree with the loader": after a
// save through the accessor, the NVS tag on every one of the 145 keys is the
// tag the row's kind declares. A row whose accessor picked the wrong store
// call - the mistake that silently resets a fleet's settings - fails here by
// name. It is also exactly the condition the boot audit checks on real
// devices, so a device the accessors have written can never trip it.
TEST_F(SettingsAccessorTest, EveryRowSavesUnderTheTagItsKindDeclares) {
  BatteryEmulatorSettingsStore store;
  save_all_defaults(store, std::make_index_sequence<SID_COUNT>{});

  for (size_t i = 0; i < SID_COUNT; ++i) {
    const SettingDesc& r = SETTINGS_TABLE[i];
    ASSERT_TRUE(store.settingExists(r.nvs_key)) << r.nvs_key << " was not written by its accessor";
    EXPECT_EQ(store.getType(r.nvs_key), expected_tag(r.kind))
        << r.nvs_key << " is stored under a different NVS tag than its row declares";
  }
}

// --- The failure the tags exist for ----------------------------------------

// A key stored under the wrong type reads back as the default, not as
// reinterpreted bits - real NVS enforces its tags. The accessor inherits that
// honesty instead of hiding it: this is the data-loss mode the shadow audit
// hunts, made visible.
TEST_F(SettingsAccessorTest, WrongTaggedEntryReadsAsTheDefault) {
  BatteryEmulatorSettingsStore store;

  // WEBAUTH's row is BoolU8; something writes it as a u32.
  store.saveUInt(row(Sid::WEBAUTH).nvs_key, 1);

  EXPECT_FALSE(setting_get<Sid::WEBAUTH>(store)) << "a mistagged entry must default, not reinterpret";
  EXPECT_NE(store.getType(row(Sid::WEBAUTH).nvs_key), expected_tag(row(Sid::WEBAUTH).kind))
      << "the audit's mismatch condition should be visible on this key";
}

// --- Save goes through the store's skip-identical logic ---------------------

// The accessor delegates to the same saveX calls the rest of the firmware
// uses, so a save of the already-stored value must not mark settings updated.
TEST_F(SettingsAccessorTest, ResavingTheStoredValueIsANoOp) {
  {
    BatteryEmulatorSettingsStore store;
    setting_save<Sid::MQTTPORT>(store, 1883u);
    EXPECT_TRUE(store.were_settings_updated());
  }

  BatteryEmulatorSettingsStore reopened;
  setting_save<Sid::MQTTPORT>(reopened, 1883u);
  EXPECT_FALSE(reopened.were_settings_updated());
}
