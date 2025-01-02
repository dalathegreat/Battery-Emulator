#include "can_logging_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

String can_logger_processor(void) {
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
  content += "<button onclick='refreshPage()'>Refresh data</button> ";
  content += "<button onclick='exportLog()'>Export to .txt</button> ";
#ifdef LOG_CAN_TO_SD
  content += "<button onclick='deleteLogFile()'>Delete log file</button> ";
#endif
  content += "<button onclick='stopLoggingAndGoToMainPage()'>Stop &amp; Back to main page</button>";

  // Start a new block for the CAN messages
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px'>";

  // Check for messages
  if (datalayer.system.info.logged_can_messages[0] == 0) {
    content += "CAN logger started! Refresh page to display incoming(RX) and outgoing(TX) messages";
  } else {
    // Split the messages using the newline character
    String messages = String(datalayer.system.info.logged_can_messages);
    int startIndex = 0;
    int endIndex = messages.indexOf('\n');
    while (endIndex != -1) {
      // Extract a single message and wrap it in a styled div
      String singleMessage = messages.substring(startIndex, endIndex);
      content += "<div class='can-message'>" + singleMessage + "</div>";
      startIndex = endIndex + 1;  // Move past the newline character
      endIndex = messages.indexOf('\n', startIndex);
    }
  }

  content += "</div>";

  // Add JavaScript for navigation
  content += "<script>";
  content += "function refreshPage(){ location.reload(true); }";
  content += "function exportLog() { window.location.href = '/export_can_log'; }";
#ifdef LOG_CAN_TO_SD
  content += "function deleteLogFile() { window.location.href = '/delete_can_log'; }";
#endif
  content += "function stopLoggingAndGoToMainPage() {";
  content += "  fetch('/stop_can_logging').then(() => window.location.href = '/');";
  content += "}";
  content += "</script>";
  content += index_html_footer;
  return content;
}
