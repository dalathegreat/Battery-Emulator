#include "comm_nvm.h"
#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../battery/Shunt.h"
#include "../../charger/CanCharger.h"
#include "../../communication/can/comm_can.h"
#include "../../devboard/mqtt/mqtt.h"
#include "../../devboard/wifi/wifi.h"
#include "../../inverter/INVERTERS.h"
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
    DEBUG_PRINTF("Equipment stop status set in boot.");
    set_event(EVENT_EQUIPMENT_STOP, 1);
  }

#ifndef LOAD_SAVED_SETTINGS_ON_BOOT
  settings.clear();  // If this clear function is executed, no settings will be read from storage

  //always save the equipment stop status
  settings.putBool("EQUIPMENT_STOP", datalayer.system.settings.equipment_stop_active);
#endif  // LOAD_SAVED_SETTINGS_ON_BOOT

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
  temp = settings.getUInt("SOFAR_ID", false);
  if (temp < 16) {
    datalayer.battery.settings.sofar_user_specified_battery_id = temp;
  }
  temp = settings.getUInt("BMSRESETDUR", false);
  if (temp != 0) {
    datalayer.battery.settings.user_set_bms_reset_duration_ms = temp;
  }

#ifdef COMMON_IMAGE
  user_selected_battery_type = (BatteryType)settings.getUInt("BATTTYPE", (int)BatteryType::None);
  user_selected_battery_chemistry =
      (battery_chemistry_enum)settings.getUInt("BATTCHEM", (int)battery_chemistry_enum::NCA);
  user_selected_inverter_protocol = (InverterProtocolType)settings.getUInt("INVTYPE", (int)InverterProtocolType::None);
  user_selected_charger_type = (ChargerType)settings.getUInt("CHGTYPE", (int)ChargerType::None);
  user_selected_shunt_type = (ShuntType)settings.getUInt("SHUNTTYPE", (int)ShuntType::None);
  user_selected_max_pack_voltage_dV = settings.getUInt("BATTPVMAX", 0);
  user_selected_min_pack_voltage_dV = settings.getUInt("BATTPVMIN", 0);
  user_selected_max_cell_voltage_mV = settings.getUInt("BATTCVMAX", 0);
  user_selected_min_cell_voltage_mV = settings.getUInt("BATTCVMIN", 0);
  user_selected_inverter_cells = settings.getUInt("INVCELLS", 0);
  user_selected_inverter_modules = settings.getUInt("INVMODULES", 0);
  user_selected_inverter_cells_per_module = settings.getUInt("INVCELLSPER", 0);
  user_selected_inverter_voltage_level = settings.getUInt("INVVLEVEL", 0);
  user_selected_inverter_ah_capacity = settings.getUInt("INVAHCAPACITY", 0);
  user_selected_inverter_battery_type = settings.getUInt("INVBTYPE", 0);
  user_selected_inverter_ignore_contactors = settings.getBool("INVICNT", false);

  auto readIf = [](const char* settingName) {
    auto batt1If = (comm_interface)settings.getUInt(settingName, (int)comm_interface::CanNative);
    switch (batt1If) {
      case comm_interface::CanNative:
        return CAN_Interface::CAN_NATIVE;
      case comm_interface::CanFdNative:
        return CAN_Interface::CANFD_NATIVE;
      case comm_interface::CanAddonMcp2515:
        return CAN_Interface::CAN_ADDON_MCP2515;
      case comm_interface::CanFdAddonMcp2518:
        return CAN_Interface::CANFD_ADDON_MCP2518;
    }

    return CAN_Interface::CAN_NATIVE;
  };

  can_config.battery = readIf("BATTCOMM");
  can_config.battery_double = readIf("BATT2COMM");
  can_config.inverter = readIf("INVCOMM");
  can_config.charger = readIf("CHGCOMM");
  can_config.shunt = readIf("SHUNTCOMM");

  equipment_stop_behavior = (STOP_BUTTON_BEHAVIOR)settings.getUInt("EQSTOP", (int)STOP_BUTTON_BEHAVIOR::NOT_CONNECTED);
  user_selected_second_battery = settings.getBool("DBLBTR", false);
  contactor_control_enabled = settings.getBool("CNTCTRL", false);
  contactor_control_enabled_double_battery = settings.getBool("CNTCTRLDBL", false);
  pwm_contactor_control = settings.getBool("PWMCNTCTRL", false);
  periodic_bms_reset = settings.getBool("PERBMSRESET", false);
  remote_bms_reset = settings.getBool("REMBMSRESET", false);
  use_canfd_as_can = settings.getBool("CANFDASCAN", false);

  datalayer.system.info.usb_logging_active = settings.getBool("USBENABLED", false);
  datalayer.system.info.web_logging_active = settings.getBool("WEBENABLED", false);

  // WIFI AP is enabled by default unless disabled in the settings
  wifiap_enabled = settings.getBool("WIFIAPENABLED", true);
  passwordAP = settings.getString("APPASSWORD", "123456789").c_str();
  mqtt_enabled = settings.getBool("MQTTENABLED", false);
  ha_autodiscovery_enabled = settings.getBool("HADISC", false);

  custom_hostname = settings.getString("HOSTNAME").c_str();

  mqtt_server = settings.getString("MQTTSERVER").c_str();
  mqtt_port = settings.getUInt("MQTTPORT", 0);
  mqtt_user = settings.getString("MQTTUSER").c_str();
  mqtt_password = settings.getString("MQTTPASSWORD").c_str();

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

  if (!settings.putString("SSID", String(ssid.c_str()))) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 1);
  }
  if (!settings.putString("PASSWORD", String(password.c_str()))) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 2);
  }

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
  if (!settings.putUInt("BMSRESETDUR", datalayer.battery.settings.user_set_bms_reset_duration_ms)) {
    set_event(EVENT_PERSISTENT_SAVE_INFO, 13);
  }

  settings.end();  // Close preferences handle
}
