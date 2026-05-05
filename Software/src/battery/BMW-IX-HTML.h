#ifndef _BMW_IX_HTML_H
#define _BMW_IX_HTML_H

#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BmwIXBattery;

class BmwIXHtmlRenderer : public BatteryHtmlRenderer {
 private:
  BmwIXBattery& batt;

 public:
  BmwIXHtmlRenderer(BmwIXBattery& b) : batt(b) {}

  String getDTCDescription(uint32_t code);
  String get_status_html();
};

#endif
