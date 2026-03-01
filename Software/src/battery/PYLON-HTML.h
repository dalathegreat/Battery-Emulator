#ifndef _PYLON_HTML_H
#define _PYLON_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class PylonHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h4>Charge cutoff voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
    content += "<h4>Discharge cutoff voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
    return content;
  }
};

#endif
