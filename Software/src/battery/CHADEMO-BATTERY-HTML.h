#ifndef _CHADEMO_BATTERY_HTML_H
#define _CHADEMO_BATTERY_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class ChademoBatteryHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
