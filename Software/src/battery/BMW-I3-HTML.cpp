#include "BMW-I3-HTML.h"
#include "../include.h"
#include "BMW-I3-BATTERY.h"

// Helper function for safe array access
static const char* safeArrayAccess(const char* const arr[], size_t arrSize, int idx) {
  if (idx >= 0 && static_cast<size_t>(idx) < arrSize) {
    return arr[idx];
  }
  return "Unknown";
}

String BmwI3HtmlRenderer::get_status_html() {
  String content;

  content += "<h4>SOC raw: " + String(batt.SOC_raw()) + "</h4>";
  content += "<h4>SOC dash: " + String(batt.SOC_dash()) + "</h4>";
  content += "<h4>SOC OBD2: " + String(batt.SOC_OBD2()) + "</h4>";
  static const char* statusText[16] = {
      "Not evaluated", "OK", "Error!", "Invalid signal", "", "", "", "", "", "", "", "", "", "", "", ""};
  content += "<h4>Interlock: " + String(safeArrayAccess(statusText, 16, batt.ST_interlock())) + "</h4>";
  content += "<h4>Isolation external: " + String(safeArrayAccess(statusText, 16, batt.ST_iso_ext())) + "</h4>";
  content += "<h4>Isolation internal: " + String(safeArrayAccess(statusText, 16, batt.ST_iso_int())) + "</h4>";
  content += "<h4>Isolation: " + String(safeArrayAccess(statusText, 16, batt.ST_isolation())) + "</h4>";
  content += "<h4>Cooling valve: " + String(safeArrayAccess(statusText, 16, batt.ST_valve_cooling())) + "</h4>";
  content += "<h4>Emergency: " + String(safeArrayAccess(statusText, 16, batt.ST_EMG())) + "</h4>";
  static const char* prechargeText[16] = {"Not evaluated",
                                          "Not active, closing not blocked",
                                          "Error precharge blocked",
                                          "Invalid signal",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          "",
                                          ""};
  content += "<h4>Precharge: " + String(safeArrayAccess(prechargeText, 16, batt.ST_precharge())) +
             "</h4>";  //Still unclear of enum
  static const char* DCSWText[16] = {"Contactors open",
                                     "Precharge ongoing",
                                     "Contactors engaged",
                                     "Invalid signal",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     ""};
  content += "<h4>Contactor status: " + String(safeArrayAccess(DCSWText, 16, batt.ST_DCSW())) + "</h4>";
  static const char* contText[16] = {"Contactors OK",
                                     "One contactor welded!",
                                     "Two contactors welded!",
                                     "Invalid signal",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     "",
                                     ""};
  content += "<h4>Contactor weld: " + String(safeArrayAccess(contText, 16, batt.ST_WELD())) + "</h4>";
  static const char* valveText[16] = {"OK",
                                      "Short circuit to GND",
                                      "Short circuit to 12V",
                                      "Line break",
                                      "",
                                      "",
                                      "Driver error",
                                      "",
                                      "",
                                      "",
                                      "",
                                      "",
                                      "Stuck",
                                      "Stuck",
                                      "",
                                      "Invalid Signal"};
  content +=
      "<h4>Cold shutoff valve: " + String(safeArrayAccess(valveText, 16, batt.ST_cold_shutoff_valve())) + "</h4>";

  return content;
}
