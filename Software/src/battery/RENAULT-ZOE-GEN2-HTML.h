#ifndef _RENAULT_ZOE_GEN2_HTML_H
#define _RENAULT_ZOE_GEN2_HTML_H

#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RenaultZoeGen2HtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
