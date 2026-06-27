#ifndef SMART_EQ_453_HTML_H
#define SMART_EQ_453_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class SmartEq453HtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String s;
    s += "<h4>Smart EQ / Renault 453 BMS</h4>";
    s += "<p>Support: experimental — simulation only, no physical contactor control.</p>";
    s += "<p>See <code>SMART-EQ-453-BATTERY.h</code> for safety notes.</p>";
    return s;
  }
};

#endif
