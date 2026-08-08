#ifndef _RENAULT_ZOE_GEN1_HTML_H
#define _RENAULT_ZOE_GEN1_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RenaultZoeGen1HtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>CUV " + String(datalayer_extended.zoe.CUV) + "</h4>";
    content += "<h4>HVBIR " + String(datalayer_extended.zoe.HVBIR) + "</h4>";
    content += "<h4>HVBUV " + String(datalayer_extended.zoe.HVBUV) + "</h4>";
    content += "<h4>EOCR " + String(datalayer_extended.zoe.EOCR) + "</h4>";
    content += "<h4>HVBOC " + String(datalayer_extended.zoe.HVBOC) + "</h4>";
    content += "<h4>HVBOT " + String(datalayer_extended.zoe.HVBOT) + "</h4>";
    content += "<h4>HVBOV " + String(datalayer_extended.zoe.HVBOV) + "</h4>";
    content += "<h4>COV " + String(datalayer_extended.zoe.COV) + "</h4>";
    tr_h4(content, TrKey::DRV_BATTERY_MILEAGE, String(datalayer_extended.zoe.mileage_km), " km");
    tr_h4(content, TrKey::DRV_ALLTIME_ENERGY, String(datalayer_extended.zoe.alltime_kWh), " kWh");
    return content;
  }
};

#endif
