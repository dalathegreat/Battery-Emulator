#include "comm_nvm.h"
#include <esp_phy_init.h>  // esp_phy_erase_cal_data_in_nvs()
#include <cstdlib>         // strtof
#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../charger/CanCharger.h"
#include "../../communication/can/comm_can.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/network/hostname.h"
#include "../../devboard/utils/logging.h"
#include "../../devboard/webserver/settings.h"
#include "../../devboard/webserver/webserver_new.h"
#include "../../devboard/wifi/wifi.h"
#include "../../inverter/INVERTERS.h"
#include "../../shunt/Shunt.h"
#include "../contactorcontrol/comm_contactorcontrol.h"
#include "../equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../precharge_control/precharge_control.h"

// Keys holding the static IP configuration, as dotted-quad strings.
static const char* const STATIC_IP_KEYS[] = {"LOCALIP", "GATEWAY", "SUBNET", "DNS"};

// Releases <= 10.x stored the static IP configuration as twelve separate octet keys. Fold them into the
// dotted-quad string keys once, then drop the old keys.
// Added in 2026.07. Can be removed couple of releases from now (suggested after 2027.01)
static void migrate_static_ip_settings(BatteryEmulatorSettingsStore& settings) {
  static const char* const legacy_keys[] = {"LOCALIP1", "LOCALIP2", "LOCALIP3", "LOCALIP4", "GATEWAY1", "GATEWAY2",
                                            "GATEWAY3", "GATEWAY4", "SUBNET1",  "SUBNET2",  "SUBNET3",  "SUBNET4"};

  if (settings.settingExists("LOCALIP1")) {
    for (int i = 0; i < 3; i++) {  // LOCALIP, GATEWAY, SUBNET - DNS did not exist before, leave it empty
      String value;
      for (int octet = 0; octet < 4; octet++) {
        value += settings.getUInt(legacy_keys[i * 4 + octet], 0);
        if (octet < 3) {
          value += '.';
        }
      }
      settings.saveString(STATIC_IP_KEYS[i], value.c_str());
    }
    DEBUG_PRINTLN("Static IPv4 settings migrated successfully");
  }

  for (auto key : legacy_keys) {
    settings.removeKey(key);
  }
}

// Maps a user-selected communication interface to the CAN interface used by
// the CAN layer. RS485/Modbus mean "no CAN on this interface".
static CAN_Interface remap_comm_interface(comm_interface interface) {
  switch (interface) {
    case comm_interface::CanNative:
      return CAN_Interface::CAN_NATIVE;
    case comm_interface::CanFdNative:
      return CAN_Interface::CANFD_NATIVE;
    case comm_interface::CanAddonMcp2515:
      return CAN_Interface::CAN_ADDON_MCP2515;
    case comm_interface::CanFdAddonMcp2518:
      return CAN_Interface::CANFD_ADDON_MCP2518;
    case comm_interface::CanFdAddonMcp2518_2:
      return CAN_Interface::CANFD_ADDON_MCP2518_2;
    case comm_interface::RS485:
    case comm_interface::Modbus:
    case comm_interface::Highest:
      return CAN_Interface::NO_CAN_INTERFACE;
  }

  return CAN_Interface::CAN_NATIVE;  //Failed to determine, return CAN native
}

// Initialization functions

void init_stored_settings() {
  BatteryEmulatorSettingsStore settings(false);
  //  ATTENTION ! The maximum length for settings keys is 15 characters

  // Always get the equipment stop status
  datalayer.system.info.equipment_stop_active = settings.getBool("EQUIPMENT_STOP", false);
  if (datalayer.system.info.equipment_stop_active) {
    DEBUG_PRINTF("Equipment stop status set in boot.");
    set_event(EVENT_EQUIPMENT_STOP, 1);
  }

  //settings.clear();  // If this clear function is executed, no settings will be read from storage. For dev

  esp32hal->set_default_configuration_values();

  // Load everything declared in the settings tables (settings.cpp), which are
  // the single source of truth for keys, validation ranges and the storage
  // variables.
  migrate_static_ip_settings(settings);  // Must run before the string loads read LOCALIP etc.
  load_stored_settings(settings);

  // ---- Boot-only derived state, from the settings loaded above ----

  if (webserver_auth && (http_username.empty() || http_password.empty())) {
    // Disable webserver auth if the username or password is empty
    webserver_auth = false;
  }

  // The settings loaded these as strings, but we now re-read them as IPAddress
  // objects.
  wifi_static_local_IP = settings.getIP("LOCALIP");
  wifi_static_gateway = settings.getIP("GATEWAY");
  wifi_static_subnet = settings.getIP("SUBNET");
  wifi_static_dns = settings.getIP("DNS");

  // CAN interfaces: the user picks a comm_interface, the CAN layer uses
  // CAN_Interface; this mapping is applied only at boot.
  can_config.battery = remap_comm_interface(user_selected_batt_comm);
  can_config.battery_double = remap_comm_interface(user_selected_batt2_comm);
  can_config.battery_triple = remap_comm_interface(user_selected_batt3_comm);
  can_config.inverter = remap_comm_interface(user_selected_inv_comm);
  can_config.charger = remap_comm_interface(user_selected_chg_comm);
  can_config.shunt = remap_comm_interface(user_selected_shunt_comm);

  // The capacity setting is mirrored to the parallel batteries at boot.
  if (datalayer.battery.info.total_capacity_Wh != 0) {
    datalayer.battery2.info.total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
    datalayer.battery3.info.total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
  }

  setup_charge_taper_band();

  // Firmware versions before the reset interval was configurable only stored
  // the enable flag, so an upgraded installation has no PERBMSRESETH key at
  // all (the storage default 24 applies). Treat any value we don't offer in
  // the UI the same way.
  if (periodic_bms_reset_interval_h != 24 && periodic_bms_reset_interval_h != 48) {
    periodic_bms_reset_interval_h = 24;
  }

  // Guard against out-of-range values stored by older firmware.
  if (datalayer.battery.settings.sofar_user_specified_battery_id >= 16) {
    datalayer.battery.settings.sofar_user_specified_battery_id = 0;
  }

  // CTOFFSET is edited as a string but applied as a float.
  ct_clamp_offset_mV = strtof(ct_clamp_offset_text.c_str(), nullptr);
}

void clear_wifi_sta_settings() {
  BatteryEmulatorSettingsStore settings;
  settings.saveString("SSID", "");
  settings.saveString("PASSWORD", "");
  settings.saveUInt("WIFICHANNEL", 0);
  settings.saveBool("STATICIP", false);
  // Force the AP on so the device is reachable after the STA settings are cleared,
  // overriding a user preference that may have disabled it:
  settings.saveBool("WIFIAPENABLED", true);
  // Clear the static IP settings (STATICIP=false already disables their use):
  for (auto key : STATIC_IP_KEYS) {
    settings.saveString(key, "");
  }
}

void store_settings_equipment_stop() {
  BatteryEmulatorSettingsStore settings(false);
  settings.saveBool("EQUIPMENT_STOP", datalayer.system.info.equipment_stop_active);
}

// Erase RF PHY calibration data (the "phy" NVS namespace — untouched by
// clearAll(), which only clears our own settings namespace). A full RF
// calibration runs on the next boot (~100 ms extra WiFi/RF init).
void erase_phy_cal_data() {
  esp_err_t err = esp_phy_erase_cal_data_in_nvs();
  if (err == ESP_OK) {
    logging.println("RF PHY calibration data erased, full RF calibration will run on next boot.");
  } else {
    logging.printf("RF PHY calibration data erase failed (err %d)\n", err);
  }
}

void store_settings() {
  //  ATTENTION ! The maximum length for settings keys is 15 characters
  BatteryEmulatorSettingsStore settings(false);

  // Runtime-mutable settings backed by the settings tables (BATTERY_WH_MAX,
  // USE_SCALED_SOC, MAX/MINPERCENTAGE, MAXCHARGEAMP, MAXDISCHARGEAMP,
  // TARGETCHVOLT/TARGETDISCHVOLT, BMSRESETDUR, BYDAUTOCAL*, BYDKEEPISOOFF).
  store_settings_from_live(settings);

  // Voltage limits can be toggled at runtime via the legacy webserver routes.
  settings.saveBool("USEVOLTLIMITS", datalayer.battery.settings.user_set_voltage_limits_active);
}
