#ifndef _INTER_UNIT_CONTROLLER_HTML_H_
#define _INTER_UNIT_CONTROLLER_HTML_H_

#ifndef SMALL_FLASH_DEVICE

#include "../../devboard/webserver/BatteryHtmlRenderer.h"

class InterUnitControllerHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif  // SMALL_FLASH_DEVICE

#endif  // _INTER_UNIT_CONTROLLER_HTML_H_
