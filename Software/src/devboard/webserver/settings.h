#pragma once

#include <stdint.h>
#include <string.h>
#include <string>
#include <type_traits>

#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"

class BatteryEmulatorSettingsStore;  // Defined in comm_nvm.h

void apply_setting_updates(const JsonDocument& doc, JsonDocument& errors, BatteryEmulatorSettingsStore& settings,
                           bool save, bool& reboot_required_saved);

void build_settings_json(JsonDocument& doc, BatteryEmulatorSettingsStore& settings);

// Boot-time load of every table-driven setting (NVM -> live variable).
// Called from init_stored_settings() during boot only.
void load_stored_settings(BatteryEmulatorSettingsStore& settings);

// Persists the runtime-mutable ("Instant") settings: live variable -> NVM.
// Called from store_settings().
void store_settings_from_live(BatteryEmulatorSettingsStore& settings);

// ---------------------------------------------------------------------------
// Setting records. kind/type/mechanism are implicit in *which table* an entry
// lives in, so entries only carry the fields they need:
//
//  PERSISTED_*   - reboot-required, persisted to NVM, value takes effect on
//                  the next boot. Backed by a plain storage variable.
//  INSTANT_*     - persisted AND applied to live state immediately.
//  VOLATILE_*    - not persisted; apply()/read() accesses live state.
//
// The storage pointer always points at the *edited* representation (what the
// UI shows). Boot-only derived state (e.g. can_config remaps) is applied in
// init_stored_settings() from these loaded values.
// ---------------------------------------------------------------------------

// Numeric setting of any width (uint32/uint16/uint8/int/enum). NVM stores a
// uint32; `width` bytes are transferred to/from `storage` on load.
struct PersistedUint {
  const char* name;
  uint32_t min, max;
  uint8_t width;  // sizeof(*storage): 1, 2 or 4
  void* storage;
};
// Storage width is derived from the pointer type, so it cannot be wrong.
constexpr PersistedUint UintSetting(const char* name, uint32_t min, uint32_t max, auto* storage) {
  using StorageType = std::remove_cv_t<std::remove_pointer_t<decltype(storage)>>;
  static_assert(std::is_unsigned_v<StorageType> || std::is_enum_v<StorageType>,
                "UintSetting storage must be unsigned or an enum");
  static_assert(sizeof(*storage) == 1 || sizeof(*storage) == 2 || sizeof(*storage) == 4,
                "UintSetting storage must be uint8_t, uint16_t or uint32_t");
  return {name, min, max, static_cast<uint8_t>(sizeof(*storage)), storage};
}

struct PersistedInt {
  const char* name;
  int32_t min, max;
  int32_t* storage;
};
constexpr PersistedInt IntSetting(const char* name, int32_t min, int32_t max, int32_t* storage) {
  return {name, min, max, storage};
}

struct PersistedBool {
  const char* name;
  bool* storage;
};
constexpr PersistedBool BoolSetting(const char* name, bool* storage) {
  return {name, storage};
}

struct PersistedString {
  const char* name;
  uint8_t max_length;
  uint8_t flags;  // SETTING_SECRET
  std::string* storage;
};
constexpr PersistedString StringSetting(const char* name, uint8_t max_length, std::string* storage, uint8_t flags = 0) {
  return {name, max_length, flags, storage};
}

// Edited/validated as a float, stored as uint16_t = edit_value * scale.
struct PersistedScaled {
  const char* name;
  float min, max, scale;
  uint16_t* storage;
};
constexpr PersistedScaled ScaledSetting(const char* name, float min, float max, float scale, uint16_t* storage) {
  return {name, min, max, scale, storage};
}

// --- Instant: persisted AND applied to live state immediately ---

struct InstantUint {
  const char* name;
  uint32_t min, max;
  uint8_t width;  // sizeof(*storage): 1, 2 or 4
  void* storage;
};
constexpr InstantUint InstantUintSetting(const char* name, uint32_t min, uint32_t max, auto* storage) {
  return {name, min, max, static_cast<uint8_t>(sizeof(*storage)), storage};
}

struct InstantInt {
  const char* name;
  int32_t min, max;
  int32_t* storage;
};
constexpr InstantInt InstantIntSetting(const char* name, int32_t min, int32_t max, int32_t* storage) {
  return {name, min, max, storage};
}

struct InstantScaled {
  const char* name;
  float min, max, scale;
  uint16_t* storage;
};
constexpr InstantScaled InstantScaledSetting(const char* name, float min, float max, float scale, uint16_t* storage) {
  return {name, min, max, scale, storage};
}

struct InstantBool {
  const char* name;
  bool* storage;
};
constexpr InstantBool InstantBoolSetting(const char* name, bool* storage) {
  return {name, storage};
}

// Instant settings whose live representation differs from the NVM value.
// on_load() reproduces the boot-time NVM->live load, apply() the POST-time
// update (validated, scaled value), save() persists the edited value and
// read_edit() returns the value shown in the UI.
struct InstantCodec {
  const char* name;
  float min, max, scale;
  void (*on_load)(BatteryEmulatorSettingsStore&, const char*);
  void (*apply)(int32_t);
  void (*save)(BatteryEmulatorSettingsStore&, const char*, float);
  float (*read_edit)();
};
constexpr InstantCodec CodecSetting(const char* name, float min, float max, float scale,
                                    void (*on_load)(BatteryEmulatorSettingsStore&, const char*), void (*apply)(int32_t),
                                    void (*save)(BatteryEmulatorSettingsStore&, const char*, float),
                                    float (*read_edit)()) {
  return {name, min, max, scale, on_load, apply, save, read_edit};
}

// --- Volatile: not persisted, hooks only ---

struct VolatileUint {
  const char* name;
  uint32_t min, max;
  void (*apply)(uint32_t);
  uint32_t (*read)();
};
constexpr VolatileUint VolatileUintSetting(const char* name, uint32_t min, uint32_t max, void (*apply)(uint32_t),
                                           uint32_t (*read)()) {
  return {name, min, max, apply, read};
}

struct VolatileBool {
  const char* name;
  void (*apply)(bool);
  bool (*read)();
};
constexpr VolatileBool VolatileBoolSetting(const char* name, void (*apply)(bool), bool (*read)()) {
  return {name, apply, read};
}

struct VolatileFloat {
  const char* name;
  float min, max;
  void (*apply)(float);
  float (*read)();
};
constexpr VolatileFloat VolatileFloatSetting(const char* name, float min, float max, void (*apply)(float),
                                             float (*read)()) {
  return {name, min, max, apply, read};
}

struct VolatileScaled {
  const char* name;
  float min, max, scale;
  void (*apply)(float);
  float (*read)();
};
constexpr VolatileScaled VolatileScaledSetting(const char* name, float min, float max, float scale,
                                               void (*apply)(float), float (*read)()) {
  return {name, min, max, scale, apply, read};
}

static constexpr uint8_t SETTING_SECRET = 0x01;  // String only: never echoed back by GET.

// The settings tables, defined in webserver_settings.cpp.
extern const PersistedUint PERSISTED_UINTS[];
extern const PersistedInt PERSISTED_INTS[];
extern const PersistedBool PERSISTED_BOOLS[];
extern const PersistedString PERSISTED_STRINGS[];
extern const PersistedScaled PERSISTED_SCALEDS[];
extern const InstantUint INSTANT_UINTS[];
extern const InstantScaled INSTANT_SCALEDS[];
extern const InstantBool INSTANT_BOOLS[];
extern const VolatileUint VOLATILE_UINTS[];
extern const VolatileBool VOLATILE_BOOLS[];
extern const VolatileFloat VOLATILE_FLOATS[];
extern const VolatileScaled VOLATILE_SCALEDS[];
