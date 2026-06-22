#ifndef _INTER_UNIT_CONTROLLER_HTML_H_
#define _INTER_UNIT_CONTROLLER_HTML_H_

#include "../../devboard/webserver/BatteryHtmlRenderer.h"

class InterUnitControllerHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif  // _INTER_UNIT_CONTROLLER_HTML_H_
