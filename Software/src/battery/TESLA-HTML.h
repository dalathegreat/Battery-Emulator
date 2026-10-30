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

 protected:
  DATALAYER_INFO_TESLA* tesla_info;
  DATALAYER_BATTERY_TYPE* core_data;
};

// Status page for the legacy Model S/X (2012-2020) packs. The calculated values block is the Model 3/Y one (the legacy driver
// fills the same datalayer fields).
class TeslaLegacyHtmlRenderer : public TeslaHtmlRenderer {
 public:
  TeslaLegacyHtmlRenderer(DATALAYER_INFO_TESLA* extended, DATALAYER_BATTERY_TYPE* core)
      : TeslaHtmlRenderer(extended, core) {}

  String get_status_html();
};

#endif
