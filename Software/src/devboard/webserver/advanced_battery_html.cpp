#include "advanced_battery_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_web.h"

String advanced_battery_processor(const String& var) {
  if (var == "X") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

#ifdef TESLA_BATTERY
    content += "<h4>Contactor Status: " + String(datalayer_web.tesla.status_contactor) + "</h4>";
    content += "<h4>HVIL: " + String(datalayer_web.tesla.hvil_status) + "</h4>";
    content += "<h4>Negative contactor: " + String(datalayer_web.tesla.packContNegativeState) + "</h4>";
    content += "<h4>Positive contactor: " + String(datalayer_web.tesla.packContPositiveState) + "</h4>";
    content += "<h4>Contactor set state: " + String(datalayer_web.tesla.packContactorSetState) + "</h4>";
    content += "<h4>Closing allowed?: " + String(datalayer_web.tesla.packCtrsClosingAllowed) + "</h4>";
    content += "<h4>Pyrotest: " + String(datalayer_web.tesla.pyroTestInProgress) + "</h4>";
#endif

#ifndef TESLA_BATTERY  //Only the listed types have extra info
    content += "No extra information available for this battery type";
#endif

    content += "</div>";

    content += "<script>";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    return content;
  }
  return String();
}
