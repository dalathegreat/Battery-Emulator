#include <gtest/gtest.h>

#include "../Software/src/communication/nvm/comm_nvm.h"

#include "Arduino.h"

// A stored setting carries a TYPE TAG, and a typed read returns the caller's
// default when the tag does not match. That makes one value per type dangerous
// to save: the value the mistagged read already reports.
//
// saveX() skipped a write when the current read equalled the new value, without
// checking the tag. On a mistagged key the read returns the default, so saving
// exactly that default compared equal and the write was skipped - and the write
// is the only thing that repairs the tag, because nvs_set_* replaces an entry
// whatever type it had. The key stayed mistagged and every later boot read the
// row default instead of the user's choice.
//
// The case that makes it concrete is WIFIAPENABLED, whose default is TRUE: a
// user switching the access point off would be ignored, silently, forever.

namespace {

class SettingsTagTest : public ::testing::Test {
 protected:
  BatteryEmulatorSettingsStore store{false};
};

}  // namespace

TEST_F(SettingsTagTest, SavingFalseOntoAKeyMistaggedAsUIntRepairsTheTag) {
  // How a key gets mistagged: written once through the wrong saver.
  store.saveUInt("WIFIAPENABLED", 1);
  ASSERT_TRUE(store.settingExists("WIFIAPENABLED"));

  // The bool read cannot see it and reports its default.
  ASSERT_FALSE(store.getBool("WIFIAPENABLED", false)) << "test setup is wrong: the mistag is not in effect";

  // The user switches the AP off. Value equals what the mistagged read reports,
  // which is exactly when the old code skipped the write.
  store.saveBool("WIFIAPENABLED", false);

  // The tag must have been repaired, so the next boot reads a real false rather
  // than the row default of true.
  EXPECT_EQ(store.stored_type("WIFIAPENABLED"), PT_U8)
      << "the key is still mistagged - the repairing write was skipped, and the device would boot "
         "with the AP on despite the user disabling it";
  EXPECT_FALSE(store.getBool("WIFIAPENABLED", true))
      << "reading with the row's real default (true) still yields true - the false never landed";
}

TEST_F(SettingsTagTest, SavingZeroOntoAMistaggedIntRepairsTheTag) {
  store.saveUInt("CPUTEMPOFFSET", 5);
  store.saveInt("CPUTEMPOFFSET", 0);  // 0 is what the mistagged getInt() reports
  EXPECT_EQ(store.stored_type("CPUTEMPOFFSET"), PT_I32);
  EXPECT_EQ(store.getInt("CPUTEMPOFFSET", 99), 0);
}

TEST_F(SettingsTagTest, SavingAnEmptyStringOntoAMistaggedKeyRepairsTheTag) {
  store.saveUInt("HOSTNAME", 7);
  store.saveString("HOSTNAME", "");  // "" is what the mistagged getString() reports
  EXPECT_EQ(store.stored_type("HOSTNAME"), PT_STR);
  EXPECT_EQ(store.getString("HOSTNAME", "fallback"), String(""));
}

TEST_F(SettingsTagTest, SavingZeroOntoAMistaggedUIntRepairsTheTag) {
  store.saveInt("MQTTPORT", -1);
  store.saveUInt("MQTTPORT", 0);
  EXPECT_EQ(store.stored_type("MQTTPORT"), PT_U32);
  EXPECT_EQ(store.getUInt("MQTTPORT", 1883u), 0u);
}

// The skip must survive - but note what it is NOT worth. It does not save flash
// wear: ESP-IDF's NVS compares before writing and skips an identical set by
// itself, measured on the bench (7f40ebc1). What it does keep correct is the
// store's own bookkeeping - settingsUpdated drives whether the user is told to
// reboot to apply a setting - and it avoids a pointless NVS round trip per key
// per save cycle.
TEST_F(SettingsTagTest, AnUnchangedCorrectlyTaggedValueIsStillSkipped) {
  store.saveBool("WEBAUTH", true);
  const unsigned after_first = store.storage_writes();
  ASSERT_EQ(after_first, 1u);

  store.saveBool("WEBAUTH", true);
  EXPECT_EQ(store.storage_writes(), after_first)
      << "an unchanged, correctly-tagged value was written again - the skip is gone";

  store.saveBool("WEBAUTH", false);
  EXPECT_EQ(store.storage_writes(), after_first + 1) << "a changed value was skipped";
}

// First save of a falsy value into a MISSING key must still be written - the
// isKey() guard the original comments describe.
TEST_F(SettingsTagTest, FirstSaveOfAFalsyValueIntoAMissingKeyIsWritten) {
  store.saveBool("PERFPROFILE", false);
  EXPECT_TRUE(store.settingExists("PERFPROFILE"));
  EXPECT_EQ(store.stored_type("PERFPROFILE"), PT_U8);

  store.saveString("SSID", "");
  EXPECT_TRUE(store.settingExists("SSID"));
  EXPECT_EQ(store.stored_type("SSID"), PT_STR);
}
