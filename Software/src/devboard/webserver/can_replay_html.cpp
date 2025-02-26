#include "can_replay_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

String can_replay_processor(void) {
  if (!datalayer.system.info.can_logging_active) {
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
  }
  datalayer.system.info.can_logging_active =
      true;  // Signal to main loop that we should log messages. Disabled by default for performance reasons
  String content = index_html_header;
  // Page format
  content += "<style>";
  content += "body { background-color: black; color: white; font-family: Arial, sans-serif; }";
  content +=
      "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; "
      "cursor: pointer; border-radius: 10px; }";
  content += "button:hover { background-color: #3A4A52; }";
  content +=
      ".can-message { background-color: #404E57; margin-bottom: 5px; padding: 10px; border-radius: 5px; font-family: "
      "monospace; }";
  content += "</style>";
  content += "<button onclick='importLog()'>Import .txt</button> ";
  content += "<button onclick='stopPlaybackAndGoToMainPage()'>Stop &amp; Back to main page</button>";

  // Start a new block for the CAN messages
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px'>";

  // Ask user to select which CAN interface log should be sent to
  content += "<h4>Select CAN Interface for Playback</h4>";

  // Dropdown with choices
  content += "<label for='canInterface'>CAN Interface:</label>";
  content += "<select id='canInterface' name='canInterface'>";
  content += "<option value='" + String(CAN_NATIVE) + "'>CAN Native</option>";
  content += "<option value='" + String(CANFD_NATIVE) + "'>CANFD Native</option>";
  content += "<option value='" + String(CAN_ADDON_MCP2515) + "'>CAN Addon MCP2515</option>";
  content += "<option value='" + String(CANFD_ADDON_MCP2518) + "'>CANFD Addon MCP2518</option>";
  content += "</select>";

  // Add a button to submit the selected CAN interface
  content += "<button onclick='sendCANSelection()'>Apply</button>";

  // Show text that log file is not loaded yet
  content += "<h4>Log file not loaded yet. Import log before starting replay!</h4>";

  // Add a button to start playing the log
  content += "<button onclick='startReplay()'>Start</button> ";

  // Add a button to stop playing the log
  content += "<button onclick='startReplay()'>Stop</button> ";

  // Add a checkbox to loop the log
  //content += "<input type="checkbox" id="myCheck" onclick="myFunction()">";

  content += "</div>";

  // Add JavaScript for navigation
  content += "<script>";
  content += "function sendCANSelection() {";
  content += "  var selectedInterface = document.getElementById('canInterface').value;";
  content += "  var xhr = new XMLHttpRequest();";
  content += "  xhr.open('GET', '/setCANInterface?interface=' + selectedInterface, true);";
  content += "  xhr.send();";
  content += "}";
  content += "function importLog() { window.location.href = '/import_can_log'; }";
  content += "function stopPlaybackAndGoToMainPage() {";
  content += "  fetch('/stop_can_logging').then(() => window.location.href = '/');";
  content += "}";
  content += "</script>";
  content += index_html_footer;
  return content;
}
