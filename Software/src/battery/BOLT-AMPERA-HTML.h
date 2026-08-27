#ifndef _BOLT_AMPERA_HTML_H
#define _BOLT_AMPERA_HTML_H

#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BoltAmperaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  BoltAmperaHtmlRenderer(DATALAYER_INFO_BOLTAMPERA* dl) : boltampera_dl(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html();

 private:
  DATALAYER_INFO_BOLTAMPERA* boltampera_dl;
};

#endif
