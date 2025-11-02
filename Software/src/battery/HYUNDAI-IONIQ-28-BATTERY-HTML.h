#ifndef _HYUNDAI_IONIQ_28_BATTERY_HTML_H
#define _HYUNDAI_IONIQ_28_BATTERY_HTML_H

#include "../datalayer/datalayer.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class HyundaiIoniq28Battery;

class HyundaiIoniq28BatteryHtmlRenderer : public BatteryHtmlRenderer {
 private:
  HyundaiIoniq28Battery& batt;

 public:
  HyundaiIoniq28BatteryHtmlRenderer(HyundaiIoniq28Battery& b) : batt(b) {}

  String get_status_html();
};

#endif
