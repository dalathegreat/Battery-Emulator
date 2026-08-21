#ifndef _BYD_ATTO_3_HTML_H
#define _BYD_ATTO_3_HTML_H

#include <Arduino.h>
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BydAtto3HtmlRenderer : public BatteryHtmlRenderer {
 public:
  BydAtto3HtmlRenderer(DATALAYER_INFO_BYDATTO3* dl, const String& sfx = "") : byd_datalayer(dl), s(sfx) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html();

 private:
  DATALAYER_INFO_BYDATTO3* byd_datalayer;
  String s;
};

#endif
