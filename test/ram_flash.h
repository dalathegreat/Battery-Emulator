#ifndef TEST_RAM_FLASH_H
#define TEST_RAM_FLASH_H

#include <cstring>
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
    // Simulate a write that reports success but lands wrong (bit rot, a
    // marginal cell): the store can only catch this by reading back.
    if (corrupt_writes_at != 0 && offset <= corrupt_writes_at && corrupt_writes_at < offset + len) {
      data_[corrupt_writes_at] ^= 0xFF;
    }
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

  int write_budget = -1;           // -1 = unlimited mutations
  uint32_t corrupt_writes_at = 0;  // != 0: silently flip this byte on write

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

#endif
