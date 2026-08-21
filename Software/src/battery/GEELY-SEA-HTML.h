#ifndef _GEELY_SEA_HTML_H
#define _GEELY_SEA_HTML_H

#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class GeelySeaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
