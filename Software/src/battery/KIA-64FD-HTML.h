#ifndef _KIA_64FD_HTML_H
#define _KIA_64FD_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class Kia64FDHtmlRenderer : public BatteryHtmlRenderer {
 public:
  Kia64FDHtmlRenderer(DATALAYER_INFO_KIA64FD* dl) : kia_datalayer(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    DATALAYER_INFO_KIA64FD& data = *kia_datalayer;

    content += "<h4>SOC (BMS): " + String(data.SOC_BMS / 10.0f, 1) + " %</h4>";
    content += "<h4>SOC (display): " + String(data.SOC_Display / 10.0f, 1) + " %</h4>";
    content += "<h4>SOC (estimated from lowest cell): " + String(data.SOC_estimated_lowest / 100.0f, 2) + " %</h4>";
    content += "<h4>SOC (estimated from highest cell): " + String(data.SOC_estimated_highest / 100.0f, 2) + " %</h4>";
    content += "<h4>SOH: " + String(data.batterySOH / 10.0f, 1) + " %</h4>";
    content += "<h4>Allowed charge power: " + String(data.allowedChargePower / 100.0f, 2) + " kW</h4>";
    content += "<h4>Allowed discharge power: " + String(data.allowedDischargePower / 100.0f, 2) + " kW</h4>";
    content += "<h4>Highest cell: no " + String(data.CellVmaxNo) + "</h4>";
    content += "<h4>Lowest cell: no " + String(data.CellVminNo) + "</h4>";
    content += "<h4>12V voltage: " + String(data.leadAcidBatteryVoltage / 10.0f, 1) + " V</h4>";
    content += "<h4>Inverter voltage: " + String(data.inverterVoltage) + " V</h4>";
    content += "<h4>Temperature, water inlet: " + String(data.temperature_water_inlet) + " &deg;C</h4>";
    content += "<h4>Temperature, heater: " + String(data.heatertemp) + " &deg;C</h4>";
    content += "<h4>Batterymanagement mode: " + String(data.batteryManagementMode) + "</h4>";
    content += "<h4>BMS ignition: " + String(data.BMS_ign) + "</h4>";

    return content;
  }

 private:
  DATALAYER_INFO_KIA64FD* kia_datalayer;
};

#endif
