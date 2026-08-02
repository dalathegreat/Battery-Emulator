#include "KIA-E-GMP-HTML.h"
#include "../devboard/i18n/tr.h"
#include "KIA-E-GMP-BATTERY.h"

String KiaEGMPHtmlRenderer::get_status_html() {
  String content;
  tr_h4(content, TrKey::DRV_CELLS, String(datalayer.battery.info.number_of_cells), "S");
  tr_h4(content, TrKey::DRV_12V_VOLTAGE, String(batt.get_battery_12V() / 10.0f, 1));
  tr_h4(content, TrKey::DRV_WATERLEAKAGE, String(batt.get_waterleakageSensor()));
  tr_h4(content, TrKey::DRV_TEMPERATURE_WATER_INLET, String(batt.get_temperature_water_inlet()));
  tr_h4(content, TrKey::DRV_TEMPERATURE_POWER_RELAY, String(batt.get_powerRelayTemperature() * 2));
  tr_h4(content, TrKey::DRV_BATTERYMANAGEMENT_MODE, String(batt.get_batteryManagementMode()));
  tr_h4(content, TrKey::DRV_BMS_IGNITION, String(batt.get_BMS_ign()));
  tr_h4(content, TrKey::DRV_BATTERY_RELAY, String(batt.get_batRelay()));
  return content;
}
