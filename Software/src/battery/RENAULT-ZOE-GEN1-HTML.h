#ifndef _RENAULT_ZOE_GEN1_HTML_H
#define _RENAULT_ZOE_GEN1_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RenaultZoeGen1HtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
