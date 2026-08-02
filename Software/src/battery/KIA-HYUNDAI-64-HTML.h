#ifndef _KIA_HYUNDAI_64_HTML_H
#define _KIA_HYUNDAI_64_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class KiaHyundai64HtmlRenderer : public BatteryHtmlRenderer {
 public:
  KiaHyundai64HtmlRenderer(DATALAYER_INFO_KIAHYUNDAI64* dl) : kia_datalayer(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html();

 private:
  DATALAYER_INFO_KIAHYUNDAI64* kia_datalayer;
};

#endif
