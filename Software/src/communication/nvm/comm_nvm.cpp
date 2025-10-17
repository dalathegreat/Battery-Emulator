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
#include "../equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../precharge_control/precharge_control.h"

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

  //settings.clear();  // If this clear function is executed, no settings will be read from storage. For dev

  esp32hal->set_default_configuration_values();

  ssid = settings.getString("SSID").c_str();
  password = settings.getString("PASSWORD").c_str();

  temp = settings.getUInt("BATTERY_WH_MAX", false);
  if (temp != 0) {
    datalayer.battery.info.total_capacity_Wh = temp;
  }
  temp = settings.getUInt("MAXPERCENTAGE", false);
  if (temp != 0) {
    datalayer.battery.settings.max_percentage = temp * 10;  // Multiply by 10 for backwards compatibility
  }
  temp = settings.getUInt("MINPERCENTAGE", false);
  if (temp < 499) {
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
  user_selected_pylon_send = settings.getUInt("PYLONSEND", 0);
  user_selected_pylon_30koffset = settings.getBool("PYLONOFFSET", false);
  user_selected_pylon_invert_byteorder = settings.getBool("PYLONORDER", false);
  user_selected_inverter_cells = settings.getUInt("INVCELLS", 0);
  user_selected_inverter_modules = settings.getUInt("INVMODULES", 0);
  user_selected_inverter_cells_per_module = settings.getUInt("INVCELLSPER", 0);
  user_selected_inverter_voltage_level = settings.getUInt("INVVLEVEL", 0);
  user_selected_inverter_ah_capacity = settings.getUInt("INVAHCAPACITY", 0);
  user_selected_inverter_battery_type = settings.getUInt("INVBTYPE", 0);
  user_selected_inverter_ignore_contactors = settings.getBool("INVICNT", false);
  user_selected_inverter_deye_workaround = settings.getBool("DEYEBYD", false);
  user_selected_can_addon_crystal_frequency_mhz = settings.getUInt("CANFREQ", 8);
  user_selected_canfd_addon_crystal_frequency_mhz = settings.getUInt("CANFDFREQ", 40);
  user_selected_LEAF_interlock_mandatory = settings.getBool("INTERLOCKREQ", false);
  user_selected_use_estimated_SOC = settings.getBool("SOCESTIMATED", false);
  user_selected_tesla_digital_HVIL = settings.getBool("DIGITALHVIL", false);
  user_selected_tesla_GTW_country = settings.getUInt("GTWCOUNTRY", 0);
  user_selected_tesla_GTW_rightHandDrive = settings.getBool("GTWRHD", false);
  user_selected_tesla_GTW_mapRegion = settings.getUInt("GTWMAPREG", 0);
  user_selected_tesla_GTW_chassisType = settings.getUInt("GTWCHASSIS", 0);
  user_selected_tesla_GTW_packEnergy = settings.getUInt("GTWPACK", 0);

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
      case comm_interface::RS485:
      case comm_interface::Modbus:
      case comm_interface::Highest:
        return CAN_Interface::NO_CAN_INTERFACE;
    }

    return CAN_Interface::CAN_NATIVE;  //Failed to determine, return CAN native
  };

  can_config.battery = readIf("BATTCOMM");
  can_config.battery_double = readIf("BATT2COMM");
  can_config.inverter = readIf("INVCOMM");
  can_config.charger = readIf("CHGCOMM");
  can_config.shunt = readIf("SHUNTCOMM");

  equipment_stop_behavior = (STOP_BUTTON_BEHAVIOR)settings.getUInt("EQSTOP", (int)STOP_BUTTON_BEHAVIOR::NOT_CONNECTED);
  user_selected_second_battery = settings.getBool("DBLBTR", false);
  contactor_control_enabled = settings.getBool("CNTCTRL", false);
  precharge_time_ms = settings.getUInt("PRECHGMS", 100);
  contactor_control_enabled_double_battery = settings.getBool("CNTCTRLDBL", false);
  pwm_contactor_control = settings.getBool("PWMCNTCTRL", false);
  pwm_frequency = settings.getUInt("PWMFREQ", 20000);
  pwm_hold_duty = settings.getUInt("PWMHOLD", 250);
  periodic_bms_reset = settings.getBool("PERBMSRESET", false);
  remote_bms_reset = settings.getBool("REMBMSRESET", false);
  use_canfd_as_can = settings.getBool("CANFDASCAN", false);

  precharge_control_enabled = settings.getBool("EXTPRECHARGE", false);
  precharge_inverter_normally_open_contactor = settings.getBool("NOINVDISC", false);
  precharge_max_precharge_time_before_fault = settings.getUInt("MAXPRETIME", 15000);

  datalayer.system.info.performance_measurement_active = settings.getBool("PERFPROFILE", false);
  datalayer.system.info.CAN_usb_logging_active = settings.getBool("CANLOGUSB", false);
  datalayer.system.info.usb_logging_active = settings.getBool("USBENABLED", false);
  datalayer.system.info.web_logging_active = settings.getBool("WEBENABLED", false);
  datalayer.system.info.CAN_SD_logging_active = settings.getBool("CANLOGSD", false);
  datalayer.system.info.SD_logging_active = settings.getBool("SDLOGENABLED", false);
  datalayer.battery.status.led_mode = (led_mode_enum)settings.getUInt("LEDMODE", false);

  //Some early integrations need manually set allowed charge/discharge power
  datalayer.battery.status.override_charge_power_W = settings.getUInt("CHGPOWER", 1000);
  datalayer.battery.status.override_discharge_power_W = settings.getUInt("DCHGPOWER", 1000);

  // WIFI AP is enabled by default unless disabled in the settings
  wifiap_enabled = settings.getBool("WIFIAPENABLED", true);
  wifi_channel = settings.getUInt("WIFICHANNEL", 0);
  ssidAP = settings.getString("APNAME", "BatteryEmulator").c_str();
  passwordAP = settings.getString("APPASSWORD", "123456789").c_str();
  mqtt_enabled = settings.getBool("MQTTENABLED", false);
  mqtt_timeout_ms = settings.getUInt("MQTTTIMEOUT", 2000);
  ha_autodiscovery_enabled = settings.getBool("HADISC", false);
  mqtt_transmit_all_cellvoltages = settings.getBool("MQTTCELLV", false);
  custom_hostname = settings.getString("HOSTNAME").c_str();

  static_IP_enabled = settings.getBool("STATICIP", false);
  static_local_IP1 = settings.getUInt("LOCALIP1", 192);
  static_local_IP2 = settings.getUInt("LOCALIP2", 168);
  static_local_IP3 = settings.getUInt("LOCALIP3", 10);
  static_local_IP4 = settings.getUInt("LOCALIP4", 150);
  static_gateway1 = settings.getUInt("GATEWAY1", 192);
  static_gateway2 = settings.getUInt("GATEWAY2", 168);
  static_gateway3 = settings.getUInt("GATEWAY3", 10);
  static_gateway4 = settings.getUInt("GATEWAY4", 1);
  static_subnet1 = settings.getUInt("SUBNET1", 255);
  static_subnet2 = settings.getUInt("SUBNET2", 255);
  static_subnet3 = settings.getUInt("SUBNET3", 255);
  static_subnet4 = settings.getUInt("SUBNET4", 0);

  mqtt_server = settings.getString("MQTTSERVER").c_str();
  mqtt_port = settings.getUInt("MQTTPORT", 0);
  mqtt_user = settings.getString("MQTTUSER").c_str();
  mqtt_password = settings.getString("MQTTPASSWORD").c_str();

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
