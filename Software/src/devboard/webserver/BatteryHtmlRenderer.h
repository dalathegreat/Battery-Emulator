#ifndef _BATTERY_HTML_RENDERER_H
#define _BATTERY_HTML_RENDERER_H

#include <WString.h>

// Each battery can implement this interface to render more battery specific HTML
// content
class BatteryHtmlRenderer {
 public:
  virtual String get_status_html() = 0;
};

class BatteryDefaultRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() { return String("No extra information available for this battery type"); }
};

#endif
