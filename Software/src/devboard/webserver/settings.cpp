#include "settings.h"
#include "settings_handlers.h"
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

// ---------------------------------------------------------------------------
// Settings tables. Each setting appears exactly once, in the table whose
// persistence/type semantics it needs. These are loaded in comm_nvm.cpp and
// saved in the GET/POST handlers and store_settings().
// ---------------------------------------------------------------------------

// Settings are distinguished into three classes:
//
// PERSISTED settings are persisted to NVM and require a reboot to take effect.
// They are loaded at boot by init_stored_settings().
//
// INSTANT settings are persisted to NVM but also applied to the live state
// immediately. They are loaded at boot by init_stored_settings().
//
// VOLATILE settings are not persisted to NVM, they are only applied to the live
// state.

// clang-format off

// Unsigned ints and enums, which are persisted to flash, and require a reboot
// to take effect.
const PersistedUint PERSISTED_UINTS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - min (inclusive)
  //  - max (inclusive)
  //  - storage pointer
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
};

// Signed numeric settings. These are persisted to flash, and require a reboot to take effect.
const PersistedInt PERSISTED_INTS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - min (inclusive)
  //  - max (inclusive)
  //  - storage pointer
  IntSetting("CPUTEMPOFFSET", -100, 100, &datalayer.system.info.CPU_temperature_calibration_offset),
};

// Boolean settings. These are persisted to flash, and require a reboot to take effect.
const PersistedBool PERSISTED_BOOLS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - storage pointer
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
};

// String settings. These are persisted to flash, and require a reboot to take effect.
const PersistedString PERSISTED_STRINGS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - max length (excluding any null terminator)
  //  - storage pointer
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
};

// Scaled fixed-point settings. These are persisted to flash, and require a reboot to take effect.
// Edited/validated as float, stored scaled (uint16_t).
const PersistedScaled PERSISTED_SCALEDS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - min (inclusive)
  //  - max (inclusive)
  //  - scale factor (float), multiplied before storage (and divided after retrieval)
  //  - storage pointer
  ScaledSetting("BATTPVMAX", 0.0f, 1000.0f, 10.0f, &user_selected_max_pack_voltage_dV),
  ScaledSetting("BATTPVMIN", 0.0f, 1000.0f, 10.0f, &user_selected_min_pack_voltage_dV),
};

// Instant unsigned integer settings. These are persisted to flash, but take
// effect immediately without a reboot.
const InstantUint INSTANT_UINTS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - min (inclusive)
  //  - max (inclusive)
  //  - storage pointer
  InstantUintSetting("BATTERY_WH_MAX", 1, 400000, &datalayer.battery.info.total_capacity_Wh),
  InstantUintSetting("BMSRESETDUR", 0, 60000, &datalayer.battery.settings.user_set_bms_reset_duration_ms),
  InstantUintSetting("BYDAUTOCALDRIFT", 1, 20, &datalayer_extended.bydAtto3.auto_calibrate_soc_drift_percent),
  InstantUintSetting("BYDAUTOCALDRFT2", 1, 20, &datalayer_extended.bydAtto3_2.auto_calibrate_soc_drift_percent),
};

const InstantInt INSTANT_INTS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - min (inclusive)
  //  - max (inclusive)
  //  - storage pointer
  InstantIntSetting("MINPERCENTAGE", -10, 50, &min_percentage_tenths),
  InstantIntSetting("MAXPERCENTAGE", 0, 200, &max_percentage_tenths),
};

// Instant fixed-point settings. These are persisted to flash, but take effect
// immediately without a reboot. 
const InstantScaled INSTANT_SCALEDS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - min (inclusive)
  //  - max (inclusive)
  //  - scale factor (float), multiplied before storage (and divided after retrieval)
  //  - storage pointer
  InstantScaledSetting("MAXCHARGEAMP", 0.0f, 100.0f, 10.0f, &datalayer.battery.settings.max_user_set_charge_dA),
  InstantScaledSetting("MAXDISCHARGEAMP", 0.0f, 100.0f, 10.0f,
                        &datalayer.battery.settings.max_user_set_discharge_dA),
  InstantScaledSetting("TARGETCHVOLT", 0.0f, 1000.0f, 10.0f,
                        &datalayer.battery.settings.max_user_set_charge_voltage_dV),
  InstantScaledSetting("TARGETDISCHVOLT", 0.0f, 1000.0f, 10.0f,
                        &datalayer.battery.settings.max_user_set_discharge_voltage_dV),
};

// Instant boolean settings. These are persisted to flash, but take effect
// immediately without a reboot.
const InstantBool INSTANT_BOOLS[] = {
  // Arguments:
  //  - name (must be <= 15 chars for NVS)
  //  - storage pointer
  InstantBoolSetting("USE_SCALED_SOC", &datalayer.battery.settings.soc_scaling_active),
  InstantBoolSetting("BYDAUTOCALEN", &datalayer_extended.bydAtto3.auto_calibrate_soc_enabled),
  InstantBoolSetting("BYDAUTOCALEN2", &datalayer_extended.bydAtto3_2.auto_calibrate_soc_enabled),
};

// Volatile unsigned integer settings. These are not persisted to flash, and
// take effect immediately without a reboot.
const VolatileUint VOLATILE_UINTS[] = {
  // Arguments:
  //  - name
  //  - min (inclusive)
  //  - max (inclusive)
  //  - apply function (web -> runtime)
  //  - read function (runtime -> web)
  VolatileUintSetting("TMP_CALTARGETSOC", 0, 100,
    [](uint32_t value) { datalayer_extended.bydAtto3.calibrationTargetSOC = (uint16_t)value; },
    []() { return (uint32_t)datalayer_extended.bydAtto3.calibrationTargetSOC; }),
  VolatileUintSetting("TMP_CALTARGETAH", 0, 1000,
    [](uint32_t value) { datalayer_extended.bydAtto3.calibrationTargetAH = (uint16_t)value; },
    []() { return (uint32_t)datalayer_extended.bydAtto3.calibrationTargetAH; }),
  VolatileUintSetting("TMP_CALTARGETSOC2", 0, 100,
    [](uint32_t value) { datalayer_extended.bydAtto3_2.calibrationTargetSOC = (uint16_t)value; },
    []() { return (uint32_t)datalayer_extended.bydAtto3_2.calibrationTargetSOC; }),
  VolatileUintSetting("TMP_CALTARGETAH2", 0, 1000,
    [](uint32_t value) { datalayer_extended.bydAtto3_2.calibrationTargetAH = (uint16_t)value; },
    []() { return (uint32_t)datalayer_extended.bydAtto3_2.calibrationTargetAH; }),
  VolatileUintSetting("TMP_FAKEBATTERYV", 0, 1000,
    [](uint32_t value) { if (battery != nullptr) battery->set_fake_voltage((float)value); },
    []() { return battery ? (uint32_t)battery->get_voltage() : 0; }),
  VolatileUintSetting("TMP_BALFLOATPOWER", 0, UINT32_MAX,
    [](uint32_t value) { datalayer.battery.settings.balancing_float_power_W = (uint16_t)value; },
    []() { return (uint32_t)datalayer.battery.settings.balancing_float_power_W; }),
  VolatileUintSetting("TMP_BALMAXPACKV", 0, UINT32_MAX,
    [](uint32_t value) { datalayer.battery.settings.balancing_max_pack_voltage_dV = (uint16_t)value; },
    []() { return (uint32_t)datalayer.battery.settings.balancing_max_pack_voltage_dV; }),
  VolatileUintSetting("TMP_BALMAXCELLV", 0, UINT32_MAX,
    [](uint32_t value) { datalayer.battery.settings.balancing_max_cell_voltage_mV = (uint16_t)value; },
    []() { return (uint32_t)datalayer.battery.settings.balancing_max_cell_voltage_mV; }),
  VolatileUintSetting("TMP_BALMAXDEVCELLV", 0, UINT32_MAX,
    [](uint32_t value) { datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV = (uint16_t)value; },
    []() { return (uint32_t)datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV; }),
};

// Volatile boolean settings. These are not persisted to flash, and take effect
// immediately without a reboot.
const VolatileBool VOLATILE_BOOLS[] = {
  // Arguments:
  //  - name
  //  - apply function (web -> runtime)
  //  - read function (runtime -> web)
  VolatileBoolSetting("TMP_RECOVERYMODE",
    [](bool value) { datalayer.battery.settings.user_requests_forced_charging_recovery_mode = value; },
    []() { return datalayer.battery.settings.user_requests_forced_charging_recovery_mode; }),
  VolatileBoolSetting("TMP_BALANCE",
    [](bool value) { datalayer.battery.settings.user_requests_balancing = value; },
    []() { return datalayer.battery.settings.user_requests_balancing; }),
  VolatileBoolSetting("TMP_CHARGERHVENABLED",
    [](bool value) { datalayer.charger.charger_HV_enabled = value; },
    []() { return datalayer.charger.charger_HV_enabled; }),
  VolatileBoolSetting("TMP_CHARGERAUX12VENABLED",
    [](bool value) { datalayer.charger.charger_aux12V_enabled = value; },
    []() { return datalayer.charger.charger_aux12V_enabled; }),
  // BYD Atto 3 isolation monitor: keep it disabled across BMS restarts. The
  // value is persisted in comm_nvm.cpp (BYDKEEPISOOFF), this table entry only
  // exposes it to the web API; it applies to both packs at once.
  VolatileBoolSetting("BYDKEEPISOOFF",
    [](bool value) {
      datalayer_extended.bydAtto3.keep_iso_disabled = value;
      datalayer_extended.bydAtto3_2.keep_iso_disabled = value;
    },
    []() { return datalayer_extended.bydAtto3.keep_iso_disabled; }),
};

// Volatile float settings. These are not persisted to flash, and take effect
// immediately without a reboot.
const VolatileFloat VOLATILE_FLOATS[] = {
  // Arguments:
  //  - name
  //  - min (inclusive)
  //  - max (inclusive)
  //  - apply function (web -> runtime)
  //  - read function (runtime -> web)
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
};

// Volatile scaled fixed-point settings. These are not persisted to flash, and
// take effect immediately without a reboot.
const VolatileScaled VOLATILE_SCALEDS[] = {
  // Arguments:
  //  - name
  //  - min (inclusive)
  //  - max (inclusive)
  //  - scale factor (float), multiplied before storage (and divided after retrieval)
  //  - apply function (web -> runtime)
  //  - read function (runtime -> web)
  VolatileScaledSetting("TMP_BALTIME", 0.0f, (float)UINT32_MAX / 60000.0f, 60000.0f,
    [](float value) { datalayer.battery.settings.balancing_max_time_ms = (uint32_t)value; },
    []() { return (float)datalayer.battery.settings.balancing_max_time_ms; }),
};

// clang-format on

// ---------------------------------------------------------------------------

void populate_editable_settings() {
  // Some instant variables are stored differently in the NVM than they are in
  // the working variables. We can't directly use the settings framework for
  // these, since they need processing before editing (to convert them to their
  // editable representation) and after editing (to convert them back to their
  // working representation).

  // We use parallel 'editable' variables for these settings, which we populate
  // from the live variables just before editing, and then apply back to the
  // live variables afterwards.

  max_percentage_tenths = (int32_t)datalayer.battery.settings.max_percentage / 10;
  min_percentage_tenths = (int32_t)datalayer.battery.settings.min_percentage / 10;
}

void apply_editable_settings() {
  datalayer.battery.settings.max_percentage = (uint16_t)max_percentage_tenths * 10;
  datalayer.battery.settings.min_percentage = (int16_t)min_percentage_tenths * 10;
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
