#ifndef _TESLA_HTML_H
#define _TESLA_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class TeslaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  TeslaHtmlRenderer(DATALAYER_INFO_TESLA* extended, DATALAYER_BATTERY_TYPE* core)
      : tesla_info(extended), core_data(core) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html();

 private:
  DATALAYER_INFO_TESLA* tesla_info;
  DATALAYER_BATTERY_TYPE* core_data;
};

#endif
