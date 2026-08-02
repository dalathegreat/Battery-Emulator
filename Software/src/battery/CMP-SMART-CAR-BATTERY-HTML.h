#ifndef _CMP_SMART_CAR_BATTERY_HTML_H
#define _CMP_SMART_CAR_BATTERT_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class CmpSmartCarHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
