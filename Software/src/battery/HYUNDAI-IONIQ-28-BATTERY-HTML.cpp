#include "HYUNDAI-IONIQ-28-BATTERY-HTML.h"
#include "../devboard/i18n/tr.h"
#include "HYUNDAI-IONIQ-28-BATTERY.h"

String HyundaiIoniq28BatteryHtmlRenderer::get_status_html() {
  String content;
  tr_h4(content, TrKey::DRV_12V_VOLTAGE, String(batt.get_lead_acid_voltage() / 10.0f, 1));
  tr_h4(content, TrKey::DRV_TEMPERATURE_POWER_RELAY, String(batt.get_power_relay_temperature()));
  tr_h4(content, TrKey::DRV_BATTERYMANAGEMENT_MODE, String(batt.get_battery_management_mode()));
  tr_h4(content, TrKey::DRV_ISOLATION_RESISTANCE, String(batt.get_isolation_resistance()), " kOhm");
  return content;
}
