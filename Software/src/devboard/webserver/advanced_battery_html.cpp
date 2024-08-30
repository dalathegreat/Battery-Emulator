#include "advanced_battery_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"

String advanced_battery_processor(const String& var) {
  if (var == "X") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content += "</style>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    content += "<h4 style='color: white;'>SSID: <span id='SSID'></span> <button onclick='editSSID()'>Edit</button></h4>";

    content += "</div>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";
    content += "<script>";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    return content;
  }
  return String();
}