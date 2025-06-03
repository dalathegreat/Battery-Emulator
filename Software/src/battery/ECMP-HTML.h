#ifndef _ECMP_HTML_H
#define _ECMP_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class EcmpHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>Main Connector State: ";
    if (datalayer_extended.stellantisECMP.MainConnectorState == 0) {
      content += "Contactors open</h4>";
    } else if (datalayer_extended.stellantisECMP.MainConnectorState == 0x01) {
      content += "Precharged</h4>";
    } else {
      content += "Invalid</h4>";
    }
    content +=
        "<h4>Insulation Resistance: " + String(datalayer_extended.stellantisECMP.InsulationResistance) + "kOhm</h4>";

    return content;
  }
};

#endif
