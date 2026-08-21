#ifndef BE_DEVBOARD_SETTINGS_SETTINGS_ACCESSORS_H
#define BE_DEVBOARD_SETTINGS_SETTINGS_ACCESSORS_H

#include <WString.h>
#include "settings_table.h"

// Typed access to one setting, generated from its row.
//
// A call site that reads a setting by key has to repeat three facts the table
// already states - the key's spelling, its on-flash NVS type and its default -
// and NVS punishes getting the type wrong silently: a typed read on a key
// stored under a different tag returns the default instead of the stored
// value, which is how a refactor loses a user's settings with no visible
// symptom (the failure the boot audit in comm_nvm.cpp exists to catch).
//
// Here the row supplies all three. setting_get<Sid::X>() picks the store call
// from the row's kind, so the type it reads with IS the type the table
// declares, and its return type follows: bool for BoolU8 rows, uint32_t for
// U32, int32_t for I32, String for Str. A call site cannot ask for a setting
// under the wrong type, because it never names a type at all - only the id.
//
// The store is a template parameter rather than BatteryEmulatorSettingsStore
// spelled out, purely to keep this header a leaf like settings_table.h; the
// get/save vocabulary is that class's. Loader-only semantics stay in the
// loader: SF_SKIP_IF_ZERO rows read back their stored value like any other -
// "0 means leave the destination alone" is the applier's business, not the
// key's.

template <SettingKind K>
struct SettingValueType;

template <>
struct SettingValueType<SettingKind::BoolU8> {
  using type = bool;
};
template <>
struct SettingValueType<SettingKind::U32> {
  using type = uint32_t;
};
template <>
struct SettingValueType<SettingKind::I32> {
  using type = int32_t;
};
template <>
struct SettingValueType<SettingKind::Str> {
  using type = String;
};

template <Sid S>
inline constexpr const SettingDesc& setting_row_v = SETTINGS_TABLE[static_cast<size_t>(S)];

template <Sid S>
using setting_value_t = typename SettingValueType<setting_row_v<S>.kind>::type;

// The stored value, or the row's default when the key was never stored - or
// was stored under a different type tag, where the typed read defaults just
// like real NVS does.
template <Sid S, typename Store>
setting_value_t<S> setting_get(Store& store) {
  constexpr const SettingDesc& row = setting_row_v<S>;
  if constexpr (row.kind == SettingKind::BoolU8) {
    return store.getBool(row.nvs_key, row.def.i != 0);
  } else if constexpr (row.kind == SettingKind::U32) {
    return store.getUInt(row.nvs_key, static_cast<uint32_t>(row.def.i));
  } else if constexpr (row.kind == SettingKind::I32) {
    return store.getInt(row.nvs_key, row.def.i);
  } else {
    return store.getString(row.nvs_key, row.def.s);
  }
}

// Store the value under the row's key with the row's on-flash type. The value
// parameter is already the row's type, so a mistyped save is a compile error
// at the call site, not a mistagged NVS entry.
template <Sid S, typename Store>
void setting_save(Store& store, const setting_value_t<S>& value) {
  constexpr const SettingDesc& row = setting_row_v<S>;
  if constexpr (row.kind == SettingKind::BoolU8) {
    store.saveBool(row.nvs_key, value);
  } else if constexpr (row.kind == SettingKind::U32) {
    store.saveUInt(row.nvs_key, value);
  } else if constexpr (row.kind == SettingKind::I32) {
    store.saveInt(row.nvs_key, value);
  } else {
    store.saveString(row.nvs_key, value.c_str());
  }
}

#endif  // BE_DEVBOARD_SETTINGS_SETTINGS_ACCESSORS_H
