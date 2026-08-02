#ifndef _BMW_PHEV_HTML_H
#define _BMW_PHEV_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BmwPhevHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
