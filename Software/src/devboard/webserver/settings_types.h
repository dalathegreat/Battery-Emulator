#pragma once

#include <stdint.h>
#include <string.h>
#include <array>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

// ---------------------------------------------------------------------------
// Compile-time validation constants & helpers.
//
// The record factories are always called at constant evaluation (inside
// build_tables() in settings.cpp), but their arguments reach them as ordinary
// function parameters, which are NOT constant expressions inside the body - so
// value-based checks cannot be static_asserts inside the factories (only
// type-based checks, like storage width, can). Instead the factories store the
// raw values, and the table-level static_asserts in settings.cpp validate every
// collected record through the valid_setting()/names_unique() helpers below.
// A violation is a compile error, with the offending values shown in the
// compiler note (e.g. `(50 <= 10)` for a min/max mix-up) and the first failing
// record's name available as FIRST_INVALID_SETTING in settings.cpp.
// ---------------------------------------------------------------------------

// NVM layer key limit (comm_nvm.cpp): persisted/instant keys are stored in NVM
// and must be exactly 1..MAX_SETTING_NAME_LEN chars. Volatile keys never reach
// NVM, so they only have to be non-empty (several TMP_* keys are longer).
static constexpr uint8_t MAX_SETTING_NAME_LEN = 15;

static constexpr uint8_t SETTING_SECRET = 0x01;  // String only: never echoed back by GET.

constexpr size_t setting_name_len(const char* name) {
  size_t n = 0;
  while (name != nullptr && name[n] != '\0') {
    ++n;
  }
  return n;
}
constexpr bool valid_setting_name(const char* name) {
  return name != nullptr && setting_name_len(name) >= 1 && setting_name_len(name) <= MAX_SETTING_NAME_LEN;
}
constexpr bool valid_volatile_name(const char* name) {
  return setting_name_len(name) >= 1;
}
constexpr bool setting_names_equal(const char* a, const char* b) {
  if (a == nullptr || b == nullptr) {
    return false;
  }
  while (*a != '\0' && *b != '\0') {
    if (*a != *b) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == *b;
}

// Storage width for the numeric settings (1/2/4 bytes). Single place that
// verifies the storage *type*; used by UintSetting/InstantUintSetting/
// VolatileUintSetting so they cannot drift apart (and InstantUintSetting got
// the check that UintSetting already had).
template <typename T>
constexpr uint8_t uint_storage_width(T* storage) {
  using StorageType = std::remove_cv_t<T>;
  static_assert(std::is_unsigned_v<StorageType> || std::is_enum_v<StorageType>,
                "numeric setting storage must be an unsigned integer or an enum");
  static_assert(sizeof(StorageType) == 1 || sizeof(StorageType) == 2 || sizeof(StorageType) == 4,
                "numeric setting storage must be uint8_t, uint16_t or uint32_t");
  return static_cast<uint8_t>(sizeof(StorageType));
}

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
  return {name, min, max, uint_storage_width(storage), storage};
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
  return {name, min, max, uint_storage_width(storage), storage};
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

// A volatile setting either mirrors a live variable directly (preferred, no
// callbacks) or needs hooks to convert the web value to/from the live state.
// `width` discriminates the two modes: 1/2/4 bytes means stored mode (the
// union holds `storage`, `apply` is unused); 0 means hook mode (the union
// holds `read`, and a real `apply` callback is present).
struct VolatileUint {
  const char* name;
  uint32_t min, max;
  uint8_t width;            // 0 = hook mode, 1/2/4 = stored width
  void (*apply)(uint32_t);  // hook mode only
  union {
    void* storage;       // stored mode
    uint32_t (*read)();  // hook mode
  };
};
constexpr VolatileUint VolatileUintSetting(const char* name, uint32_t min, uint32_t max, auto* storage) {
  VolatileUint s{};
  s.name = name;
  s.min = min;
  s.max = max;
  s.width = uint_storage_width(storage);
  s.storage = storage;
  return s;
}
constexpr VolatileUint VolatileUintHooked(const char* name, uint32_t min, uint32_t max, void (*apply)(uint32_t),
                                          uint32_t (*read)()) {
  VolatileUint s{};
  s.name = name;
  s.min = min;
  s.max = max;
  s.width = 0;
  s.apply = apply;
  s.read = read;
  return s;
}

// A volatile boolean mirrors a live variable directly (preferred, no
// callbacks) or needs hooks to convert the web value to/from the live state.
// A non-null `storage` means stored mode (`apply`/`read` are unused); a null
// `storage` means hook mode (a real `apply` callback and `read` are present).
struct VolatileBool {
  const char* name;
  bool* storage;        // stored mode (non-null); hook mode leaves it null
  void (*apply)(bool);  // hook mode only
  bool (*read)();       // hook mode only
};
constexpr VolatileBool VolatileBoolSetting(const char* name, bool* storage) {
  VolatileBool s{};
  s.name = name;
  s.storage = storage;
  return s;
}
constexpr VolatileBool VolatileBoolHooked(const char* name, void (*apply)(bool), bool (*read)()) {
  VolatileBool s{};
  s.name = name;
  s.apply = apply;
  s.read = read;
  return s;
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

// ---------------------------------------------------------------------------
// Validity checks for the collected records. One shared rule per record
// *shape*: NVM-backed records (Persisted_*/Instant_*/InstantCodec) carry a
// storage pointer and obey the NVM key-length limit; the pure-hook volatile
// records (VolatileFloat/VolatileScaled) and the dual-mode VolatileUint/
// VolatileBool are checked by their own rules. valid_setting() is the single
// entry point the table-level static_asserts in settings.cpp call.
// ---------------------------------------------------------------------------

// NVM-backed records: 1..15-char key (NVM limit), min <= max, scale > 0,
// storage pointer set, string key length/flags sane, codec hooks set.
// The `requires` branches only apply to records that have the member.
template <typename T>
constexpr bool valid_nvm_record(const T& s) {
  bool ok = valid_setting_name(s.name);
  if constexpr (requires {
                  s.min;
                  s.max;
                }) {
    ok = ok && s.min <= s.max;
  }
  if constexpr (requires { s.scale; }) {
    ok = ok && s.scale > 0;
  }
  if constexpr (requires { s.storage; }) {
    ok = ok && s.storage != nullptr;
  }
  if constexpr (requires { s.max_length; }) {
    ok = ok && s.max_length > 0;
  }
  if constexpr (requires { s.flags; }) {
    ok = ok && (s.flags & ~SETTING_SECRET) == 0;
  }
  if constexpr (requires {
                  s.on_load;
                  s.save;
                  s.read_edit;
                }) {
    ok = ok && s.on_load != nullptr && s.save != nullptr && s.read_edit != nullptr;
  }
  return ok;
}

// Pure-hook volatile records (VolatileFloat/VolatileScaled): non-empty key,
// min <= max, scale > 0, apply/read hooks set.
template <typename T>
constexpr bool valid_volatile_hook_record(const T& s) {
  bool ok = valid_volatile_name(s.name);
  if constexpr (requires {
                  s.min;
                  s.max;
                }) {
    ok = ok && s.min <= s.max;
  }
  if constexpr (requires { s.scale; }) {
    ok = ok && s.scale > 0;
  }
  if constexpr (requires {
                  s.apply;
                  s.read;
                }) {
    ok = ok && s.apply != nullptr && s.read != nullptr;
  }
  return ok;
}

// VolatileUint: width 0 means hook mode (apply/read set), 1/2/4 means stored
// mode (storage pointer set).
constexpr bool valid_volatile_uint_record(const VolatileUint& s) {
  if (s.width == 0) {
    return valid_volatile_name(s.name) && s.apply != nullptr && s.read != nullptr;
  }
  return valid_volatile_name(s.name) && (s.width == 1 || s.width == 2 || s.width == 4) && s.storage != nullptr;
}

// VolatileBool: non-null storage is stored mode, null storage means hook mode.
constexpr bool valid_volatile_bool_record(const VolatileBool& s) {
  if (s.storage != nullptr) {
    return valid_volatile_name(s.name);
  }
  return valid_volatile_name(s.name) && s.apply != nullptr && s.read != nullptr;
}

// Single validity entry point; overload dispatch picks the rule family.
template <typename T>
constexpr bool valid_setting(const T& s) {
  using U = std::remove_cv_t<T>;
  if constexpr (std::is_same_v<U, VolatileUint>) {
    return valid_volatile_uint_record(s);
  } else if constexpr (std::is_same_v<U, VolatileBool>) {
    return valid_volatile_bool_record(s);
  } else if constexpr (std::is_same_v<U, VolatileFloat> || std::is_same_v<U, VolatileScaled>) {
    return valid_volatile_hook_record(s);
  } else {
    return valid_nvm_record(s);
  }
}

template <typename Tuple, size_t... Is>
constexpr bool all_valid_impl(const Tuple& t, std::index_sequence<Is...>) {
  return (valid_setting(std::get<Is>(t)) && ...);
}
template <typename Tuple>
constexpr bool all_valid(const Tuple& t) {
  return all_valid_impl(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

// Name of the first record that fails validation, or nullptr when the whole
// tuple is valid. Exposed as FIRST_INVALID_SETTING in settings.cpp so a build
// failure points at the offending entry by name.
template <typename Tuple, size_t... Is>
constexpr const char* first_invalid_name_impl(const Tuple& t, std::index_sequence<Is...>) {
  const char* first = nullptr;
  ((((first == nullptr) && !valid_setting(std::get<Is>(t))) && ((first = std::get<Is>(t).name), true)), ...);
  return first;
}
template <typename Tuple>
constexpr const char* first_invalid_name(const Tuple& t) {
  return first_invalid_name_impl(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

// No two records in the whole table may share a key: the web API merges all
// tables into one JSON settings object, so a duplicate would clobber itself.
template <typename Tuple, size_t I, size_t J>
constexpr bool names_conflict_at(const Tuple& t) {
  if constexpr (I == J) {
    return false;
  }
  return setting_names_equal(std::get<I>(t).name, std::get<J>(t).name);
}
template <typename Tuple, size_t I, size_t... Js>
constexpr bool names_unique_for(const Tuple& t, std::index_sequence<Js...>) {
  return ((Js <= I || !names_conflict_at<Tuple, I, Js>(t)) && ...);
}
template <typename Tuple, size_t... Is>
constexpr bool names_unique_impl(const Tuple& t, std::index_sequence<Is...>) {
  return (names_unique_for<Tuple, Is>(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{}) && ...);
}
template <typename Tuple>
constexpr bool names_unique(const Tuple& t) {
  return names_unique_impl(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

// ---------------------------------------------------------------------------
// Table assembly. Every setting is written exactly once, as a call to its
// record factory, inside the single ALL_SETTINGS tuple built in settings.cpp
// (function build_tables()). collect<SplitType>() then splits that tuple at
// compile time into the per-class tables the handlers walk
// (PERSISTED_*/INSTANT_*/VOLATILE_*). The master tuple only exists during
// constant evaluation and is never emitted; the collected std::arrays are the
// only output (and live in .rodata, same as the old C arrays).
// ---------------------------------------------------------------------------
// Step one tuple element into the output array. `if constexpr` discards the
// assignment entirely for non-matching element types, so the array type and
// the tuple element type never collide in one expression.
template <typename Needle, size_t Count, typename Tuple, size_t I>
constexpr void collect_one(const Tuple& t, std::array<Needle, Count>& out, size_t& n) {
  if constexpr (std::is_same_v<std::tuple_element_t<I, Tuple>, Needle>) {
    out[n++] = std::get<I>(t);
  }
}

template <typename Needle, typename Tuple, size_t... Is>
constexpr auto collect_impl(const Tuple& t, std::index_sequence<Is...>) {
  constexpr size_t count = ((std::is_same_v<std::tuple_element_t<Is, Tuple>, Needle>)+...);
  std::array<Needle, count> out{};
  size_t n = 0;
  (collect_one<Needle, count, Tuple, Is>(t, out, n), ...);
  return out;
}
template <typename Needle, typename Tuple>
constexpr auto collect(const Tuple& t) {
  return collect_impl<Needle>(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}
