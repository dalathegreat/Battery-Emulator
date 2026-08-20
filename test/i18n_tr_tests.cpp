#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "../Software/src/devboard/i18n/i18n_store.h"
#include "../Software/src/devboard/i18n/tr.h"
#include "ram_flash.h"

// i18n.cpp (the ESP32 partition glue) is not part of the host build, so the
// store the runtime resolves through i18n_store() is provided here instead.
static RamFlash tr_flash(128 * 1024);
static I18nStore tr_store(tr_flash);
I18nStore& i18n_store() {
  return tr_store;
}
std::string user_selected_language = "";

// Build .blp pack bytes exactly per the format the generator emits:
// 'BLP1' | u16 version | u16 count | u16 max_id | u16 reserved |
// count x (u16 id, u32 offset) sorted by id | NUL-terminated UTF-8 blob
static std::string make_pack(const std::vector<std::pair<uint16_t, std::string>>& entries) {
  std::string index;
  std::string blob;
  uint16_t max_id = 0;
  for (const auto& e : entries) {
    uint16_t id = e.first;
    uint32_t offset = (uint32_t)blob.size();
    index.append((const char*)&id, 2);
    index.append((const char*)&offset, 4);
    blob += e.second;
    blob += '\0';
    if (id > max_id) {
      max_id = id;
    }
  }
  std::string pack = "BLP1";
  uint16_t version = 1;
  uint16_t count = (uint16_t)entries.size();
  uint16_t reserved = 0;
  pack.append((const char*)&version, 2);
  pack.append((const char*)&count, 2);
  pack.append((const char*)&max_id, 2);
  pack.append((const char*)&reserved, 2);
  return pack + index + blob;
}

static void store_pack(const char* name, const std::string& bytes) {
  ASSERT_TRUE(tr_store.stream_begin(name, (uint32_t)bytes.size()));
  ASSERT_TRUE(tr_store.stream_write(bytes.data(), bytes.size()));
  ASSERT_TRUE(tr_store.stream_commit());
}

class I18nTrTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tr_flash.write_budget = -1;
    ASSERT_TRUE(tr_store.format());
    i18n_activate("");  // Drop pack state from the previous test
  }
};

TEST_F(I18nTrTest, DensePackDirectLookup) {
  // Every id 0..3 present: lookup must take the dense-direct path
  store_pack("en.blp", make_pack({{0, "zero"}, {1, "one"}, {2, "two"}, {3, "three"}}));
  ASSERT_TRUE(i18n_activate("en"));
  EXPECT_EQ(TR((TrKey)0), String("zero"));
  EXPECT_EQ(TR((TrKey)3), String("three"));
}

TEST_F(I18nTrTest, SparsePackUsesBinarySearch) {
  // Gaps in the id space: count != max_id + 1 forces binary search
  store_pack("en.blp", make_pack({{2, "two"}, {5, "five"}, {9, "nine"}}));
  ASSERT_TRUE(i18n_activate("en"));
  EXPECT_EQ(TR((TrKey)2), String("two"));
  EXPECT_EQ(TR((TrKey)5), String("five"));
  EXPECT_EQ(TR((TrKey)9), String("nine"));
  EXPECT_EQ(TR((TrKey)4), String("[T4]")) << "Absent id inside the range must miss, not mis-hit";
  EXPECT_EQ(TR((TrKey)10), String("[T10]")) << "Id beyond max_id must miss";
}

TEST_F(I18nTrTest, ActiveLanguageWinsEnglishFillsGaps) {
  store_pack("en.blp", make_pack({{0, "Battery"}, {1, "Inverter"}}));
  store_pack("sv.blp", make_pack({{0, "Batteri"}}));  // Incomplete translation
  ASSERT_TRUE(i18n_activate("sv"));
  EXPECT_EQ(TR((TrKey)0), String("Batteri"));
  EXPECT_EQ(TR((TrKey)1), String("Inverter")) << "Untranslated key must fall back to English";
  EXPECT_EQ(TR((TrKey)7), String("[T7]")) << "Key in no pack ends at the visible marker";
}

TEST_F(I18nTrTest, MissingEnglishPackStillServesActiveLanguage) {
  store_pack("sv.blp", make_pack({{0, "Batteri"}}));
  ASSERT_TRUE(i18n_activate("sv")) << "Activation must not require en.blp to be present";
  EXPECT_EQ(TR((TrKey)0), String("Batteri"));
  EXPECT_EQ(TR((TrKey)1), String("[T1]"));
}

TEST_F(I18nTrTest, EmptySelectionMeansEnglish) {
  store_pack("en.blp", make_pack({{0, "Battery"}}));
  store_pack("sv.blp", make_pack({{0, "Batteri"}}));
  ASSERT_TRUE(i18n_activate(""));
  EXPECT_EQ(TR((TrKey)0), String("Battery"));
}

TEST_F(I18nTrTest, CorruptPackIsRejected) {
  std::string bad = make_pack({{0, "zero"}});
  bad[0] = 'X';  // Break the magic
  store_pack("en.blp", bad);
  EXPECT_FALSE(i18n_activate("en"));
  EXPECT_EQ(TR((TrKey)0), String("[T0]")) << "A rejected pack must not serve strings";
}

TEST_F(I18nTrTest, TruncatedIndexIsRejected) {
  std::string pack = make_pack({{0, "zero"}, {1, "one"}});
  pack.resize(14);  // Header claims 2 entries but the index is cut off
  store_pack("en.blp", pack);
  EXPECT_FALSE(i18n_activate("en"));
}

TEST_F(I18nTrTest, NoPacksAtAllYieldsMarkers) {
  EXPECT_FALSE(i18n_activate("sv"));
  EXPECT_EQ(TR((TrKey)42), String("[T42]"));
}

TEST(I18nExpandTest, SingleToken) {
  EXPECT_EQ(tr_expand(String("Cell {0} low"), String("42")), String("Cell 42 low"));
}

TEST(I18nExpandTest, TwoTokensAndRepeats) {
  EXPECT_EQ(tr_expand(String("{0} of {1} ({0})"), String("3"), String("8")), String("3 of 8 (3)"));
}

TEST(I18nExpandTest, NoTokenPassesThrough) {
  EXPECT_EQ(tr_expand(String("100 % done"), String("x")), String("100 % done"));
}

TEST(I18nExpandTest, ValueContainingBracesIsNotReExpanded) {
  EXPECT_EQ(tr_expand(String("{0} {1}"), String("{1}"), String("V")), String("{1} V"))
      << "Substitution is literal and single-pass per token";
}

// A pack is uploaded data. TR() feeds markup at ~850 call sites, so the
// escaping has to happen in the lookup, not at each of them.
TEST_F(I18nTrTest, TrEscapesHtmlAndTrRawDoesNot) {
  store_pack("en.blp", make_pack({{0, "<script>alert(1)</script>"}, {1, "U<20V & rising"}, {2, "l'accu"}}));
  ASSERT_TRUE(i18n_activate("en"));

  EXPECT_EQ(TR((TrKey)0), String("&lt;script&gt;alert(1)&lt;/script&gt;"));
  EXPECT_EQ(TR((TrKey)1), String("U&lt;20V &amp; rising")) << "Literal metacharacters in real catalogs";
  EXPECT_EQ(TR((TrKey)2), String("l&#39;accu"));

  EXPECT_EQ(TR_RAW((TrKey)0), String("<script>alert(1)</script>")) << "Non-HTML consumers get the text as written";
  EXPECT_EQ(TR_RAW((TrKey)1), String("U<20V & rising"));
}

TEST_F(I18nTrTest, EscapingLeavesUtf8Intact) {
  store_pack("sv.blp", make_pack({{0, "Överspänning på cell"}}));
  ASSERT_TRUE(i18n_activate("sv"));
  EXPECT_EQ(TR((TrKey)0), String("Överspänning på cell"));
}

// A crafted header sizes two heap allocations; unbounded it asks for
// hundreds of KB on a device that has far less, on every boot.
TEST_F(I18nTrTest, PackWithImplausibleCountIsRejected) {
  std::string pack = make_pack({{0, "zero"}});
  uint16_t huge = 60000;
  memcpy(&pack[6], &huge, 2);  // count
  memcpy(&pack[8], &huge, 2);  // max_id
  store_pack("en.blp", pack);
  EXPECT_FALSE(i18n_activate("en")) << "count/max_id beyond the generated key space must not load";
}

TEST_F(I18nTrTest, PackWithIdsBeyondKeySpaceIsRejected) {
  store_pack("en.blp", make_pack({{(uint16_t)(TR_KEY_COUNT + 10), "past the end"}}));
  EXPECT_FALSE(i18n_activate("en"));
}

// Offsets are attacker-controlled; a lookup must not turn into a huge
// allocation or read outside the file.
TEST_F(I18nTrTest, PackWithOutOfRangeOffsetYieldsFallback) {
  std::string pack = make_pack({{0, "zero"}, {1, "one"}});
  uint32_t bogus = 0x00FFFFFFu;
  memcpy(&pack[12 + 2], &bogus, 4);  // first index entry's offset
  store_pack("en.blp", pack);
  ASSERT_TRUE(i18n_activate("en"));
  EXPECT_EQ(TR((TrKey)0), String("[T0]")) << "Unreadable slot falls through to the marker";
}

// find_index() binary-searches the pack index. An unsorted pack would not
// crash - it would return the wrong string for a key, silently.
TEST_F(I18nTrTest, PackWithUnsortedIndexIsRejected) {
  std::string pack = make_pack({{0, "zero"}, {5, "five"}, {9, "nine"}});
  // Swap the ids of the first two index entries, leaving the blob untouched
  uint16_t five = 5, zero = 0;
  memcpy(&pack[12], &five, 2);
  memcpy(&pack[12 + 6], &zero, 2);
  store_pack("en.blp", pack);
  EXPECT_FALSE(i18n_activate("en")) << "Descending ids break binary search";
}

TEST_F(I18nTrTest, PackWithDuplicateIdsIsRejected) {
  std::string pack = make_pack({{0, "zero"}, {5, "five"}, {9, "nine"}});
  uint16_t zero = 0;
  memcpy(&pack[12 + 6], &zero, 2);  // Second entry repeats id 0
  store_pack("en.blp", pack);
  EXPECT_FALSE(i18n_activate("en"));
}

/* Retiring a key tombstones its id instead of reusing it, so a real pack has
 * holes: the generator ships count == TR_KEY_COUNT with max_id == TR_MAX_ID,
 * and those differ. Bounding the header by the key count instead of the max
 * id rejected every catalog we produce - the feature was dead on hardware
 * while these tests stayed green, because synthetic packs here had dense id
 * ranges where count and max_id + 1 coincide. */
TEST_F(I18nTrTest, SparsePackReachingMaxIdLoads) {
  store_pack("en.blp", make_pack({{0, "zero"}, {1, "one"}, {TR_MAX_ID, "last"}}));
  ASSERT_TRUE(i18n_activate("en")) << "Ids up to TR_MAX_ID are legal however few keys the pack carries";
  EXPECT_EQ(TR((TrKey)TR_MAX_ID), String("last"));
  EXPECT_EQ(TR((TrKey)0), String("zero"));
}

// The exact shape the generator emits: every id present except the
// tombstoned ones, so count < max_id + 1.
TEST_F(I18nTrTest, GeneratorShapedPackWithTombstoneHolesLoads) {
  ASSERT_GT(TR_MAX_ID + 1, TR_KEY_COUNT) << "Fixture assumes at least one tombstone exists";
  std::vector<std::pair<uint16_t, std::string>> entries;
  const uint16_t holes = (uint16_t)(TR_MAX_ID + 1 - TR_KEY_COUNT);
  for (uint16_t id = 0; id <= TR_MAX_ID; id++) {
    // Holes sit in the middle, as retired ids do: the top id must still be
    // TR_MAX_ID, which is the whole point of the shape
    if (id >= 10 && id < 10 + holes) {
      continue;
    }
    entries.push_back({id, "s" + std::to_string(id)});
  }
  ASSERT_EQ((uint16_t)entries.size(), TR_KEY_COUNT);
  store_pack("en.blp", make_pack(entries));
  ASSERT_TRUE(i18n_activate("en")) << "A pack with tombstone holes is what we actually ship";
  EXPECT_EQ(TR((TrKey)0), String("s0"));
  EXPECT_EQ(TR((TrKey)100), String("s100"));
  EXPECT_EQ(TR((TrKey)TR_MAX_ID), String("s" + std::to_string(TR_MAX_ID)).c_str());
}

/* Publish semantics for a rejected pack: fall back cleanly, never serve
 * garbage. NOTE this does NOT cover the reason load_pack was restructured to
 * stage-and-publish - that is a cross-task race (web task reloading while
 * MQTT and the battery tasks read), and a single-threaded host test cannot
 * reach it. This case passes against the old in-place rebuild too; it is here
 * to pin the fallback behaviour the restructure had to preserve. The race
 * itself is covered by inspection and by the hardware language-switch test. */
TEST_F(I18nTrTest, RejectedPackDoesNotCorruptTheActiveOne) {
  store_pack("en.blp", make_pack({{0, "english zero"}, {1, "english one"}}));
  store_pack("sv.blp", make_pack({{0, "svenska noll"}, {1, "svenska ett"}}));
  ASSERT_TRUE(i18n_activate("sv"));
  ASSERT_EQ(TR((TrKey)0), String("svenska noll"));

  // Replace sv with a hostile pack and re-activate
  std::string bad = make_pack({{0, "x"}, {1, "y"}});
  uint16_t huge = 60000;
  memcpy(&bad[6], &huge, 2);
  store_pack("sv.blp", bad);
  EXPECT_FALSE(i18n_activate("sv"));

  // Never garbage: the rejected language falls back to English cleanly
  EXPECT_EQ(TR((TrKey)0), String("english zero"));
  EXPECT_EQ(TR((TrKey)1), String("english one"));
}

/* find_index() uses the id AS the slot subscript on the dense path. A pack
 * whose declared max_id does not match its actual ids would take that path
 * and serve the wrong string - silently, which for these keys means the
 * wrong fault message rather than a visible marker. */
TEST_F(I18nTrTest, PackWithIdAboveDeclaredMaxIdIsRejected) {
  std::string pack = make_pack({{0, "zero"}, {1, "one"}, {2, "two"}});
  uint16_t claimed_max = 2;
  uint16_t out_of_range = 99;
  memcpy(&pack[8], &claimed_max, 2);            // header says max_id = 2 (dense: count 3 == 2+1)
  memcpy(&pack[12 + 2 * 6], &out_of_range, 2);  // but the last entry's id is 99
  store_pack("en.blp", pack);
  EXPECT_FALSE(i18n_activate("en")) << "Dense-direct would return slot 2's text for key 2";
}

/* TR_JS() exists so call sites cannot compose the escaper with the wrong
 * lookup - js_string_escape(TR(...)) double-escapes, and omitting the wrapper
 * puts HTML-escaped text into a <script>, which is how a translation once
 * broke out of a template literal. */
TEST_F(I18nTrTest, TrJsEscapesForJavaScriptNotHtml) {
  store_pack("en.blp", make_pack({{0, "l'accu \"x\""}, {1, "a`b ${c}"}, {2, "U<20V & rising"}}));
  ASSERT_TRUE(i18n_activate("en"));

  EXPECT_EQ(TR_JS((TrKey)0), String("l\\'accu \\\"x\\\""));
  EXPECT_EQ(TR_JS((TrKey)1), String("a\\`b \\${c}")) << "Template-literal metacharacters";

  // The same key through each variant: three sinks, three answers
  EXPECT_EQ(TR_RAW((TrKey)2), String("U<20V & rising"));
  EXPECT_EQ(TR((TrKey)2), String("U&lt;20V &amp; rising"));
  EXPECT_EQ(TR_JS((TrKey)2), String("U<20V & rising"))
      << "No HTML entities in the JS form - they would render literally in a script";
}

// The argument is substituted before escaping, so a hostile value in the
// argument cannot break out either.
TEST_F(I18nTrTest, TrJsWithArgumentEscapesTheExpandedResult) {
  store_pack("en.blp", make_pack({{0, "Configured {0} cells"}}));
  ASSERT_TRUE(i18n_activate("en"));

  EXPECT_EQ(TR_JS((TrKey)0, String("48")), String("Configured 48 cells"));
  EXPECT_EQ(TR_JS((TrKey)0, String("');alert(1);//")), String("Configured \\');alert(1);// cells"))
      << "Escaping must cover the substituted value, not just the template";
}

// The two-argument form exists for the "value between X and Y" family, which
// collapses ~25 near-identical alerts into one key. Both substituted values
// must be escaped, not just the template.
TEST_F(I18nTrTest, TrJsWithTwoArgumentsEscapesBothSubstitutions) {
  store_pack("en.blp", make_pack({{0, "Enter a value between {0} and {1}."}}));
  ASSERT_TRUE(i18n_activate("en"));

  EXPECT_EQ(TR_JS((TrKey)0, String("0"), String("100")), String("Enter a value between 0 and 100."));
  EXPECT_EQ(TR_JS((TrKey)0, String("'; alert(1); '"), String("`${x}`")),
            String("Enter a value between \\'; alert(1); \\' and \\`\\${x}\\`."))
      << "A hostile argument must not break out of the literal it lands in";
}
