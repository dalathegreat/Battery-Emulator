#include "i18n_store.h"
#include <cstring>

// CRC-32 (IEEE 802.3), bitwise - no table, this is a cold path
uint32_t i18n_crc32(uint32_t crc, const void* buf, size_t len) {
  const uint8_t* p = static_cast<const uint8_t*>(buf);
  crc = ~crc;
  while (len--) {
    crc ^= *p++;
    for (int k = 0; k < 8; k++) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
    }
  }
  return ~crc;
}

static uint32_t round_up_sector(uint32_t len) {
  return (len + I18nStore::SECTOR - 1) & ~(I18nStore::SECTOR - 1);
}

bool I18nStore::mount() {
  mounted_ = false;
  entries_.clear();
  stream_active_ = false;
  bool found = false;
  for (uint8_t block = 0; block < 2; block++) {
    uint8_t buf[DIR_BLOCK_MAX];
    if (!flash_.read(block * SECTOR, buf, sizeof(buf))) {
      continue;
    }
    if (memcmp(buf, "I18S", 4) != 0) {
      continue;
    }
    uint16_t version = 0;
    memcpy(&version, buf + 4, 2);
    if (version != FORMAT_VERSION) {
      continue;
    }
    uint32_t sequence = 0;
    memcpy(&sequence, buf + 6, 4);
    uint16_t count = 0;
    memcpy(&count, buf + 10, 2);
    if (count > MAX_ENTRIES) {
      continue;
    }
    size_t payload = DIR_HEADER_SIZE + count * sizeof(I18nStoreEntry);
    uint32_t stored_crc = 0;
    memcpy(&stored_crc, buf + payload, 4);
    if (i18n_crc32(0, buf, payload) != stored_crc) {
      continue;
    }
    if (found && sequence <= sequence_) {
      continue;
    }
    // A valid CRC only proves the block is the one we wrote; it does not make
    // the extents it describes sane. Everything downstream (read, find_extent,
    // compact) assumes entries stay inside the partition, so enforce it here
    // rather than trusting flash content.
    std::vector<I18nStoreEntry> parsed;
    bool entries_valid = true;
    for (uint16_t i = 0; i < count; i++) {
      I18nStoreEntry entry;
      memcpy(&entry, buf + DIR_HEADER_SIZE + i * sizeof(I18nStoreEntry), sizeof(I18nStoreEntry));
      entry.name[sizeof(entry.name) - 1] = '\0';  // Never let a name run off the field
      if (entry.name[0] == '\0' || entry.length == 0 || entry.length > MAX_FILE_SIZE || entry.offset < DATA_START ||
          entry.offset > flash_.size() || entry.length > flash_.size() - entry.offset) {
        entries_valid = false;
        break;
      }
      parsed.push_back(entry);
    }
    if (!entries_valid) {
      continue;  // Fall back to the other block
    }
    found = true;
    sequence_ = sequence;
    active_block_ = block;
    memcpy(hint_, buf + 12, HINT_SIZE);
    hint_[HINT_SIZE - 1] = '\0';
    entries_ = parsed;
  }
  mounted_ = found;
  return mounted_;
}

bool I18nStore::format() {
  stream_active_ = false;
  if (!flash_.erase(0, 2 * SECTOR)) {
    return false;
  }
  entries_.clear();
  memset(hint_, 0, sizeof(hint_));
  sequence_ = 0;
  active_block_ = 1;  // So the fresh directory lands in block 0
  mounted_ = true;
  if (!commit_entries(entries_)) {
    mounted_ = false;
    return false;
  }
  return true;
}

bool I18nStore::commit_entries(const std::vector<I18nStoreEntry>& new_entries) {
  if (!mounted_ || new_entries.size() > MAX_ENTRIES) {
    return false;
  }
  uint8_t buf[DIR_BLOCK_MAX];
  memset(buf, 0xFF, sizeof(buf));
  memcpy(buf, "I18S", 4);
  uint16_t version = FORMAT_VERSION;
  memcpy(buf + 4, &version, 2);
  uint32_t sequence = sequence_ + 1;
  memcpy(buf + 6, &sequence, 4);
  uint16_t count = (uint16_t)new_entries.size();
  memcpy(buf + 10, &count, 2);
  memcpy(buf + 12, hint_, HINT_SIZE);
  for (uint16_t i = 0; i < count; i++) {
    memcpy(buf + DIR_HEADER_SIZE + i * sizeof(I18nStoreEntry), &new_entries[i], sizeof(I18nStoreEntry));
  }
  size_t payload = DIR_HEADER_SIZE + count * sizeof(I18nStoreEntry);
  uint32_t crc = i18n_crc32(0, buf, payload);
  memcpy(buf + payload, &crc, 4);

  uint8_t target_block = active_block_ ^ 1;  // Always the OLDER block
  if (!flash_.erase(target_block * SECTOR, SECTOR)) {
    return false;
  }
  if (!flash_.write(target_block * SECTOR, buf, payload + 4)) {
    return false;
  }
  // The flip: only adopt the new generation once it is fully on flash
  sequence_ = sequence;
  active_block_ = target_block;
  entries_ = new_entries;
  return true;
}

int I18nStore::find_entry(const char* name) const {
  for (unsigned int i = 0; i < entries_.size(); i++) {
    if (strncmp(entries_[i].name, name, sizeof(entries_[i].name)) == 0) {
      return (int)i;
    }
  }
  return -1;
}

std::vector<String> I18nStore::file_names() const {
  std::vector<String> names;
  for (const I18nStoreEntry& entry : entries_) {
    names.push_back(String(entry.name));
  }
  return names;
}

bool I18nStore::exists(const char* name) const {
  return mounted_ && find_entry(name) >= 0;
}

int32_t I18nStore::length(const char* name) const {
  int index = find_entry(name);
  if (!mounted_ || index < 0) {
    return -1;
  }
  return (int32_t)entries_[index].length;
}

uint32_t I18nStore::file_crc32(const char* name) const {
  int index = find_entry(name);
  if (!mounted_ || index < 0) {
    return 0;
  }
  return entries_[index].crc32;
}

bool I18nStore::read(const char* name, uint32_t offset, void* buf, size_t len) const {
  int index = find_entry(name);
  if (!mounted_ || index < 0) {
    return false;
  }
  const I18nStoreEntry& entry = entries_[index];
  if (offset > entry.length || len > entry.length - offset) {
    return false;
  }
  return flash_.read(entry.offset + offset, buf, len);
}

bool I18nStore::remove(const char* name) {
  int index = find_entry(name);
  if (!mounted_ || index < 0) {
    return false;
  }
  std::vector<I18nStoreEntry> new_entries = entries_;
  new_entries.erase(new_entries.begin() + index);
  return commit_entries(new_entries);
}

bool I18nStore::find_extent(uint32_t len, uint32_t* offset) const {
  uint32_t need = round_up_sector(len);
  // Collect used extents (sector-rounded), sorted by offset
  std::vector<I18nStoreEntry> used = entries_;
  for (unsigned int i = 0; i + 1 < used.size(); i++) {
    for (unsigned int j = i + 1; j < used.size(); j++) {
      if (used[j].offset < used[i].offset) {
        I18nStoreEntry tmp = used[i];
        used[i] = used[j];
        used[j] = tmp;
      }
    }
  }
  uint32_t cursor = DATA_START;
  for (const I18nStoreEntry& entry : used) {
    if (entry.offset >= cursor + need) {
      break;  // Gap before this extent fits
    }
    uint32_t end = entry.offset + round_up_sector(entry.length);
    if (end > cursor) {
      cursor = end;
    }
  }
  if (cursor + need > flash_.size()) {
    return false;
  }
  *offset = cursor;
  return true;
}

/* Space not claimed by a committed entry. An active stream's reservation is
 * erased but not yet in entries_, so this reads high by up to stream_reserved_
 * mid-upload - informational only, since stream_active_ blocks a second
 * allocation. */
uint32_t I18nStore::free_space() const {
  uint32_t used = 0;
  for (const I18nStoreEntry& entry : entries_) {
    used += round_up_sector(entry.length);
  }
  uint32_t data_size = flash_.size() - DATA_START;
  return (used > data_size) ? 0 : (data_size - used);
}

/* Best-effort defragmentation: repeatedly move an extent to the lowest free
 * slot it fits in, one directory flip per move so power loss at any point
 * keeps every catalog reachable. Converges for the <=16-entry scale this
 * store serves; a pathological layout just fails the allocation. */
bool I18nStore::compact() {
  for (unsigned int pass = 0; pass < 2 * MAX_ENTRIES; pass++) {
    bool moved = false;
    for (unsigned int i = 0; i < entries_.size(); i++) {
      const I18nStoreEntry entry = entries_[i];
      uint32_t rounded = round_up_sector(entry.length);
      uint32_t target = 0;
      if (!find_extent(entry.length, &target)) {
        continue;
      }
      if (target >= entry.offset) {
        continue;  // Only move toward the start
      }
      if (target + rounded > entry.offset) {
        continue;  // Overlaps its own source - not safely movable in place
      }
      if (!flash_.erase(target, rounded)) {
        return false;
      }
      uint8_t buf[256];
      for (uint32_t pos = 0; pos < entry.length; pos += sizeof(buf)) {
        size_t chunk = entry.length - pos;
        if (chunk > sizeof(buf)) {
          chunk = sizeof(buf);
        }
        if (!flash_.read(entry.offset + pos, buf, chunk) || !flash_.write(target + pos, buf, chunk)) {
          return false;
        }
      }
      /* Verify the copy the same way stream_commit() verifies an upload:
       * the directory flip is what makes the new extent authoritative, and
       * committing it unchecked would promote a bad copy over a good
       * original that is about to be treated as free space. */
      uint32_t moved_crc = 0;
      for (uint32_t pos = 0; pos < entry.length; pos += sizeof(buf)) {
        size_t chunk = entry.length - pos;
        if (chunk > sizeof(buf)) {
          chunk = sizeof(buf);
        }
        if (!flash_.read(target + pos, buf, chunk)) {
          return false;
        }
        moved_crc = i18n_crc32(moved_crc, buf, chunk);
      }
      if (moved_crc != entry.crc32) {
        return false;  // Leave the directory pointing at the intact original
      }
      std::vector<I18nStoreEntry> new_entries = entries_;
      new_entries[i].offset = target;
      if (!commit_entries(new_entries)) {
        return false;
      }
      moved = true;
      break;  // Free map changed: recompute from scratch
    }
    if (!moved) {
      return true;
    }
  }
  return true;
}

bool I18nStore::stream_begin(const char* name, uint32_t reserve_len) {
  if (!mounted_ || stream_active_ || reserve_len == 0 || reserve_len > MAX_FILE_SIZE) {
    return false;
  }
  size_t name_len = strlen(name);
  if (name_len == 0 || name_len >= sizeof(stream_entry_.name)) {
    return false;
  }
  if (find_entry(name) < 0 && entries_.size() >= MAX_ENTRIES) {
    return false;
  }
  uint32_t offset = 0;
  if (!find_extent(reserve_len, &offset)) {
    if (!compact() || !find_extent(reserve_len, &offset)) {
      return false;
    }
  }
  if (!flash_.erase(offset, round_up_sector(reserve_len))) {
    return false;
  }
  memset(&stream_entry_, 0, sizeof(stream_entry_));
  strncpy(stream_entry_.name, name, sizeof(stream_entry_.name) - 1);
  stream_entry_.offset = offset;
  stream_reserved_ = reserve_len;
  stream_written_ = 0;
  stream_crc_ = 0;
  stream_active_ = true;
  return true;
}

bool I18nStore::stream_write(const void* buf, size_t len) {
  // Widened: stream_written_ + len is 32-bit on target, and a len near
  // UINT32_MAX would wrap past the guard
  if (!stream_active_ || (uint64_t)stream_written_ + len > stream_reserved_) {
    stream_abort();
    return false;
  }
  if (!flash_.write(stream_entry_.offset + stream_written_, buf, len)) {
    stream_abort();
    return false;
  }
  stream_crc_ = i18n_crc32(stream_crc_, buf, len);
  stream_written_ += len;
  return true;
}

bool I18nStore::stream_commit() {
  if (!stream_active_ || stream_written_ == 0) {
    stream_abort();
    return false;
  }
  stream_active_ = false;
  // Verify the data actually on flash before the flip makes it reachable
  uint32_t verify_crc = 0;
  uint8_t buf[256];
  for (uint32_t pos = 0; pos < stream_written_; pos += sizeof(buf)) {
    size_t chunk = stream_written_ - pos;
    if (chunk > sizeof(buf)) {
      chunk = sizeof(buf);
    }
    if (!flash_.read(stream_entry_.offset + pos, buf, chunk)) {
      return false;
    }
    verify_crc = i18n_crc32(verify_crc, buf, chunk);
  }
  if (verify_crc != stream_crc_) {
    return false;
  }
  stream_entry_.length = stream_written_;
  stream_entry_.crc32 = stream_crc_;
  std::vector<I18nStoreEntry> new_entries = entries_;
  int existing = find_entry(stream_entry_.name);
  if (existing >= 0) {
    new_entries[existing] = stream_entry_;
  } else {
    new_entries.push_back(stream_entry_);
  }
  return commit_entries(new_entries);
}

void I18nStore::stream_abort() {
  stream_active_ = false;
  stream_written_ = 0;
}
