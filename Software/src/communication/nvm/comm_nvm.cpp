#include "comm_nvm.h"
#include "../../include.h"
#include "../contactorcontrol/comm_contactorcontrol.h"

// Parameters
Preferences settings;  // Store user settings

// Initialization functions

void init_stored_settings() {
  static uint32_t temp = 0;
  //  ATTENTION ! The maximum length for settings keys is 15 characters
  settings.begin("batterySettings", false);

  // Always get the equipment stop status
  datalayer.system.settings.equipment_stop_active = settings.getBool("EQUIPMENT_STOP", false);
  if (datalayer.system.settings.equipment_stop_active) {
    set_event(EVENT_EQUIPMENT_STOP, 1);
  }

#ifndef LOAD_SAVED_SETTINGS_ON_BOOT
  settings.clear();  // If this clear function is executed, no settings will be read from storage

  //always save the equipment stop status
  settings.putBool("EQUIPMENT_STOP", datalayer.system.settings.equipment_stop_active);
#endif  // LOAD_SAVED_SETTINGS_ON_BOOT

#ifdef WIFI
  char tempSSIDstring[63];  // Allocate buffer with sufficient size
  size_t lengthSSID = settings.getString("SSID", tempSSIDstring, sizeof(tempSSIDstring));
  if (lengthSSID > 0) {  // Successfully read the string from memory. Set it to SSID!
    ssid = tempSSIDstring;
  } else {  // Reading from settings failed. Do nothing with SSID. Raise event?
  }
  char tempPasswordString[63];  // Allocate buffer with sufficient size
  size_t lengthPassword = settings.getString("PASSWORD", tempPasswordString, sizeof(tempPasswordString));
  if (lengthPassword > 7) {  // Successfully read the string from memory. Set it to password!
    password = tempPasswordString;
  } else {  // Reading from settings failed. Do nothing with SSID. Raise event?
  }
#endif  // WIFI

  temp = settings.getUInt("BATTERY_WH_MAX", false);
  if (temp != 0) {
    datalayer.battery.info.total_capacity_Wh = temp;
  }
  temp = settings.getUInt("MAXPERCENTAGE", false);
  if (temp != 0) {
    datalayer.battery.settings.max_percentage = temp * 10;  // Multiply by 10 for backwards compatibility
  }
  temp = settings.getUInt("MINPERCENTAGE", false);
  if (temp != 0) {
    datalayer.battery.settings.min_percentage = temp * 10;  // Multiply by 10 for backwards compatibility
  }
  temp = settings.getUInt("MAXCHARGEAMP", false);
  if (temp != 0) {
    datalayer.battery.settings.max_user_set_charge_dA = temp;
  }
  temp = settings.getUInt("MAXDISCHARGEAMP", false);
  if (temp != 0) {
    datalayer.battery.settings.max_user_set_discharge_dA = temp;
  }
  datalayer.battery.settings.soc_scaling_active = settings.getBool("USE_SCALED_SOC", false);
  temp = settings.getUInt("TARGETCHVOLT", false);
  if (temp != 0) {
    datalayer.battery.settings.max_user_set_charge_voltage_dV = temp;
  }
  temp = settings.getUInt("TARGETDISCHVOLT", false);
  if (temp != 0) {
    datalayer.battery.settings.max_user_set_discharge_voltage_dV = temp;
  }
  datalayer.battery.settings.user_set_voltage_limits_active = settings.getBool("USEVOLTLIMITS", false);

#ifdef COMMON_IMAGE
  user_selected_battery_type = (BatteryType)settings.getUInt("BATTTYPE", (int)BatteryType::None);
  user_selected_inverter_protocol = (InverterProtocolType)settings.getUInt("INVTYPE", (int)InverterProtocolType::None);
  user_selected_charger_type = (ChargerType)settings.getUInt("CHGTYPE", (int)ChargerType::None);
  user_selected_second_battery = settings.getBool("DBLBTR", false);
  contactor_control_enabled = settings.getBool("CNTCTRL", false);
  contactor_control_enabled_double_battery = settings.getBool("CNTCTRLDBL", false);
  pwm_contactor_control = settings.getBool("PWMCNTCTRL", false);
  periodic_bms_reset = settings.getBool("PERBMSRESET", false);
  remote_bms_reset = settings.getBool("REMBMSRESET", false);
#endif

  settings.end();
}

void store_settings_equipment_stop() {
  settings.begin("batterySettings", false);
  settings.putBool("EQUIPMENT_STOP", datalayer.system.settings.equipment_stop_active);
  settings.end();
}

void store_settings() {
  //  ATTENTION ! The maximum length for settings keys is 15 characters
  if (!settings.begin("batterySettings", false)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 0);
    return;
  }

#ifdef WIFI
  if (!settings.putString("SSID", String(ssid.c_str()))) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 1);
  }
  if (!settings.putString("PASSWORD", String(password.c_str()))) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 2);
  }
#endif

  if (!settings.putUInt("BATTERY_WH_MAX", datalayer.battery.info.total_capacity_Wh)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 3);
  }
  if (!settings.putBool("USE_SCALED_SOC", datalayer.battery.settings.soc_scaling_active)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 4);
  }
  if (!settings.putUInt("MAXPERCENTAGE", datalayer.battery.settings.max_percentage / 10)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 5);
  }
  if (!settings.putUInt("MINPERCENTAGE", datalayer.battery.settings.min_percentage / 10)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 6);
  }
  if (!settings.putUInt("MAXCHARGEAMP", datalayer.battery.settings.max_user_set_charge_dA)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 7);
  }
  if (!settings.putUInt("MAXDISCHARGEAMP", datalayer.battery.settings.max_user_set_discharge_dA)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 8);
  }
  if (!settings.putBool("USEVOLTLIMITS", datalayer.battery.settings.user_set_voltage_limits_active)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 9);
  }
  if (!settings.putUInt("TARGETCHVOLT", datalayer.battery.settings.max_user_set_charge_voltage_dV)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 10);
  }
  if (!settings.putUInt("TARGETDISCHVOLT", datalayer.battery.settings.max_user_set_discharge_voltage_dV)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 11);
  }

  settings.end();  // Close preferences handle
}
