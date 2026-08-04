#ifndef _RENAULT_ZOE_GEN1_HTML_H
#define _RENAULT_ZOE_GEN1_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RenaultZoeGen1Battery;

class RenaultZoeGen1HtmlRenderer : public BatteryHtmlRenderer {
 public:
  explicit RenaultZoeGen1HtmlRenderer(RenaultZoeGen1Battery& battery) : battery(battery) {}
  String get_status_html() override;
  bool renders_own_battery_data() override { return true; }

 private:
  RenaultZoeGen1Battery& battery;
};

#endif
