#include "HYUNDAI-IONIQ-28-BATTERY-HTML.h"
#include "HYUNDAI-IONIQ-28-BATTERY.h"

String HyundaiIoniq28BatteryHtmlRenderer::get_status_html() {
  String content;
  content += "<h4>12V voltage: " + String(batt.get_lead_acid_voltage() / 10.0, 1) + "</h4>";
  content += "<h4>Temperature, power relay: " + String(batt.get_power_relay_temperature()) + "</h4>";
  content += "<h4>Batterymanagement mode: " + String(batt.get_battery_management_mode()) + "</h4>";
  content += "<h4>Isolation resistance: " + String(batt.get_isolation_resistance()) + " kOhm</h4>";
  return content;
}
