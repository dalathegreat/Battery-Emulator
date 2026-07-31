#ifndef _I18N_STORE_H_
#define _I18N_STORE_H_

#include <stdint.h>
#include <cstddef>
#include <vector>
#include "i18n_util.h"

/* Raw slot store for language catalogs on the spiffs partition: <=16 named
 * blobs, double-buffered directory (blocks A/B at 0 and 4096, valid-CRC block
 * with the higher sequence wins), data extents at 4 KB granularity from 8 KB
 * up. The directory flip is the commit point: power loss at any moment leaves
 * the previous directory - and every catalog it references - intact.
 * Backed by a narrow flash interface so the store is host-testable against a
 * RAM-backed fake. */

class I18nFlash {
 public:
  virtual ~I18nFlash() = default;
  virtual bool read(uint32_t offset, void* buf, size_t len) = 0;
  virtual bool write(uint32_t offset, const void* buf, size_t len) = 0;
  virtual bool erase(uint32_t offset, size_t len) = 0;  // 4 KB aligned
  virtual uint32_t size() = 0;
};

struct I18nStoreEntry {
  char name[24];  // NUL-padded, e.g. "uk.json.gz"
  uint32_t offset;
  uint32_t length;
  uint32_t crc32;
};

class I18nStore {
 public:
  static constexpr uint32_t SECTOR = 4096;
  static constexpr uint32_t DATA_START = 8192;
  static constexpr uint16_t MAX_ENTRIES = 16;
  static constexpr uint16_t FORMAT_VERSION = 1;
  static constexpr uint32_t MAX_FILE_SIZE = 131072;  // 128 KB upload cap
  // magic + version + sequence + count, entries, block crc
  static constexpr size_t DIR_HEADER_SIZE = 12;
  static constexpr size_t DIR_BLOCK_MAX = DIR_HEADER_SIZE + MAX_ENTRIES * sizeof(I18nStoreEntry) + 4;

  explicit I18nStore(I18nFlash& flash) : flash_(flash) {}

  bool mount();   // False = unformatted/corrupt: feature stays off
  bool format();  // Erase the directory blocks + write a fresh empty directory
  bool mounted() const { return mounted_; }

  std::vector<String> file_names() const;
  bool exists(const char* name) const;
  int32_t length(const char* name) const;  // -1 if absent
  bool read(const char* name, uint32_t offset, void* buf, size_t len) const;
  bool remove(const char* name);

  /* Streaming replace: begin (reserving up to reserve_len bytes) -> write
   * chunks -> commit (verifies CRC by read-back, then flips the directory)
   * or abort. The entry records the actual streamed length. One stream at a
   * time. A replaced file's old extent stays live until the flip. */
  bool stream_begin(const char* name, uint32_t reserve_len);
  bool stream_write(const void* buf, size_t len);
  bool stream_commit();
  void stream_abort();
  bool stream_active() const { return stream_active_; }

  uint32_t free_space() const;

 private:
  I18nFlash& flash_;
  bool mounted_ = false;
  uint32_t sequence_ = 0;
  uint8_t active_block_ = 0;
  std::vector<I18nStoreEntry> entries_;

  // Active stream state
  bool stream_active_ = false;
  I18nStoreEntry stream_entry_ = {};
  uint32_t stream_reserved_ = 0;
  uint32_t stream_written_ = 0;
  uint32_t stream_crc_ = 0;

  // Writes new_entries as the next directory generation; adopts it on success
  bool commit_entries(const std::vector<I18nStoreEntry>& new_entries);
  bool find_extent(uint32_t len, uint32_t* offset) const;
  bool compact();
  int find_entry(const char* name) const;
};

uint32_t i18n_crc32(uint32_t crc, const void* buf, size_t len);

#endif
