#ifndef _BMW_IX_HTML_H
#define _BMW_IX_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BmwIXBattery;

class BmwIXHtmlRenderer : public BatteryHtmlRenderer {
 private:
  BmwIXBattery& batt;

 public:
  BmwIXHtmlRenderer(BmwIXBattery& b) : batt(b) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html();
};

#endif
