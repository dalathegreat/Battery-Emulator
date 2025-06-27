#include "BMW-I3-HTML.h"
#include "../include.h"
#include "BMW-I3-BATTERY.h"

String BmwI3HtmlRenderer::get_status_html() {
  String content;

  content += "<h4>SOC raw: " + String(batt.SOC_raw()) + "</h4>";
  content += "<h4>SOC dash: " + String(batt.SOC_dash()) + "</h4>";
  content += "<h4>SOC OBD2: " + String(batt.SOC_OBD2()) + "</h4>";
  static const char* statusText[16] = {
      "Not evaluated", "OK", "Error!", "Invalid signal", "", "", "", "", "", "", "", "", "", "", "", ""};
  content += "<h4>Interlock: " + String(statusText[batt.ST_interlock()]) + "</h4>";
  content += "<h4>Isolation external: " + String(statusText[batt.ST_iso_ext()]) + "</h4>";
  content += "<h4>Isolation internal: " + String(statusText[batt.ST_iso_int()]) + "</h4>";
  content += "<h4>Isolation: " + String(statusText[batt.ST_isolation()]) + "</h4>";
  content += "<h4>Cooling valve: " + String(statusText[batt.ST_valve_cooling()]) + "</h4>";
  content += "<h4>Emergency: " + String(statusText[batt.ST_EMG()]) + "</h4>";
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
  content += "<h4>Precharge: " + String(prechargeText[batt.ST_precharge()]) + "</h4>";  //Still unclear of enum
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
  content += "<h4>Contactor status: " + String(DCSWText[batt.ST_DCSW()]) + "</h4>";
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
  content += "<h4>Contactor weld: " + String(contText[batt.ST_WELD()]) + "</h4>";
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
  content += "<h4>Cold shutoff valve: " + String(contText[batt.ST_cold_shutoff_valve()]) + "</h4>";

  return content;
}
