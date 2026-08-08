#ifndef _FORD_MACH_E_BATTERY_HTML_H
#define _FORD_MACH_E_BATTERY_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class FordMachEHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html();

 private:
  // The BMS reports standard 3-byte DTCs, but Ford service data and ford_machE_dtc.json
  // all use the 5-character short form (B11D5, U1000) built from the first two bytes only. That is
  // therefore what goes into data-dtc-code for the JSON loader to match on. The third byte is the
  // failure type: it is appended for display when set ("B11D5-2F") so nothing is silently dropped,
  // but it stays out of the lookup key.
  static String render_dtc_section(DATALAYER_BATTERY_DTC_TYPE& dtc);
  DATALAYER_BATTERY_TYPE* battery_dl;
};

#endif
