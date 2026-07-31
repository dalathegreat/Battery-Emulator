#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "../Software/src/devboard/i18n/i18n_store.h"

// RAM-backed fake of the narrow flash interface. write_budget simulates
// power loss: once the budget is exhausted every mutation fails, and a new
// store mounted on the same buffer sees exactly what made it to "flash".
class RamFlash : public I18nFlash {
 public:
  explicit RamFlash(uint32_t size) : data_(size, 0x00) {}  // 0x00 = unformatted junk

  bool read(uint32_t offset, void* buf, size_t len) override {
    if (offset + len > data_.size()) {
      return false;
    }
    memcpy(buf, data_.data() + offset, len);
    return true;
  }

  bool write(uint32_t offset, const void* buf, size_t len) override {
    if (!consume_budget() || offset + len > data_.size()) {
      return false;
    }
    memcpy(data_.data() + offset, buf, len);
    return true;
  }

  bool erase(uint32_t offset, size_t len) override {
    if (!consume_budget() || offset + len > data_.size()) {
      return false;
    }
    memset(data_.data() + offset, 0xFF, len);
    return true;
  }

  uint32_t size() override { return (uint32_t)data_.size(); }

  int write_budget = -1;  // -1 = unlimited mutations

 private:
  bool consume_budget() {
    if (write_budget < 0) {
      return true;
    }
    if (write_budget == 0) {
      return false;
    }
    write_budget--;
    return true;
  }

  std::vector<uint8_t> data_;
};

static std::string blob(char fill, size_t len) {
  return std::string(len, fill);
}

static bool upload(I18nStore& store, const char* name, const std::string& content) {
  if (!store.stream_begin(name, (uint32_t)content.size())) {
    return false;
  }
  // Feed in uneven chunks to exercise the streaming path
  size_t pos = 0;
  while (pos < content.size()) {
    size_t chunk = std::min<size_t>(1000, content.size() - pos);
    if (!store.stream_write(content.data() + pos, chunk)) {
      return false;
    }
    pos += chunk;
  }
  return store.stream_commit();
}

static std::string read_all(I18nStore& store, const char* name) {
  int32_t len = store.length(name);
  if (len < 0) {
    return "<absent>";
  }
  std::string out((size_t)len, 0);
  if (!store.read(name, 0, out.data(), out.size())) {
    return "<readfail>";
  }
  return out;
}

TEST(I18nStoreTest, UnformattedFlashDoesNotMount) {
  RamFlash flash(128 * 1024);
  I18nStore store(flash);
  EXPECT_FALSE(store.mount()) << "Junk flash must leave the feature off, not auto-format";
  EXPECT_TRUE(store.format());
  EXPECT_TRUE(store.mounted());
  EXPECT_EQ(store.file_names().size(), 0u);

  I18nStore reopened(flash);
  EXPECT_TRUE(reopened.mount()) << "A formatted store must mount";
}

TEST(I18nStoreTest, UploadListReadRoundTrip) {
  RamFlash flash(128 * 1024);
  I18nStore store(flash);
  store.format();

  std::string content = blob('S', 30000);
  ASSERT_TRUE(upload(store, "sv.json.gz", content));
  EXPECT_TRUE(store.exists("sv.json.gz"));
  EXPECT_EQ(store.length("sv.json.gz"), 30000);
  EXPECT_EQ(read_all(store, "sv.json.gz"), content);

  // Survives a remount
  I18nStore reopened(flash);
  ASSERT_TRUE(reopened.mount());
  EXPECT_EQ(read_all(reopened, "sv.json.gz"), content);

  ASSERT_TRUE(reopened.remove("sv.json.gz"));
  EXPECT_FALSE(reopened.exists("sv.json.gz"));
}

TEST(I18nStoreTest, PowerLossMidUploadKeepsOldState) {
  RamFlash flash(128 * 1024);
  I18nStore store(flash);
  store.format();
  ASSERT_TRUE(upload(store, "sv.json.gz", blob('A', 8000)));

  // Power dies while streaming the second file: no commit, no flip
  ASSERT_TRUE(store.stream_begin("fi.json.gz", 8000));
  std::string partial = blob('B', 4000);
  ASSERT_TRUE(store.stream_write(partial.data(), partial.size()));
  // (reboot)

  I18nStore reopened(flash);
  ASSERT_TRUE(reopened.mount());
  EXPECT_EQ(read_all(reopened, "sv.json.gz"), blob('A', 8000));
  EXPECT_FALSE(reopened.exists("fi.json.gz"));
}

TEST(I18nStoreTest, ReplaceThenPowerLossServesOldVersion) {
  RamFlash flash(128 * 1024);
  I18nStore store(flash);
  store.format();
  ASSERT_TRUE(upload(store, "sv.json.gz", blob('1', 6000)));

  // Replacement streams fully but power dies at the directory flip
  ASSERT_TRUE(store.stream_begin("sv.json.gz", 6000));
  std::string v2 = blob('2', 6000);
  ASSERT_TRUE(store.stream_write(v2.data(), v2.size()));
  flash.write_budget = 0;  // Everything from here fails to reach flash
  EXPECT_FALSE(store.stream_commit());
  flash.write_budget = -1;
  // (reboot)

  I18nStore reopened(flash);
  ASSERT_TRUE(reopened.mount());
  EXPECT_EQ(read_all(reopened, "sv.json.gz"), blob('1', 6000)) << "The old catalog must survive a failed replace";
}

TEST(I18nStoreTest, CorruptedActiveBlockFallsBackToOlder) {
  RamFlash flash(128 * 1024);
  I18nStore store(flash);
  store.format();                                             // Directory generation 1 in block 0
  ASSERT_TRUE(upload(store, "sv.json.gz", blob('A', 5000)));  // Generation 2 in block 1

  // Corrupt the active directory block (bit rot / interrupted write)
  uint8_t junk = 0x5A;
  flash.write(I18nStore::SECTOR + 8, &junk, 1);

  I18nStore reopened(flash);
  ASSERT_TRUE(reopened.mount()) << "The older valid block must win";
  EXPECT_FALSE(reopened.exists("sv.json.gz")) << "Fell back to the pre-upload generation";
}

TEST(I18nStoreTest, CompactionMakesRoom) {
  // Dirs 8 KB + 32 KB data area
  RamFlash flash(40 * 1024);
  I18nStore store(flash);
  store.format();

  ASSERT_TRUE(upload(store, "aa.json.gz", blob('A', 12 * 1024)));
  ASSERT_TRUE(upload(store, "bb.json.gz", blob('B', 12 * 1024)));
  ASSERT_TRUE(store.remove("aa.json.gz"));

  // Free space is 20 KB but split 12 KB + 8 KB: 16 KB only fits after compaction
  ASSERT_TRUE(upload(store, "cc.json.gz", blob('C', 16 * 1024)));
  EXPECT_EQ(read_all(store, "bb.json.gz"), blob('B', 12 * 1024)) << "Compaction must preserve moved catalogs";
  EXPECT_EQ(read_all(store, "cc.json.gz"), blob('C', 16 * 1024));
}

TEST(I18nStoreTest, FullStoreRejectsUpload) {
  RamFlash flash(40 * 1024);  // 32 KB data area
  I18nStore store(flash);
  store.format();
  ASSERT_TRUE(upload(store, "aa.json.gz", blob('A', 20 * 1024)));

  EXPECT_FALSE(store.stream_begin("bb.json.gz", 16 * 1024)) << "No amount of compaction makes 36 KB fit in 32 KB";
  EXPECT_TRUE(upload(store, "bb.json.gz", blob('B', 12 * 1024))) << "A fitting upload must still work";
}

TEST(I18nStoreTest, UploadCapAndNameLimits) {
  RamFlash flash(512 * 1024);
  I18nStore store(flash);
  store.format();
  EXPECT_FALSE(store.stream_begin("sv.json.gz", I18nStore::MAX_FILE_SIZE + 1));
  EXPECT_TRUE(store.stream_begin("sv.json.gz", I18nStore::MAX_FILE_SIZE));
  store.stream_abort();
  EXPECT_FALSE(store.stream_begin("this-name-is-way-too-long-for-an-entry.json.gz", 100));
}
