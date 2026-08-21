#ifndef _ECMP_BATTERY_HTML_H
#define _ECMP_BATTERT_HTML_H

#include "../datalayer/datalayer.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class EcmpHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
