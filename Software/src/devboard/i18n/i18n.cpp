#include "i18n.h"
#include <esp_partition.h>
#include "../utils/logging.h"

std::string user_selected_language = "";

// The slot store's narrow flash interface, backed by the spiffs partition
class EspPartitionFlash : public I18nFlash {
 public:
  bool init() {
    partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    return partition_ != nullptr;
  }

  bool read(uint32_t offset, void* buf, size_t len) override {
    return partition_ && esp_partition_read(partition_, offset, buf, len) == ESP_OK;
  }

  bool write(uint32_t offset, const void* buf, size_t len) override {
    return partition_ && esp_partition_write(partition_, offset, buf, len) == ESP_OK;
  }

  bool erase(uint32_t offset, size_t len) override {
    return partition_ && esp_partition_erase_range(partition_, offset, len) == ESP_OK;
  }

  uint32_t size() override { return partition_ ? partition_->size : 0; }

 private:
  const esp_partition_t* partition_ = nullptr;
};

static EspPartitionFlash flash_backend;
static I18nStore store(flash_backend);

I18nStore& i18n_store() {
  return store;
}

bool init_i18n_storage() {
  if (!flash_backend.init()) {
    logging.printf("i18n: no spiffs partition found, feature off\n");
    return false;
  }
  // No format-on-fail: a user formats explicitly from the Languages block
  if (!store.mount()) {
    logging.printf("i18n: language storage not mounted (unformatted?), feature off\n");
    return false;
  }
  return true;
}

bool i18n_storage_available() {
  return store.mounted();
}

bool format_i18n_storage() {
  return store.format();
}

std::vector<String> i18n_stored_files() {
  if (!store.mounted()) {
    return std::vector<String>();
  }
  return store.file_names();
}
