#include "KIA-E-GMP-HTML.h"
#include "KIA-E-GMP-BATTERY.h"

String KiaEGMPHtmlRenderer::get_status_html() {
  String content;
  content += "<h4>Cells: " + String(datalayer.battery.info.number_of_cells) + "S</h4>";
  content += "<h4>12V voltage: " + String(batt.get_battery_12V() / 10.0, 1) + "</h4>";
  content += "<h4>Waterleakage: " + String(batt.get_waterleakageSensor()) + "</h4>";
  content += "<h4>Temperature, water inlet: " + String(batt.get_temperature_water_inlet()) + "</h4>";
  content += "<h4>Temperature, power relay: " + String(batt.get_powerRelayTemperature() * 2) + "</h4>";
  content += "<h4>Batterymanagement mode: " + String(batt.get_batteryManagementMode()) + "</h4>";
  content += "<h4>BMS ignition: " + String(batt.get_BMS_ign()) + "</h4>";
  content += "<h4>Battery relay: " + String(batt.get_batRelay()) + "</h4>";
  return content;
}
