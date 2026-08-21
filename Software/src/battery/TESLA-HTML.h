#ifndef _TESLA_HTML_H
#define _TESLA_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class TeslaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
