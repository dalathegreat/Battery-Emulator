#ifndef _MEB_HTML_H
#define _MEB_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

// Mirrors the scaling applied in MEB-BATTERY.cpp when BMS_voltage_intermediate is decoded
static constexpr int32_t meb_interm_raw_to_dV(int32_t raw) {
  return (raw - 2000) * 10 / 2;
}

class MebHtmlRenderer : public BatteryHtmlRenderer {
 public:
  // platform's battery setup() overrides
  const char* dtc_json_filename = "vag_meb_dtc.json";

  String get_status_html();

 private:
  // Emits "<h4>label: value</h4>" from separately stored fragments, so no label prefix is
  // duplicated in flash and the repeated values pool into a single copy each.
  static void add_h4(String& out, const char* label, const char* value) {
    out += "<h4>";
    out += label;
    out += ": ";
    out += value;
    out += "</h4>";
  }

  static void add_h4(String& out, const char* label, const String& value) {
    out += "<h4>";
    out += label;
    out += ": ";
    out += value;
    out += "</h4>";
  }

  // Bounds checked table lookup, reproducing the `default: "?"` of the switches this replaces.
  // Taking the array by reference deduces N, so a table and its length can never drift apart.
  template <size_t N>
  static const char* enum_str(const char* const (&table)[N], uint8_t value) {
    return value < N ? table[value] : "?";
  }
};

#endif
