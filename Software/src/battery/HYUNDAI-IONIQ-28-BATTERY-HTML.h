#ifndef _HYUNDAI_IONIQ_28_BATTERY_HTML_H
#define _HYUNDAI_IONIQ_28_BATTERY_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class HyundaiIoniq28BatteryHtmlRenderer : public BatteryHtmlRenderer {
 public:
  HyundaiIoniq28BatteryHtmlRenderer(DATALAYER_INFO_IONIQ28* dl) : hyundai_datalayer(dl) {}

  String get_status_html() {
    String content;

    auto print_hyundai = [&content](DATALAYER_INFO_IONIQ28& data) {
      content += "<h4>12V voltage: " + String(data.battery_12V / 10.0, 1) + "</h4>";
      content += "<h4>Temperature, water inlet: " + String(data.temperature_water_inlet) + "</h4>";
      content += "<h4>Temperature, power relay: " + String(data.powerRelayTemperature) + "</h4>";
      content += "<h4>Batterymanagement mode: " + String(data.batteryManagementMode) + "</h4>";
      content += "<h4>BMS ignition: " + String(data.BMS_ign) + "</h4>";
      content += "<h4>Battery relay: " + String(data.batteryRelay) + "</h4>";
    };

    print_hyundai(*hyundai_datalayer);

    return content;
  }

 private:
  DATALAYER_INFO_IONIQ28* hyundai_datalayer;
};

#endif
