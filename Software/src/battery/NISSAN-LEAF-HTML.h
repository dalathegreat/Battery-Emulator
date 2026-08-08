#ifndef _NISSAN_LEAF_HTML_H
#define _NISSAN_LEAF_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class NissanLeafHtmlRenderer : public BatteryHtmlRenderer {
 public:
  NissanLeafHtmlRenderer(DATALAYER_BATTERY_TYPE* battery_dl, DATALAYER_INFO_NISSAN_LEAF* dl)
      : battery_dl(battery_dl), nissan_dl(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html();

 private:
  // The LBC reports standard 3-byte DTCs, but Nissan service data, LeafSpy and nissan_leaf_dtc.json
  // all use the 5-character short form (P33D7, U1000) built from the first two bytes only. That is
  // therefore what goes into data-dtc-code for the JSON loader to match on. The third byte is the
  // failure type: it is appended for display when set ("P33D7-2F") so nothing is silently dropped,
  // but it stays out of the lookup key.
  static String render_dtc_section(DATALAYER_BATTERY_DTC_TYPE& dtc);

  DATALAYER_BATTERY_TYPE* battery_dl;
  DATALAYER_INFO_NISSAN_LEAF* nissan_dl;
};

#endif
