#ifndef _WEBSERVER_SETTINGS_HANDLERS_H_
#define _WEBSERVER_SETTINGS_HANDLERS_H_

#include "../../communication/nvm/comm_nvm.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "webserver_settings.h"

#include <cmath>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Table-driven helper machinery for the settings tables in webserver_settings.cpp.
//
// For every record type there is one overloaded handler plus a generic walker
// that drives a whole table. Overload resolution picks the handler by the
// static element type; the element count `N` is deduced from the array, so a
// table and its consumers can never drift apart (no *_COUNT globals needed).
//
// All of this is self-contained reusable boilerplate. The `inline` qualifier
// makes it safe to live in a header. The consumer functions that call the
// walkers (load_stored_settings, apply_setting_updates, build_settings_json,
// store_settings_from_live) are deliberately NOT here: template `N` deduction
// requires the array types to be complete at the call site, so those consumers
// stay in webserver_settings.cpp, next to the tables they hand to the walkers.
// ---------------------------------------------------------------------------

namespace {

enum class ParseResult : uint8_t { Ok, Invalid, OutOfRange };

inline ParseResult parse_uint(const char* str, uint32_t min, uint32_t max, uint32_t* out) {
  char* end = nullptr;
  unsigned long val = strtoul(str, &end, 10);
  if (end == nullptr || *end != 0) {
    return ParseResult::Invalid;
  }
  if (val < min || val > max) {
    return ParseResult::OutOfRange;
  }
  *out = (uint32_t)val;
  return ParseResult::Ok;
}

inline ParseResult parse_int(const char* str, int32_t min, int32_t max, int32_t* out) {
  char* end = nullptr;
  long val = strtol(str, &end, 10);
  if (end == nullptr || *end != 0) {
    return ParseResult::Invalid;
  }
  if (val < min || val > max) {
    return ParseResult::OutOfRange;
  }
  *out = (int32_t)val;
  return ParseResult::Ok;
}

inline ParseResult parse_float(const char* str, float min, float max, float* out) {
  char* end = nullptr;
  float val = strtof(str, &end);
  if (end == nullptr || *end != 0) {
    return ParseResult::Invalid;
  }
  if (val < min || val > max) {
    return ParseResult::OutOfRange;
  }
  *out = val;
  return ParseResult::Ok;
}

inline void set_parse_error(JsonDocument& errors, const char* name, ParseResult r) {
  errors[name] = (r == ParseResult::Invalid) ? "Invalid value." : "Value out of range.";
}

// Read/write `width` bytes (1, 2 or 4) as a uint32. Aliasing-safe for the 4-byte
// case (enums, ints) via memcpy.
inline uint32_t readSettingValue(const void* storage, uint8_t width) {
  if (width == 1) {
    return *(const uint8_t*)storage;
  }
  if (width == 2) {
    return *(const uint16_t*)storage;
  }
  uint32_t value;
  memcpy(&value, storage, 4);
  return value;
}
inline void writeSettingValue(void* storage, uint8_t width, uint32_t value) {
  if (width == 1) {
    *(uint8_t*)storage = (uint8_t)value;
  } else if (width == 2) {
    *(uint16_t*)storage = (uint16_t)value;
  } else {
    *(uint32_t*)storage = value;
  }
}

// ---------------------------------------------------------------------------
// load_setting: NVM -> storage. Volatile settings are never persisted, so only
// the PERSISTED_/INSTANT_ record types have a loader.
// ---------------------------------------------------------------------------

inline void load_setting(const PersistedUint& s, BatteryEmulatorSettingsStore& settings) {
  writeSettingValue(s.storage, s.width, settings.getUInt(s.name, readSettingValue(s.storage, s.width)));
}
inline void load_setting(const PersistedInt& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = settings.getInt(s.name, *s.storage);
}
inline void load_setting(const PersistedBool& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = settings.getBool(s.name, *s.storage);
}
inline void load_setting(const PersistedString& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = settings.getString(s.name, s.storage->c_str()).c_str();
}
inline void load_setting(const PersistedScaled& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = (uint16_t)settings.getUInt(s.name, *s.storage);
}
inline void load_setting(const InstantUint& s, BatteryEmulatorSettingsStore& settings) {
  writeSettingValue(s.storage, s.width, settings.getUInt(s.name, readSettingValue(s.storage, s.width)));
}
inline void load_setting(const InstantInt& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = settings.getInt(s.name, *s.storage);
}
inline void load_setting(const InstantScaled& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = (uint16_t)settings.getUInt(s.name, *s.storage);
}
inline void load_setting(const InstantBool& s, BatteryEmulatorSettingsStore& settings) {
  *s.storage = settings.getBool(s.name, *s.storage);
}
inline void load_setting(const InstantCodec& s, BatteryEmulatorSettingsStore& settings) {
  s.on_load(settings, s.name);
}

// ---------------------------------------------------------------------------
// store_setting: live storage -> NVM. Only the runtime-mutable (INSTANT_)
// settings are swept back to NVM.
// ---------------------------------------------------------------------------

inline void store_setting(const InstantUint& s, BatteryEmulatorSettingsStore& settings) {
  settings.saveUInt(s.name, readSettingValue(s.storage, s.width));
}
inline void store_setting(const InstantInt& s, BatteryEmulatorSettingsStore& settings) {
  settings.saveInt(s.name, *s.storage);
}
inline void store_setting(const InstantScaled& s, BatteryEmulatorSettingsStore& settings) {
  settings.saveUInt(s.name, *s.storage);
}
inline void store_setting(const InstantBool& s, BatteryEmulatorSettingsStore& settings) {
  settings.saveBool(s.name, *s.storage);
}
inline void store_setting(const InstantCodec& s, BatteryEmulatorSettingsStore& settings) {
  s.save(settings, s.name, s.read_edit());
}

// ---------------------------------------------------------------------------
// process_setting: validate/apply a value from a POST body. Handlers must
// validate (recording parse errors) on every pass, so a bad value fails the
// validation pass before anything is written, and must only touch NVM/live
// state once `save` is true.
//
// PERSISTED_* settings are stored in NVM and only flag a reboot when a value
// actually changed. INSTANT_* settings are also stored but applied immediately.
// VOLATILE_* settings are never stored, only applied.
// ---------------------------------------------------------------------------

// PersistedUint: parsed as unsigned, saved as uint32.
inline void process_setting(const PersistedUint& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool& reboot_required) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  uint32_t val;
  ParseResult r = parse_uint(doc[s.name].as<const char*>(), s.min, s.max, &val);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save && settings.saveUInt(s.name, val)) {
    reboot_required = true;
  }
}

// PersistedInt: parsed as signed int32.
inline void process_setting(const PersistedInt& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool& reboot_required) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  int32_t val;
  ParseResult r = parse_int(doc[s.name].as<const char*>(), s.min, s.max, &val);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save && settings.saveInt(s.name, val)) {
    reboot_required = true;
  }
}

// PersistedBool: accepted as "true"/"1", anything else is treated as false.
inline void process_setting(const PersistedBool& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool& reboot_required) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  const char* str = doc[s.name].as<const char*>();
  bool bval = (strcmp(str, "true") == 0 || strcmp(str, "1") == 0);
  if (save && settings.saveBool(s.name, bval)) {
    reboot_required = true;
  }
}

// PersistedString: length-checked. Empty secrets are skipped so a blanked-out
// field can't wipe a stored password without the user typing a replacement.
inline void process_setting(const PersistedString& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool& reboot_required) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  const char* str = doc[s.name].as<const char*>();
  if ((s.flags & SETTING_SECRET) && strlen(str) == 0) {
    return;
  }
  if (strlen(str) > s.max_length) {
    errors[s.name] = "Value too long.";
  } else if (save && settings.saveString(s.name, str)) {
    reboot_required = true;
  }
}

// PersistedScaled: edited as float, stored as uint32_t = value * scale.
inline void process_setting(const PersistedScaled& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool& reboot_required) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  float fval;
  ParseResult r = parse_float(doc[s.name].as<const char*>(), s.min, s.max, &fval);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save && settings.saveUInt(s.name, (uint32_t)(fval * s.scale))) {
    reboot_required = true;
  }
}

// Instant settings are persisted and applied immediately; saving them never
// demands a reboot.

// InstantUint: persisted and written to live storage.
inline void process_setting(const InstantUint& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  uint32_t val;
  ParseResult r = parse_uint(doc[s.name].as<const char*>(), s.min, s.max, &val);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    settings.saveUInt(s.name, val);
    writeSettingValue(s.storage, s.width, val);
  }
}

// InstantInt: persisted and written to live storage.
inline void process_setting(const InstantInt& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  int32_t val;
  ParseResult r = parse_int(doc[s.name].as<const char*>(), s.min, s.max, &val);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    settings.saveInt(s.name, val);
    *s.storage = val;
  }
}

// InstantScaled: persisted and written to live storage, scaled.
inline void process_setting(const InstantScaled& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  float fval;
  ParseResult r = parse_float(doc[s.name].as<const char*>(), s.min, s.max, &fval);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    uint32_t val = (uint32_t)(fval * s.scale);
    settings.saveUInt(s.name, val);
    *s.storage = (uint16_t)val;
  }
}

// InstantBool: persisted and written to live storage.
inline void process_setting(const InstantBool& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  const char* str = doc[s.name].as<const char*>();
  bool bval = (strcmp(str, "true") == 0 || strcmp(str, "1") == 0);
  if (save) {
    settings.saveBool(s.name, bval);
    *s.storage = bval;
  }
}

// InstantCodec: persisted via the codec's save() hook, applied via apply().
inline void process_setting(const InstantCodec& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  float fval;
  ParseResult r = parse_float(doc[s.name].as<const char*>(), s.min, s.max, &fval);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    int32_t val = (int32_t)(fval * s.scale);
    s.save(settings, s.name, fval);
    s.apply(val);
  }
}

// Volatile settings are never persisted. Stored-mode VolatileUints (width
// 1/2/4) write/read the live variable directly; hook-mode entries use apply().

inline void process_setting(const VolatileUint& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  uint32_t val;
  ParseResult r = parse_uint(doc[s.name].as<const char*>(), s.min, s.max, &val);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    if (s.width == 0) {
      s.apply(val);
    } else {
      writeSettingValue(s.storage, s.width, val);
    }
  }
}

inline void process_setting(const VolatileBool& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  const char* str = doc[s.name].as<const char*>();
  bool bval = (strcmp(str, "true") == 0 || strcmp(str, "1") == 0);
  if (save) {
    s.apply(bval);
  }
}

inline void process_setting(const VolatileFloat& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  float fval;
  ParseResult r = parse_float(doc[s.name].as<const char*>(), s.min, s.max, &fval);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    s.apply(fval);
  }
}

inline void process_setting(const VolatileScaled& s, const JsonDocument& doc, JsonDocument& errors,
                            BatteryEmulatorSettingsStore& settings, bool save, bool&) {
  if (!doc[s.name].is<const char*>()) {
    return;
  }
  float fval;
  ParseResult r = parse_float(doc[s.name].as<const char*>(), s.min, s.max, &fval);
  if (r != ParseResult::Ok) {
    set_parse_error(errors, s.name, r);
  } else if (save) {
    s.apply(fval * s.scale);
  }
}

// ---------------------------------------------------------------------------
// emit_setting: write one setting into the JSON settings object. PERSISTED_*
// and INSTANT_* show the stored value when present (so a change still awaiting
// its reboot is visible), else live storage; VOLATILE_* use read hooks.
// ---------------------------------------------------------------------------

inline void emit_setting(const PersistedUint& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  sets[s.name] = settings.getUInt(s.name, readSettingValue(s.storage, s.width));
}
inline void emit_setting(const PersistedInt& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  sets[s.name] = settings.getInt(s.name, *s.storage);
}
inline void emit_setting(const PersistedBool& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  sets[s.name] = settings.getBool(s.name, *s.storage);
}
inline void emit_setting(const PersistedString& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  if (!(s.flags & SETTING_SECRET)) {
    sets[s.name] = settings.getString(s.name, s.storage->c_str());
  }
}
inline void emit_setting(const PersistedScaled& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  sets[s.name] = (float)settings.getUInt(s.name, *s.storage) / s.scale;
}
inline void emit_setting(const InstantUint& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  sets[s.name] = settings.getUInt(s.name, readSettingValue(s.storage, s.width));
}
inline void emit_setting(const InstantInt& s, JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  sets[s.name] = settings.getInt(s.name, *s.storage);
}
inline void emit_setting(const InstantScaled& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  sets[s.name] = (float)*s.storage / s.scale;
}
inline void emit_setting(const InstantBool& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  sets[s.name] = *s.storage;
}
inline void emit_setting(const InstantCodec& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  sets[s.name] = s.read_edit();
}
inline void emit_setting(const VolatileUint& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  sets[s.name] = (s.width == 0) ? s.read() : readSettingValue(s.storage, s.width);
}
inline void emit_setting(const VolatileBool& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  sets[s.name] = s.read();
}
inline void emit_setting(const VolatileFloat& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  float value = s.read();
  if (!std::isnan(value)) {
    sets[s.name] = value;
  }
}
inline void emit_setting(const VolatileScaled& s, JsonObject& sets, BatteryEmulatorSettingsStore&) {
  float value = s.read();
  if (!std::isnan(value)) {
    sets[s.name] = value / s.scale;
  }
}

// ---------------------------------------------------------------------------
// Generic walkers: apply the correct overload above to every element of a
// settings table. `N` is deduced from the array declaration, so a table's
// element count can never drift from the loops that consume it.
// ---------------------------------------------------------------------------

template <typename T, size_t N>
void load_all_settings(const T (&arr)[N], BatteryEmulatorSettingsStore& settings) {
  for (size_t i = 0; i < N; i++) {
    load_setting(arr[i], settings);
  }
}

template <typename T, size_t N>
void store_all_settings(const T (&arr)[N], BatteryEmulatorSettingsStore& settings) {
  for (size_t i = 0; i < N; i++) {
    store_setting(arr[i], settings);
  }
}

template <typename T, size_t N>
void process_all_settings(const T (&arr)[N], const JsonDocument& doc, JsonDocument& errors,
                          BatteryEmulatorSettingsStore& settings, bool save, bool& reboot_required) {
  for (size_t i = 0; i < N; i++) {
    process_setting(arr[i], doc, errors, settings, save, reboot_required);
  }
}

template <typename T, size_t N>
void emit_all_settings(const T (&arr)[N], JsonObject& sets, BatteryEmulatorSettingsStore& settings) {
  for (size_t i = 0; i < N; i++) {
    emit_setting(arr[i], sets, settings);
  }
}

}  // namespace

#endif  // _WEBSERVER_SETTINGS_HANDLERS_H_
