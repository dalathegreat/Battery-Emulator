#ifndef _KIA_E_GMP_HTML_H
#define _KIA_E_GMP_HTML_H

#include "../datalayer/datalayer.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class KiaEGmpBattery;

class KiaEGMPHtmlRenderer : public BatteryHtmlRenderer {
 private:
  KiaEGmpBattery& batt;

 public:
  KiaEGMPHtmlRenderer(KiaEGmpBattery& b) : batt(b) {}
  String get_status_html();
};

#endif
