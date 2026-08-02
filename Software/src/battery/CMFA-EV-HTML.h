#ifndef _CMFA_EV_HTML_H
#define _CMFA_EV_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

class CmfaEvHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
