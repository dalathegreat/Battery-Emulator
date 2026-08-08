#ifndef _CELLPOWER_HTML_H
#define _CELLPOWER_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class CellpowerHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
