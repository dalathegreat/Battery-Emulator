#ifndef _BMW_I3_HTML_H
#define _BMW_I3_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class BmwI3Battery;

class BmwI3HtmlRenderer : public BatteryHtmlRenderer {
 private:
  BmwI3Battery& batt;

 public:
  BmwI3HtmlRenderer(BmwI3Battery& b) : batt(b) {}

  String get_status_html();
};

#endif
