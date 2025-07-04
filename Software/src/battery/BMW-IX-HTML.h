#ifndef _BMW_IX_HTML_H
#define _BMW_IX_HTML_H

#include "../datalayer/datalayer.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class BmwIXBattery;

class BmwIXHtmlRenderer : public BatteryHtmlRenderer {
 private:
  BmwIXBattery& batt;

 public:
  BmwIXHtmlRenderer(BmwIXBattery& b) : batt(b) {}

  String get_status_html();
};

#endif
