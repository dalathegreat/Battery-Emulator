#include "settings.h"
#include "settings_handlers.h"
#include "settings_types.h"
#include "webserver_new.h"

#include <cmath>
#include <cstdlib>

#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../charger/CHARGERS.h"
#include "../../charger/CanCharger.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../communication/precharge_control/precharge_control.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/mqtt/mqtt.h"
#include "../../devboard/network/hostname.h"  // custom_hostname
#include "../../devboard/utils/logging.h"
#include "../../devboard/utils/types.h"
#include "../../devboard/wifi/wifi.h"
#include "../../inverter/INVERTERS.h"
#include "../../inverter/InverterProtocol.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../../shunt/Shunt.h"

// Static IP settings are stored and edited as dotted-quad strings, but loaded
// into an IPAddress on boot, and can't be edited at runtime. We don't actually
// need to store these, but StringSetting can't handle a nullptr storage
// argument.
static std::string edited_static_local_IP, edited_static_gateway, edited_static_subnet, edited_static_dns;

// Stored as tenths of a percent, rather than hundredths like in datalayer.
static int32_t min_percentage_tenths, max_percentage_tenths;

// Backing bool for the HADISCFWU PersistentBool, used once at boot.
static bool republish_ha_on_update = false;

// ---------------------------------------------------------------------------
// Settings tables
// ---------------------------------------------------------------------------

// PERSISTED (normal) settings are stored in NVM and only applied to the live state at boot.
//   UintSetting: unsigned integer (can be uint8/16/32 or enum).
//   IntSetting: signed integer (int32).
//   ScaledSetting: float, scaled to a uint16 for storage by a given factor.
//   BoolSetting: boolean.
//   StringSetting: string, stored in NVM as a null-terminated C string.
//
// INSTANT settings are stored in NVM and applied to the live state immediately.
//   InstantUintSetting: unsigned integer (can be uint8/16/32 or enum).
//   InstantIntSetting: signed integer (int32).
//   InstantScaledSetting: float, scaled to a uint16 for storage by a given factor.
//   InstantBoolSetting: boolean.
//
// VOLATILE settings are never stored in NVM, only applied to the live state.
//   VolatileUintSetting: unsigned integer (can be uint8/16/32 or enum), get/set directly from a live variable.
//   VolatileUintHooked: unsigned integer (can be uint8/16/32 or enum), get/set via apply()/read() hooks.
//   VolatileBoolSetting: boolean, get/set directly from a live variable.
//   VolatileBoolHooked: boolean, get/set via apply()/read() hooks.

// clang-format off

constexpr auto build_tables() {
  constexpr std::tuple settings{
    // --- Unsigned ints and enums, persisted, reboot-required ---
    // (name, min, max, storage pointer)
    UintSetting("INVTYPE", 0, (uint32_t)InverterProtocolType::Highest - 1, &user_selected_inverter_protocol),
    UintSetting("INVCOMM", 0, (uint32_t)comm_interface::Highest - 1, &user_selected_inv_comm),
    UintSetting("BATTTYPE", 0, (uint32_t)BatteryType::Highest - 1, &user_selected_battery_type),
    UintSetting("BATTCHEM", 0, (uint32_t)battery_chemistry_enum::Highest - 1, &user_selected_battery_chemistry),
    UintSetting("BATTCOMM", 0, (uint32_t)comm_interface::Highest - 1, &user_selected_batt_comm),
    UintSetting("BATTCVMAX", 0, 5000, &user_selected_max_cell_voltage_mV),
    UintSetting("BATTCVMIN", 0, 5000, &user_selected_min_cell_voltage_mV),
    UintSetting("CHGTYPE", 0, (uint32_t)ChargerType::Highest - 1, &user_selected_charger_type),
    UintSetting("CHGCOMM", 0, (uint32_t)comm_interface::Highest - 1, &user_selected_chg_comm),
    UintSetting("EQSTOP", 0, (uint32_t)STOP_BUTTON_BEHAVIOR::Highest - 1, &equipment_stop_behavior),
    UintSetting("BATT2COMM", 0, (uint32_t)comm_interface::Highest - 1, &user_selected_batt2_comm),
    UintSetting("BATT3COMM", 0, (uint32_t)comm_interface::Highest - 1, &user_selected_batt3_comm),
    UintSetting("SHUNTTYPE", 0, (uint32_t)ShuntType::Highest - 1, &user_selected_shunt_type),
    UintSetting("SHUNTCOMM", 0, (uint32_t)comm_interface::Highest - 1, &user_selected_shunt_comm),
    UintSetting("MAXPRETIME", 0, 120000, &precharge_max_precharge_time_before_fault),
    UintSetting("MAXPREFREQ", 0, 65535, &Precharge_max_PWM_Freq),
    UintSetting("WIFICHANNEL", 0, 14, &wifi_channel),
    UintSetting("DCHGPOWER", 0, 100000, &datalayer.battery.status.override_discharge_power_W),
    UintSetting("CHGPOWER", 0, 100000, &datalayer.battery.status.override_charge_power_W),
    UintSetting("MQTTPORT", 0, 65535, &mqtt_port),
    UintSetting("MQTTTIMEOUT", 0, 30000, &mqtt_timeout_ms),
    UintSetting("MQTTPUBLISHMS", 0, 3600000, &mqtt_publish_interval_ms),
    UintSetting("SOFAR_ID", 0, 255, &datalayer.battery.settings.sofar_user_specified_battery_id),
    UintSetting("INVCELLS", 0, 65535, &user_selected_inverter_cells),
    UintSetting("INVMODULES", 0, 65535, &user_selected_inverter_modules),
    UintSetting("INVCELLSPER", 0, 65535, &user_selected_inverter_cells_per_module),
    UintSetting("INVVLEVEL", 0, 65535, &user_selected_inverter_voltage_level),
    UintSetting("INVCAPACITY", 0, 65535, &user_selected_inverter_ah_capacity),
    UintSetting("INVBTYPE", 0, 255, &user_selected_inverter_battery_type),
    UintSetting("INVICNT", 0, 2, &user_selected_inverter_contactor_mode),
    UintSetting("PRECHGMS", 0, 120000, &precharge_time_ms),
    UintSetting("PWMFREQ", 0, 65535, &pwm_frequency),
    UintSetting("PWMHOLD", 0, 1023, &pwm_hold_duty),
    UintSetting("GTWCOUNTRY", 0, 65535, &user_selected_tesla_GTW_country),
    UintSetting("GTWMAPREG", 0, 9, &user_selected_tesla_GTW_mapRegion),
    UintSetting("GTWCHASSIS", 0, 9, &user_selected_tesla_GTW_chassisType),
    UintSetting("GTWPACK", 0, 9, &user_selected_tesla_GTW_packEnergy),
    UintSetting("LEDMODE", 0, 10, &datalayer.battery.status.led_mode),
#ifdef HW_LILYGO2CAN
    UintSetting("GPIOOPT1", 0, 255, &user_selected_gpioopt1),
#endif
    UintSetting("GPIOOPT2", 0, 255, &user_selected_gpioopt2),
    UintSetting("GPIOOPT3", 0, 255, &user_selected_gpioopt3),
    UintSetting("GPIOOPT4", 0, 255, &user_selected_gpioopt4),
#ifdef HW_STARK
    UintSetting("GPIOOPT5", 0, 255, &user_selected_gpioopt5),
#endif
#ifdef HW_WAVESHARE
    UintSetting("GPIOOPT6", 0, 255, &user_selected_gpioopt6),
#endif
    UintSetting("INVSUNTYPE", 0, 255, &user_selected_inverter_sungrow_type),
    UintSetting("CTVNOM", 0, 65535, &ct_clamp_nominal_voltage_dV),
    UintSetting("CTANOM", 0, 65535, &ct_clamp_nominal_current_A),
    UintSetting("CTATTEN", 0, (uint32_t)adc_attenuation_enum::Highest - 1, &ct_clamp_pin_atten),
    UintSetting("PYLONBAUD", 0, 1000000, &user_selected_pylon_baudrate),
    UintSetting("PYLONBRAND", 0, 255, &user_selected_inverter_pylon_type),
    UintSetting("DALYPWRPCT", 0, 10000, &user_selected_daly_power_per_percent),
    UintSetting("DALYPWRDV", 0, 10000, &user_selected_daly_power_per_dV),
    UintSetting("DALYDVSTART", 0, 255, &user_selected_daly_power_per_dV_start),
    UintSetting("DALYPWRDEG", 0, 10000, &user_selected_daly_power_per_degree_C),
    UintSetting("DALYPWR0C", 0, 100000, &user_selected_daly_power_at_0_degree_C),
    UintSetting("PYLONSEND", 0, 1, &user_selected_pylon_send),
    UintSetting("CHGTAPERSTART", 0, 100, &charge_taper_start_soc),
    UintSetting("CHGTAPERFLOOR", 0, 2000, &charge_taper_floor_W),
    UintSetting("PERBMSRESETH", 24, 48, &periodic_bms_reset_interval_h),
    UintSetting("FOXESSTYPE", 0, 255, &user_selected_inverter_foxess_type),
    UintSetting("FOXESSSUBTYPE", 0, 255, &user_selected_inverter_foxess_subtype),
    UintSetting("FOXESSMODULES", 0, 255, &user_selected_inverter_foxess_modules),
    UintSetting("SYSLOGPORT", 0, 65535, &syslog_port),
    UintSetting("SYSLOGFAC", 0, 23, &syslog_facility),
    // --- Signed numeric settings, persisted, reboot-required ---
    // (name, min, max, storage pointer)
    IntSetting("CPUTEMPOFFSET", -100, 100, &datalayer.system.info.CPU_temperature_calibration_offset),
    // --- Booleans, persisted, reboot-required ---
    // (name, storage pointer)
    BoolSetting("DBLBTR", &user_selected_second_battery),
    BoolSetting("CNTCTRL", &contactor_control_enabled),
    BoolSetting("CNTCTRLDBL", &contactor_control_enabled_double_battery),
    BoolSetting("PWMCNTCTRL", &pwm_contactor_control),
    BoolSetting("PERBMSRESET", &periodic_bms_reset),
    BoolSetting("REMBMSRESET", &remote_bms_reset),
    BoolSetting("EXTPRECHARGE", &precharge_control_enabled),
    BoolSetting("NOINVDISC", &precharge_inverter_normally_open_contactor),
    BoolSetting("WIFIAPENABLED", &wifiap_enabled),
    BoolSetting("STATICIP", &wifi_static_IP_enabled),
    BoolSetting("PERFPROFILE", &datalayer.system.info.performance_measurement_active),
    BoolSetting("CANLOGUSB", &datalayer.system.info.CAN_usb_logging_active),
    BoolSetting("USBENABLED", &datalayer.system.info.usb_logging_active),
    BoolSetting("WEBENABLED", &datalayer.system.info.web_logging_active),
#ifdef SDCARD
    BoolSetting("CANLOGSD", &datalayer.system.info.CAN_SD_logging_active),
    BoolSetting("SDLOGENABLED", &datalayer.system.info.SD_logging_active),
#endif  // SDCARD
    BoolSetting("MQTTENABLED", &mqtt_enabled),
    BoolSetting("MQTTCELLV", &mqtt_transmit_all_cellvoltages),
    BoolSetting("HADISC", &ha_autodiscovery_enabled),
    BoolSetting("DEYEBYD", &user_selected_inverter_deye_workaround),
    BoolSetting("INTERLOCKREQ", &user_selected_LEAF_interlock_mandatory),
    BoolSetting("DIGITALHVIL", &user_selected_tesla_digital_HVIL),
    BoolSetting("GTWRHD", &user_selected_tesla_GTW_rightHandDrive),
    BoolSetting("SOCESTIMATED", &user_selected_use_estimated_SOC),
    BoolSetting("PYLONOFFSET", &user_selected_pylon_30koffset),
    BoolSetting("PYLONORDER", &user_selected_pylon_invert_byteorder),
    BoolSetting("NCCONTACTOR", &contactor_control_inverted_logic),
    BoolSetting("TRIBTR", &user_selected_triple_battery),
    BoolSetting("CNTCTRLTRI", &contactor_control_enabled_triple_battery),
    BoolSetting("ESPNOWENABLED", &espnow_enabled),
    BoolSetting("PRIMOGEN24", &user_selected_primo_gen24),
    BoolSetting("USEVOLTLIMITS", &datalayer.battery.settings.user_set_voltage_limits_active),
    BoolSetting("LOWPASSFILTER", &inverter_low_pass_filter),
    BoolSetting("CTINVERT", &ct_invert_current),
    BoolSetting("WEBAUTH", &webserver_auth),
    BoolSetting("CHGTAPERSOC", &charge_taper_soc),
    BoolSetting("SLOWCANINV", &user_selected_inverter_long_CAN_timeout),
    BoolSetting("INVOFFGRID", &user_selected_inverter_offgrid),
    BoolSetting("PERBMSDEFSOC", &periodic_bms_reset_defer_low_soc),
    BoolSetting("PERBMSSKIPBAL", &periodic_bms_reset_skip_balancing),
    BoolSetting("MEASURECPUTEMP", &datalayer.system.info.CPU_measurement_enabled),
    BoolSetting("SYSLOGEN", &datalayer.system.info.syslog_logging_active),
    BoolSetting("CHGESTIMATED", &user_selected_use_estimated_charge_limits),
    BoolSetting("MQTTHEAP", &mqtt_publish_heap_metrics),
    BoolSetting("HADISCFWU", &republish_ha_on_update),
    // --- Strings, persisted, reboot-required ---
    // (name, max length excluding NUL, storage pointer[, SETTING_SECRET])
    StringSetting("SSID", 32, &ssid),
    StringSetting("PASSWORD", 64, &password, SETTING_SECRET),
    StringSetting("APNAME", 64, &ssidAP),
    StringSetting("APPASSWORD", 64, &passwordAP, SETTING_SECRET),
    StringSetting("HOSTNAME", 64, &custom_hostname),
    StringSetting("MQTTSERVER", 64, &mqtt_server),
    StringSetting("MQTTUSER", 64, &mqtt_user),
    StringSetting("MQTTPASSWORD", 64, &mqtt_password, SETTING_SECRET),
    StringSetting("HTTPUSER", 32, &http_username),
    StringSetting("HTTPPASS", 64, &http_password, SETTING_SECRET),
    StringSetting("LOCALIP", 15, &edited_static_local_IP),
    StringSetting("GATEWAY", 15, &edited_static_gateway),
    StringSetting("SUBNET", 15, &edited_static_subnet),
    StringSetting("DNS", 15, &edited_static_dns),
    StringSetting("CTOFFSET", 16, &ct_clamp_offset_text),
    StringSetting("HADISCTOPIC", 64, &ha_autodiscovery_topic),
    StringSetting("SYSLOGIP", 15, &syslog_ip),
    StringSetting("ESPNOWMACS", 180, &espnow_peer_macs),
    // --- Scaled fixed-point, persisted, reboot-required ---
    // (name, min, max, scale factor, storage)
    ScaledSetting("BATTPVMAX", 0.0f, 1000.0f, 10.0f, &user_selected_max_pack_voltage_dV),
    ScaledSetting("BATTPVMIN", 0.0f, 1000.0f, 10.0f, &user_selected_min_pack_voltage_dV),
    // --- Instant unsigned ints, persisted, applied immediately ---
    // (name, min, max, storage pointer)
    InstantUintSetting("BATTERY_WH_MAX", 1, 400000, &datalayer.battery.info.total_capacity_Wh),
    InstantUintSetting("BMSRESETDUR", 0, 60000, &datalayer.battery.settings.user_set_bms_reset_duration_ms),
    InstantUintSetting("BYDAUTOCALDRIFT", 1, 20, &datalayer_extended.bydAtto3.auto_calibrate_soc_drift_percent),
    InstantUintSetting("BYDAUTOCALDRFT2", 1, 20, &datalayer_extended.bydAtto3_2.auto_calibrate_soc_drift_percent),
    // --- Instant ints, persisted ---
    // (name, min, max, storage pointer)
    InstantIntSetting("MINPERCENTAGE", -10, 50, &min_percentage_tenths),
    InstantIntSetting("MAXPERCENTAGE", 0, 200, &max_percentage_tenths),
    // --- Instant fixed-point, persisted ---
    // (name, min, max, scale factor, storage)
    InstantScaledSetting("MAXCHARGEAMP", 0.0f, 100.0f, 10.0f, &datalayer.battery.settings.max_user_set_charge_dA),
    InstantScaledSetting("MAXDISCHARGEAMP", 0.0f, 100.0f, 10.0f,
                          &datalayer.battery.settings.max_user_set_discharge_dA),
    InstantScaledSetting("TARGETCHVOLT", 0.0f, 1000.0f, 10.0f,
                          &datalayer.battery.settings.max_user_set_charge_voltage_dV),
    InstantScaledSetting("TARGETDISCHVOLT", 0.0f, 1000.0f, 10.0f,
                          &datalayer.battery.settings.max_user_set_discharge_voltage_dV),
    // --- Instant booleans, persisted ---
    // (name, storage pointer)
    InstantBoolSetting("USE_SCALED_SOC", &datalayer.battery.settings.soc_scaling_active),
    InstantBoolSetting("BYDAUTOCALEN", &datalayer_extended.bydAtto3.auto_calibrate_soc_enabled),
    InstantBoolSetting("BYDAUTOCALEN2", &datalayer_extended.bydAtto3_2.auto_calibrate_soc_enabled),
    // The single flag applies to both packs
    InstantBoolSetting("BYDKEEPISOOFF", &datalayer_extended.bydAtto3.keep_iso_disabled),
    // --- Volatile unsigned ints, not persisted ---
    // (name, min, max, storage pointer)
    VolatileUintSetting("TMP_CALTARGETSOC", 0, 100, &datalayer_extended.bydAtto3.calibrationTargetSOC),
    VolatileUintSetting("TMP_CALTARGETAH", 0, 1000, &datalayer_extended.bydAtto3.calibrationTargetAH),
    VolatileUintSetting("TMP_CALTARGETSOC2", 0, 100, &datalayer_extended.bydAtto3_2.calibrationTargetSOC),
    VolatileUintSetting("TMP_CALTARGETAH2", 0, 1000, &datalayer_extended.bydAtto3_2.calibrationTargetAH),
    // 'Hooked' settings have a custom setter/getter instead of storage
    VolatileUintHooked("TMP_FAKEBATTERYV", 0, 1000,
      [](uint32_t value) { if (battery != nullptr) battery->set_fake_voltage((float)value); },
      []() { return battery ? (uint32_t)battery->get_voltage() : 0; }),
    VolatileUintSetting("TMP_BALFLOATPOWER", 0, UINT32_MAX, &datalayer.battery.settings.balancing_float_power_W),
    VolatileUintSetting("TMP_BALMAXPACKV", 0, UINT32_MAX, &datalayer.battery.settings.balancing_max_pack_voltage_dV),
    VolatileUintSetting("TMP_BALMAXCELLV", 0, UINT32_MAX, &datalayer.battery.settings.balancing_max_cell_voltage_mV),
    VolatileUintSetting("TMP_BALMAXDEVCELLV", 0, UINT32_MAX,
                        &datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV),
    // --- Volatile booleans, not persisted ---
    // (name, storage pointer)
    VolatileBoolSetting("TMP_RECOVERYMODE", &datalayer.battery.settings.user_requests_forced_charging_recovery_mode),
    VolatileBoolSetting("TMP_BALANCE", &datalayer.battery.settings.user_requests_balancing),
    VolatileBoolSetting("TMP_CHARGERHVENABLED", &datalayer.charger.charger_HV_enabled),
    VolatileBoolSetting("TMP_CHARGERAUX12VENABLED", &datalayer.charger.charger_aux12V_enabled),
    // --- Volatile floats, not persisted ---
    // (name, min, max, apply hook, read hook)
    VolatileFloatSetting("TMP_CHARGERSETPOINTV", 0.0f, 1000.0f,
      [](float value) {
          if (value >= CHARGER_MIN_HV && value <= CHARGER_MAX_HV)
            datalayer.charger.charger_setpoint_HV_VDC = (float)value;
        },
      []() { return (float)datalayer.charger.charger_setpoint_HV_VDC; }),
    VolatileFloatSetting("TMP_CHARGERSETPOINTA", 0.0f, 100.0f,
      [](float value) {
          if ((value <= CHARGER_MAX_A) && (value <= datalayer.battery.settings.max_user_set_charge_dA) &&
              (value * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER))
            datalayer.charger.charger_setpoint_HV_IDC = (float)value;
        },
      []() { return (float)datalayer.charger.charger_setpoint_HV_IDC; }),
    VolatileFloatSetting("TMP_CHARGERENDA", 0.0f, 100.0f,
      [](float value) { datalayer.charger.charger_setpoint_HV_IDC_END = (float)value; },
      []() { return (float)datalayer.charger.charger_setpoint_HV_IDC_END; }),
    // --- Volatile scaled fixed-point, not persisted ---
    VolatileScaledSetting("TMP_BALTIME", 0.0f, (float)UINT32_MAX / 60000.0f, 60000.0f,
      [](float value) { datalayer.battery.settings.balancing_max_time_ms = (uint32_t)value; },
      []() { return (float)datalayer.battery.settings.balancing_max_time_ms; }),
  };

  // Compile-time validation of the settings table.
  static_assert(all_valid(settings), "invalid setting table entry!");
  // TODO: c++2c should allow more detailed static_assert output
  //[[maybe_unused]] constexpr const char* FIRST_INVALID_SETTING = first_invalid_name(settings);
  static_assert(names_unique(settings), "duplicate setting key in the settings table");

  return std::make_tuple(
      collect<PersistedUint>(settings),
      collect<PersistedInt>(settings),
      collect<PersistedBool>(settings),
      collect<PersistedString>(settings),
      collect<PersistedScaled>(settings),
      collect<InstantUint>(settings),
      collect<InstantInt>(settings),
      collect<InstantScaled>(settings),
      collect<InstantBool>(settings),
      collect<VolatileUint>(settings),
      collect<VolatileBool>(settings),
      collect<VolatileFloat>(settings),
      collect<VolatileScaled>(settings));
}

constexpr auto TABLES = build_tables();

// Split the tuple of tuples into individual tables with distinct types.
constexpr auto& PERSISTED_UINTS = std::get<0>(TABLES);
constexpr auto& PERSISTED_INTS = std::get<1>(TABLES);
constexpr auto& PERSISTED_BOOLS = std::get<2>(TABLES);
constexpr auto& PERSISTED_STRINGS = std::get<3>(TABLES);
constexpr auto& PERSISTED_SCALEDS = std::get<4>(TABLES);
constexpr auto& INSTANT_UINTS = std::get<5>(TABLES);
constexpr auto& INSTANT_INTS = std::get<6>(TABLES);
constexpr auto& INSTANT_SCALEDS = std::get<7>(TABLES);
constexpr auto& INSTANT_BOOLS = std::get<8>(TABLES);
constexpr auto& VOLATILE_UINTS = std::get<9>(TABLES);
constexpr auto& VOLATILE_BOOLS = std::get<10>(TABLES);
constexpr auto& VOLATILE_FLOATS = std::get<11>(TABLES);
constexpr auto& VOLATILE_SCALEDS = std::get<12>(TABLES);

// clang-format on

// ---------------------------------------------------------------------------

void populate_editable_settings() {
  // Some instant variables are stored differently in the NVM than they are in
  // the working variables. We can't directly use the settings framework for
  // these, as it is infeasible to convert them back to their original form for
  // editing (and then re-process them after).

  // We use parallel 'editable' variables for these settings, which we populate
  // from the live variables just before editing, and then apply back to the
  // live variables afterwards.

  max_percentage_tenths = (int32_t)datalayer.battery.settings.max_percentage / 10;
  min_percentage_tenths = (int32_t)datalayer.battery.settings.min_percentage / 10;
}

void apply_editable_settings() {
  datalayer.battery.settings.max_percentage = (uint16_t)max_percentage_tenths * 10;
  datalayer.battery.settings.min_percentage = (int16_t)min_percentage_tenths * 10;
  // The single flag applies to both packs.
  datalayer_extended.bydAtto3_2.keep_iso_disabled = datalayer_extended.bydAtto3.keep_iso_disabled;
}

// ---------------------------------------------------------------------------
// Loads stored settings from NVM at boot.
// ---------------------------------------------------------------------------

void load_stored_settings(BatteryEmulatorSettingsStore& settings) {
  // Ensure the default values are populated
  populate_editable_settings();

  // For each persisted setting, load the value from NVM and apply it to the
  // live value. The current live value is passed as the default, so if there is
  // no stored value, the live value is unchanged.
  load_all_settings(PERSISTED_UINTS, settings);
  load_all_settings(PERSISTED_INTS, settings);
  load_all_settings(PERSISTED_BOOLS, settings);
  load_all_settings(PERSISTED_STRINGS, settings);
  load_all_settings(PERSISTED_SCALEDS, settings);
  load_all_settings(INSTANT_UINTS, settings);
  load_all_settings(INSTANT_INTS, settings);
  load_all_settings(INSTANT_SCALEDS, settings);
  load_all_settings(INSTANT_BOOLS, settings);

  apply_editable_settings();

  republish_autodiscovery_if_changed(republish_ha_on_update);
}

void store_settings_from_live(BatteryEmulatorSettingsStore& settings) {
  populate_editable_settings();

  store_all_settings(INSTANT_UINTS, settings);
  store_all_settings(INSTANT_INTS, settings);
  store_all_settings(INSTANT_SCALEDS, settings);
  store_all_settings(INSTANT_BOOLS, settings);
}

// ---------------------------------------------------------------------------
// Settings (GET/POST to /api/internal/settings)
// ---------------------------------------------------------------------------

// Validate or apply the given settings:
// - `doc` contains the settings to validate or apply.
// - `errors` is a JSON object to which any validation errors will be added.
// - `settings` is the settings store to which persisted settings will be saved.
// - `save` is `false` for validation only, `true` to save the settings.
// - `reboot_required_saved` is set to true if any persisted setting was changed.
void apply_setting_updates(const JsonDocument& doc, JsonDocument& errors, BatteryEmulatorSettingsStore& settings,
                           bool save, bool& reboot_required_saved) {
  populate_editable_settings();

  process_all_settings(PERSISTED_UINTS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(PERSISTED_INTS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(PERSISTED_BOOLS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(PERSISTED_STRINGS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(PERSISTED_SCALEDS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(INSTANT_UINTS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(INSTANT_INTS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(INSTANT_SCALEDS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(INSTANT_BOOLS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(VOLATILE_UINTS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(VOLATILE_BOOLS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(VOLATILE_FLOATS, doc, errors, settings, save, reboot_required_saved);
  process_all_settings(VOLATILE_SCALEDS, doc, errors, settings, save, reboot_required_saved);

  apply_editable_settings();
}

void build_settings_json(JsonDocument& doc, BatteryEmulatorSettingsStore& settings) {
  JsonArray bats = doc["batteries"].to<JsonArray>();
  for (int i = 0; i < (int)BatteryType::Highest; i++) {
    bats[i] = name_for_battery_type((BatteryType)i);
  }
  JsonArray invs = doc["inverters"].to<JsonArray>();
  for (int i = 0; i < (int)InverterProtocolType::Highest; i++) {
    invs[i] = name_for_inverter_type((InverterProtocolType)i);
  }

  JsonObject sets = doc["settings"].to<JsonObject>();

  populate_editable_settings();

  // PERSISTED_* and INSTANT_* show the stored value when present (so a change
  // that still awaits its reboot is visible), else the live storage; VOLATILE_*
  // use read hooks over live state.
  emit_all_settings(PERSISTED_UINTS, sets, settings);
  emit_all_settings(PERSISTED_INTS, sets, settings);
  emit_all_settings(PERSISTED_BOOLS, sets, settings);
  emit_all_settings(PERSISTED_STRINGS, sets, settings);
  emit_all_settings(PERSISTED_SCALEDS, sets, settings);
  emit_all_settings(INSTANT_UINTS, sets, settings);
  emit_all_settings(INSTANT_INTS, sets, settings);
  emit_all_settings(INSTANT_SCALEDS, sets, settings);
  emit_all_settings(INSTANT_BOOLS, sets, settings);
  emit_all_settings(VOLATILE_UINTS, sets, settings);
  emit_all_settings(VOLATILE_BOOLS, sets, settings);
  emit_all_settings(VOLATILE_FLOATS, sets, settings);
  emit_all_settings(VOLATILE_SCALEDS, sets, settings);

  doc["reboot_required"] = settingsUpdated;
}
