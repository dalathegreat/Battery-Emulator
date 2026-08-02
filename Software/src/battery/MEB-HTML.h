#ifndef _MEB_HTML_H
#define _MEB_HTML_H

#include "../devboard/webserver/BatteryHtmlRenderer.h"

// Mirrors the scaling applied in MEB-BATTERY.cpp when BMS_voltage_intermediate is decoded
static constexpr int32_t meb_interm_raw_to_dV(int32_t raw) {
  return (raw - 2000) * 10 / 2;
}

class MebHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();
};

#endif
