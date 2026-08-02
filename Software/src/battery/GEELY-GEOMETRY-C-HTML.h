#ifndef _GEELY_GEOMETRY_C_HTML_H
#define _GEELY_GEOMETRY_C_HTML_H

#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class GeelyGeometryCHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
